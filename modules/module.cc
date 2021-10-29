#include "bytes.cc"
#include "tcp.cc"
#include "http.cc"

void RegisgerModulesBuiltinMethod(Executor* vm) {
    RegisgerBytesBuiltinMethod(vm);
    RegisgerTcpBuiltinMethod(vm);
    RegisgerHttpBuiltinMethod(vm);
}


#include "thirdpart/http-parser/http_parser.c"