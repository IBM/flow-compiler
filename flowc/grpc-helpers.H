#ifndef H_GRPC_HELPERS_H
#define H_GRPC_HELPERS_H

#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/importer.h>

#include "flow-ast.H"
#include "stru1.H"
#include "ansi-escapes.H"

int grpc_type_to_ftk(google::protobuf::FieldDescriptor::Type t);

class FErrorPrinter: public google::protobuf::compiler::MultiFileErrorCollector {
    google::protobuf::compiler::DiskSourceTree *source_tree;
    void ShowLine(std::string const filename, int line, int column);
public:    
    std::ostream *outs;
    FErrorPrinter(std::ostream &outsr, google::protobuf::compiler::DiskSourceTree *st): outs(&outsr), source_tree(st) {
    }
    void AddMessage(std::string const &type, ANSI_ESCAPE color, std::string const &filename, int line, int column, std::string const &message);
    virtual void AddError(std::string const &filename, int line, int column, std::string const &message);
    virtual void AddWarning(std::string const &filename, int line, int column, std::string const &message);
    virtual void AddNote(std::string const &filename, int line, int column, std::string const &message);
    void AddError(std::string const &filename, flow_token const &token, std::string const &message) { 
        AddError(filename, token.line, token.column, message);
    }
    void AddError(std::string const &filename, flow_ast_node const &pos, std::string const &message) { 
        AddError(filename, pos.token, message);
    }
    void AddWarning(std::string const &filename, flow_token const &token, std::string const &message) { 
        AddWarning(filename, token.line, token.column, message);
    }
    void AddWarning(std::string const &filename, flow_ast_node const &pos, std::string const &message) { 
        AddWarning(filename, pos.token, message);
    }
    void AddNote(std::string const &filename, flow_token const &token, std::string const &message) { 
        AddNote(filename, token.line, token.column, message);
    }
    void AddNote(std::string const &filename, flow_ast_node const &pos, std::string const &message) { 
        AddNote(filename, pos.token, message);
    }
};
class ErrorPrinter: public google::protobuf::io::ErrorCollector {
public:    
    FErrorPrinter &fperr;
    std::string filename;
    ErrorPrinter(FErrorPrinter &mfec, std::string const &file): fperr(mfec), filename(file) {
    }
    virtual void AddError(int line, int column, std::string const & message);
};
inline static 
std::string get_name(::google::protobuf::Descriptor const *dp) {
    return dp->name();
}
inline static 
std::string get_name(::google::protobuf::ServiceDescriptor const *dp) {
    return dp->name();
}
inline static 
std::string get_name(::google::protobuf::MethodDescriptor const *dp) {
    return dp->name();
}
inline static 
std::string get_name(::google::protobuf::EnumValueDescriptor const *dp) {
    return dp->name();
}
inline static 
std::string get_name(::google::protobuf::EnumDescriptor const *dp) {
    return dp->name();
}
inline static 
std::string get_full_name(::google::protobuf::EnumDescriptor const *dp) {
    std::string pkg = dp->file()->package();
    if(pkg.empty()) 
        return std::string("::") + stru1::to_underscore(dp->full_name());
    std::string full_name;
    stru1::split(nullptr, &full_name, dp->full_name(), ".");
    return std::string("::") + pkg + "::" +  stru1::to_underscore(full_name);
}
inline static 
std::string get_full_name_1(::google::protobuf::EnumDescriptor const *dp) {
    return dp->full_name();
}
inline static 
std::string get_full_type_name(::google::protobuf::EnumValueDescriptor const *dp) {
    return get_full_name(dp->type());
}
inline static 
std::string get_full_name(::google::protobuf::EnumValueDescriptor const *dp) {
    std::string pkg = dp->file()->package();
    if(dp->type()->containing_type()) {
        return get_full_name(dp->type())+"_"+dp->name();
    } else if(!pkg.empty()) {
        return std::string("::") + pkg + "::" + dp->name();
    } else {
        return dp->name();
    }
}
inline static 
std::string get_full_name(::google::protobuf::Descriptor const *dp) {
    std::string pkg = dp->file()->package();
    if(!pkg.empty()) pkg += "::";
    return std::string("::") + pkg + dp->name();
}
inline static 
std::string get_full_name(::google::protobuf::MethodDescriptor const *dp) {
    std::string pkg = dp->file()->package();
    if(!pkg.empty()) pkg += "::";
    return std::string("::") + pkg + dp->name();
}
inline static 
std::string get_full_name(::google::protobuf::ServiceDescriptor const *dp) {
    std::string pkg = dp->file()->package();
    if(!pkg.empty()) pkg += "::";
    return std::string("::") + pkg + dp->name();
}
std::string get_full_type_name(::google::protobuf::FieldDescriptor const *dp);
int get_grpc_type_size(::google::protobuf::FieldDescriptor const *dp);
std::string get_grpc_type_name(::google::protobuf::FieldDescriptor const *dp);
char const *grpc_type_name(google::protobuf::FieldDescriptor::Type t);
char const *grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type t);
/**
 * Return all field names and their dimension
 */
std::vector<std::pair<std::string, int>> get_fields(::google::protobuf::Descriptor const *dp, int dim = 0, std::string const &j=".");
/**
 * Return all field names accesible from this descriptor
 */
std::vector<std::string> get_field_names(::google::protobuf::Descriptor const *dp, std::string const &j="-", bool json=false);
/**
 * Return all field name and their corresponding field descriptors
 */
std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> get_accessors(::google::protobuf::Descriptor const *dp);
/*
 * Return the package name for a given .proto file
 */
std::string get_package(::google::protobuf::FileDescriptor const *fdp);
/**
 * Return the maximum dimension for the leaf fields of this descriptor
 */
int get_index_depth(::google::protobuf::Descriptor const *dp);
inline static
bool is_message(::google::protobuf::FieldDescriptor const *fd) {
    return fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE;
}
inline static
bool is_enum(::google::protobuf::FieldDescriptor const *fd) {
    return fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_ENUM;
}

std::string json_schema_p(google::protobuf::Descriptor const *dp, std::string const &title, std::string const &description, std::function<std::tuple<std::string, int, std::string> (std::string const &field, std::string const &prop)> get_prop, int indent=0); 
inline static std::string json_schema(google::protobuf::Descriptor const *dp, std::string const &title, std::string const &description) {
    std::function<std::tuple<std::string, int, std::string> (std::string const &field, std::string const &prop)> egpf;
    return json_schema_p(dp, title, description, egpf, 0);
}
std::string gen_proto(::google::protobuf::MethodDescriptor const *mdp);
std::string gen_proto(std::set<::google::protobuf::MethodDescriptor const *> const &mdps);
std::string grpc_error_code(int c);
std::string grpc_error_code(std::string const &id);
#endif

