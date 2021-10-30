#include "../vm.hpp"

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

#define CHECK_PARAMETER_MAP(i)                                                     \
    if (args[i].Type != ValueType::kMap) {                                         \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "map")); \
    }

#define CHECK_PARAMETER_ARRAY(i)                                                     \
    if (args[i].Type != ValueType::kMap) {                                         \
        throw RuntimeException(std::string(__FUNCTION__) + check_error(i, "array")); \
    }
#endif

using namespace Interpreter;
Value ParseJSON(std::string& str);

Value JSONDecode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    return ParseJSON(args[0].bytes);
}

Value JSONEncode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    return args[0].ToJSONString();
}

#ifndef COUNT_OF
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#endif

BuiltinMethod jsonMethod[] = {{"JSONDecode", JSONDecode},
                              {"JSONEncode", JSONEncode}};

void RegisgerJsonBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(jsonMethod, COUNT_OF(jsonMethod));
}