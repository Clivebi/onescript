#include "./network/tcp.cc"

#include <array>

#include "../vm.hpp"
#include "sstream"

#ifndef CHECK_PARAMETER_COUNT
inline std::string check_error(int i, const char* type) {
    std::stringstream s;
    s << " :the #" << i << " argument must be an " << type << std::endl;
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
#endif

Value TCPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    std::string host = args[0].bytes, port;
    if (args[1].Type == ValueType::kInteger) {
        port = args[1].ToString();
    } else {
        CHECK_PARAMETER_STRING(1);
        port = args[1].bytes;
    }
    CHECK_PARAMETER_INTEGER(2);
    CHECK_PARAMETER_INTEGER(3);
    Resource* res = NewTCPStream(host, port, (int)args[2].Integer, args[3].ToBoolen());
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value TCPRead(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    if (args[1].Integer > 1 * 1024 * 1024) {
        throw RuntimeException("TCPRead length must less 1M");
    }

    unsigned char* buffer = (unsigned char*)malloc((size_t)args[1].Integer);
    if (buffer == NULL) {
        return Value();
    }
    TCPStream* stream = (TCPStream*)(args[0].resource.get());
    int size = stream->Recv(buffer, (int)args[1].Integer);
    if (size < 0) {
        free(buffer);
        return Value();
    }
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)buffer, size);
    free(buffer);
    return ret;
}

Value TCPWrite(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    TCPStream* stream = (TCPStream*)(args[0].resource.get());
    int size = stream->Send(args[1].bytes.c_str(), args[1].bytes.size());
    return Value(size);
}

#ifndef COUNT_OF
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#endif

BuiltinMethod tcpMethod[] = {
        {"TCPConnect", TCPConnect},
        {"TCPWrite", TCPWrite},
        {"TCPRead", TCPRead},
};

void RegisgerTcpBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(tcpMethod, COUNT_OF(tcpMethod));
}