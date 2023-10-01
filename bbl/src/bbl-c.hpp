#pragma once

#include "bbl.hpp"
#include "bbl_detail.h"
#include "expr.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bbl {

struct C_StructId {
    std::string id;
};

struct C_FunctionId {
    std::string id;
};

struct C_EnumId {
    std::string id;
};

struct C_StdFunctionId {
    std::string id;
};

struct C_Type {
    std::variant<bbl_builtin_t, C_StructId, C_EnumId, C_StdFunctionId> kind;
};

class C_QType;

struct C_Pointer {
    std::shared_ptr<C_QType> pointee;
};

struct C_Array {
    std::shared_ptr<C_QType> element_type;
    size_t size;
};

struct C_QType {
    QType const* cpp_qt;
    bool is_const;
    std::variant<C_Type, C_Pointer, C_Array> type;
};

struct C_Param {
    C_QType type;
    std::string name;
};

struct C_Field {
    C_QType type;
    std::string name;
};

struct C_Struct {
    Class const& cls;
    std::string name;
    std::vector<C_Field> fields;
};

struct C_Enum {
    std::string name;
    std::vector<EnumVariant> variants;
    bbl_builtin_t integer_type;
};

struct C_StdFunction {
    C_QType return_type;
    std::vector<C_QType> params;
};

struct Generated {};

using FunctionSource =
    std::variant<Method const*, Constructor const*, Function const*, Generated>;

struct C_Function {
    FunctionSource method_or_function;
    std::string name;
    std::optional<C_Param> result;
    std::optional<C_Param> receiver;
    std::vector<C_Param> params;
    ExprCompound body;
};

using C_StructMap = MapType<std::string, C_Struct>;
using C_FunctionMap = MapType<std::string, C_Function>;
using C_StdFunctionMap = MapType<std::string, C_StdFunction>;
using C_EnumMap = MapType<std::string, C_Enum>;

struct C_Module {
    std::string cpp_id;
    std::string name;
    std::vector<Inclusion> inclusions;
    std::vector<std::string> structs;
    std::vector<std::string> functions;
    std::vector<std::string> stdfunctions;
    std::vector<std::string> enums;
};

class C_API {
    Context const& _cpp_ctx;
    std::vector<C_Module> _modules;
    std::vector<Inclusion> _inclusions;
    C_StructMap _structs;
    C_FunctionMap _functions;
    C_StdFunctionMap _stdfunctions;
    C_EnumMap _enums;

    auto _translate_parameter_list(std::vector<Param> const& params,
                                   std::vector<C_Param>& c_params,
                                   std::vector<ExprPtr>& expr_params) -> void;

    auto _translate_return_type(QType const& cpp_return_type) -> std::optional<C_Param>;

    auto _translate_method(Method const* method,
                           std::string const& function_prefix,
                           std::string const& class_id) -> C_Function;

    auto _translate_function(Function const* function,
                             std::string const& function_prefix) -> C_Function;

    auto _translate_constructor(Constructor const* ctor,
                                std::string const& function_prefix,
                                std::string const& class_id) -> C_Function;

    auto _generate_destructor(std::string const& function_prefix,
                              std::string const& class_id) -> C_Function;

    auto _translate_qtype(QType const& qt) -> C_QType;

    auto _get_c_qtype_as_string(C_QType const& qt, std::string const& name) const -> std::string;
    auto _get_function_declaration(C_Function const& c_fun) const
        -> std::string;

public:
    C_API(Context const& cpp_ctx);

    std::string get_header() const;
    std::string get_source() const;
};

} // namespace bbl