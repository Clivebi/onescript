
#ifndef TCP_IMPL
#include "./network/tcp.cc"
#endif
#include <brotli/decode.h>
#include <zlib.h>

#include "../vm.hpp"
#include "sstream"
#include "thirdpart/http-parser/http_parser.h"

#ifndef CHECK_PARAMETER_COUNT
inline std::string check_error(int i, const char* type) {
    std::stringstream s;
    s << "the #" << i << " argument must be an " << type << std::endl;
    ;
    return s.str();
}

#define CHECK_PARAMETER_COUNT(count)                                    \
    if (args.size() < count) {                                          \
        throw RuntimeException(std::string(__FUNCTION__) +              \
                               ": the count of parameters not enough"); \
    }

#define CHECK_PARAMETER_STRING(i)                                                              \
    if (args[i].Type != ValueType::kBytes && args[i].Type != ValueType::kString) {             \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "string or bytes")); \
    }

#define CHECK_PARAMETER_INTEGER(i)                                                     \
    if (args[i].Type != ValueType::kInteger) {                                         \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "integer")); \
    }

#define CHECK_PARAMETER_RESOURCE(i)                                                     \
    if (args[i].Type != ValueType::kResource) {                                         \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "resource")); \
    }

#define CHECK_PARAMETER_MAP(i)                                                     \
    if (args[i].Type != ValueType::kMap) {                                         \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "map")); \
    }
#endif

struct HeaderValue {
    std::string Name;
    std::string Value;
};

class HTTPResponse {
public:
    static const int kFIELD = 2;
    static const int kVALUE = 3;

public:
    explicit HTTPResponse()
            : Version(""),
              Status(""),
              HeaderField(-1),
              IsHeadCompleteCalled(false),
              IsMessageCompleteCalled(false),
              ChunkedTotalBodySize(0),
              Reason(""),
              Body(""),
              RawHeader(""),
              Header() {}
    std::string Version;
    std::string Status;
    std::string Reason;
    std::string Body;
    std::string RawHeader;
    std::vector<HeaderValue*> Header;

    Value ToValue() {
        Value ret = Value::make_map();
        ret._map["version"] = Version;
        ret._map["status"] = Status;
        ret._map["reason"] = Reason;
        ret._map["raw_header"] = RawHeader;
        ret._map["body"] = Value::make_bytes(Body);
        Value h = Value::make_map();
        std::vector<HeaderValue*>::iterator iter = Header.begin();
        while (iter != Header.end()) {
            h._map[(*iter)->Name] = (*iter)->Value;
            iter++;
        }
        ret._map["headers"] = h;
        return ret;
    }
    //helper
    int HeaderField;
    bool IsHeadCompleteCalled;
    bool IsMessageCompleteCalled;
    int64_t ChunkedTotalBodySize;

public:
    std::string GetHeaderValue(std::string name) {
        auto iter = Header.begin();
        while (iter != Header.end()) {
            if ((*iter)->Name == name) {
                return (*iter)->Value;
            }
            iter++;
        }
        return "";
    }
    void ParserFirstLine() {
        size_t i = RawHeader.find("\r\n");
        ParseStatus(RawHeader.substr(0, i));
    }

protected:
    bool ParseStatus(std::string line) {
        size_t i = line.find(" ");
        if (i == std::string::npos) {
            return false;
        }
        Version = line.substr(0, i);
        if (i + 1 >= line.size()) {
            return false;
        }
        line = line.substr(i + 1);
        i = line.find(" ");
        if (i == std::string::npos) {
            return false;
        }
        Status = line.substr(0, i);
        if (i + 1 >= line.size()) {
            return false;
        }
        Reason = line.substr(i + 1);
        return true;
    }
};

int header_status_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->Version.append(buf, len);
    return 0;
}

int header_field_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    HeaderValue* element = NULL;
    if (resp->HeaderField != HTTPResponse::kFIELD) {
        element = new HeaderValue();
        resp->Header.push_back(element);
    } else {
        element = resp->Header.back();
    }
    element->Name.append(buf, len);
    resp->HeaderField = HTTPResponse::kFIELD;
    return 0;
}

int header_value_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    HeaderValue* element = resp->Header.back();
    element->Value.append(buf, len);
    resp->HeaderField = HTTPResponse::kVALUE;
    return 0;
}

int headers_complete_cb(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->IsHeadCompleteCalled = true;
    return 0;
}

int on_chunk_header(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->ChunkedTotalBodySize += p->content_length;
    return 0;
}

int on_body_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->Body.append(buf, len);
    return 0;
}

int on_message_complete_cb(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->IsMessageCompleteCalled = true;
    return 0;
}

void parser_url(std::string& url, std::string& scheme, std::string& host, std::string& port,
                std::string& path, std::string& args) {
    size_t i = url.find(":");
    if (i == std::string::npos) {
        throw RuntimeException(url + " is not a valid URL");
    }
    scheme = url.substr(0, i);
    i += 3;
    if (i > url.size()) {
        throw RuntimeException(url + " is not a valid URL");
    }
    std::string part = url.substr(i);
    i = part.find("/");
    std::string host_with_port = "";
    if (i == std::string::npos) {
        host_with_port = part;
        part = "/";
    } else {
        host_with_port = part.substr(0, i);
        part = part.substr(i);
    }
    std::list<std::string> host_port = split(host_with_port, ':');
    if (host_port.size() > 0) {
        host = host_port.front();
    }
    if (host_port.size() > 1) {
        port = host_port.back();
    }
    i = part.find("?");
    if (i == std::string::npos) {
        path = part;
        return;
    }
    path = part.substr(0, i);
    if (part.size() > i + 1) {
        args = part.substr(i + 1);
    }
}

bool DeflateStream(std::string& src, std::string& out) {
    z_stream strm = {0};
    inflateInit2(&strm, 32 + 15);
    size_t process_size = 0;
    Bytef* data = (Bytef*)src.c_str();

    int ret = Z_OK;
    strm.avail_in = src.size();
    strm.next_in = data;
    char* buffer = (char*)malloc(8 * 1024);
    while (strm.avail_in > 0) {
        strm.avail_out = 8 * 1024;
        strm.next_out = (Bytef*)buffer;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);
        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            free(buffer);
            return false;
        }
        out.append(buffer, 8 * 1024 - strm.avail_out);
    }
    inflateEnd(&strm);
    free(buffer);
    return true;
}

bool BrotliDecompress(std::string& src, std::string& out) {
    BrotliDecoderResult result;
    BrotliDecoderState* state = BrotliDecoderCreateInstance(0, 0, 0);
    if (state == NULL) {
        return false;
    }
    const unsigned char* next_in = (const unsigned char*)src.c_str();
    size_t avail_in = src.size();
    size_t total_out = 0;
    char* buffer = (char*)malloc(8 * 1024);

    result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
    out.clear();
    while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
        char* next_out = buffer;
        size_t avail_out = 8 * 1024;

        result = BrotliDecoderDecompressStream(state, &avail_in, &next_in, &avail_out,
                                               reinterpret_cast<uint8_t**>(&next_out), &total_out);

        out.append(next_out, total_out);
    }
    BrotliDecoderDestroyInstance(state);
    free(buffer);
    return result == BROTLI_DECODER_RESULT_SUCCESS;
}

struct StreamSearch {
    std::string keyword;
    int pos;
    StreamSearch(const char* key) : keyword(key), pos(0) {}
    int process(const char* buf, int size) {
        for (int i = 0; i < size; i++) {
            if (keyword[pos] != buf[i]) {
                pos = 0;
                continue;
            }
            pos++;
            if (pos == keyword.size()) {
                return i;
            }
        }
        return 0;
    }
};

bool DoReadHttpResponse(scoped_refptr<TCPStream> stream, HTTPResponse* resp) {
    StreamSearch search("\r\n\r\n");
    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = resp;
    http_parser_settings settings = {0};
    settings.on_body = on_body_cb;
    settings.on_chunk_header = on_chunk_header;
    settings.on_status = header_status_cb;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = headers_complete_cb;
    settings.on_message_complete = on_message_complete_cb;
    char buffer[1024] = {0};
    int matched = 0;
    while (true) {
        int size = stream->Recv(buffer, 1024);
        if (size < 0) {
            return false;
        }
        if (!matched) {
            matched = search.process(buffer, size);
            if (matched) {
                resp->RawHeader.append(buffer, matched);
                resp->ParserFirstLine();
            } else {
                resp->RawHeader.append(buffer, size);
            }
        }
        int parse_size = http_parser_execute(&parser, &settings, buffer, size);
        if (parser.http_errno != 0) {
            return false;
        }
        if (resp->IsMessageCompleteCalled) {
            break;
        }
    }
    std::string encoding = resp->GetHeaderValue("Content-Encoding");
    if (-1 != encoding.find("gzip") || -1 != encoding.find("deflate")) {
        std::string out = "";
        if (!DeflateStream(resp->Body, out)) {
            return false;
        }
        resp->Body = out;
        return true;
    }
    if (-1 != encoding.find("br")) {
        std::string out = "";
        if (!BrotliDecompress(resp->Body, out)) {
            return false;
        }
        resp->Body = out;
        return true;
    }
    return true;
}

bool DoHttpRequest(std::string& host, std::string& port, bool isSSL, std::string& req,
                   HTTPResponse* resp) {
    scoped_refptr<TCPStream> tcp = NewTCPStream(host, port, 60, isSSL);
    if (tcp.get() == NULL) {
        return false;
    }
    int size = tcp->Send(req.c_str(), req.size());
    if (size != req.size()) {
        return false;
    }
    return DoReadHttpResponse(tcp, resp);
}

Value ReadHttpResponse(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    scoped_refptr<TCPStream> tcp = (TCPStream*)args[0].resource.get();
    HTTPResponse resp;
    if (DoReadHttpResponse(tcp, &resp)) {
        return resp.ToValue();
    }
    return Value();
}

Value DeflateBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    DeflateStream(args[0].bytes, ret.bytes);
    return ret;
}

Value BrotliDecompressBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    BrotliDecompress(args[0].bytes, ret.bytes);
    return ret;
}

Value URLParse(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::string host, port, scheme, path, querys;
    parser_url(args[0].bytes, scheme, host, port, path, querys);
    Value ret = Value::make_map();
    ret._map["host"] = host;
    ret._map["port"] = port;
    ret._map["scheme"] = scheme;
    ret._map["path"] = path;
    ret._map["query"] = querys;
    return ret;
}

Value HttpGet(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);

    std::string host, port, scheme, path, querys;
    parser_url(args[0].bytes, scheme, host, port, path, querys);
    if (port.size() == 0) {
        if (scheme == "https") {
            port = "443";
        } else {
            port = "80";
        }
    }
    args[1]._map["Connection"] = "close";
    bool is_have_host = false;
    std::stringstream o;
    o << "GET " << path << " HTTP/1.1\r\n";
    if (args.size() == 2) {
        CHECK_PARAMETER_MAP(1);
        std::map<Value, Value>::iterator iter = args[1]._map.begin();
        while (iter != args[1]._map.end()) {
            if (iter->first.Type != ValueType::kBytes && iter->first.Type != ValueType::kString) {
                throw RuntimeException("HttpGet :map key must string or bytes");
            }
            if (iter->first.bytes == "Host") {
                is_have_host = true;
            }
            o << iter->first.bytes << ": " << iter->second.bytes << "\r\n";
            iter++;
        }
    }
    if (!is_have_host) {
        o << "Host: " << host;
        if (port != "443" && port != "80") {
            o << ":" << port;
        }
        o << "\r\n";
    }
    o << "\r\n";
    std::string req = o.str();
    HTTPResponse resp;
    if (!DoHttpRequest(host, port, scheme == "https", req, &resp)) {
        return Value();
    }
    return resp.ToValue();
}

#ifndef COUNT_OF
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#endif

BuiltinMethod httpMethod[] = {{"HttpGet", HttpGet},
                              {"DeflateBytes", DeflateBytes},
                              {"DeflateString", DeflateBytes},
                              {"BrotliDecompressBytes", BrotliDecompressBytes},
                              {"BrotliDecompressString", BrotliDecompressBytes},
                              {"URLParse", URLParse},
                              {"ReadHttpResponse", ReadHttpResponse}};

void RegisgerHttpBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(httpMethod, COUNT_OF(httpMethod));
}