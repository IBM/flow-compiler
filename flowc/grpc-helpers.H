#ifndef H_GRPC_HELPERS_H
#define H_GRPC_HELPERS_H

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/importer.h>

#include "flow-ast.H"
#include "stru1.H"

static inline
int grpc_type_to_ftk(google::protobuf::FieldDescriptor::Type t) {
    switch(t) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return FTK_FLOAT;

        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return FTK_dtid;

        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            return FTK_INTEGER;

        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
        case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
            return FTK_STRING;

        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
            return FTK_fldm;

        case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
        default:
            return 0;
    }
}
class FErrorPrinter: public google::protobuf::compiler::MultiFileErrorCollector {
public:    
    std::ostream *outs;
    FErrorPrinter(std::ostream &outsr): outs(&outsr) {
    }
    void AddMessage(std::string const &type, std::string const &color, std::string const &filename, int line, int column, std::string const &message);
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
class GeneratorOD: public google::protobuf::compiler::GeneratorContext {
public:
    virtual ::google::protobuf::io::ZeroCopyOutputStream *Open(std::string const & filename);
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
inline static 
std::string get_full_type_name(::google::protobuf::FieldDescriptor const *dp) {
    switch(dp->type()) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return "float";

        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return get_full_name(dp->enum_type());

        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            return "integer";

        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
        case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
            return "string";

        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
            return get_full_name(dp->message_type());

        case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
        default:
            return "";
    }
}
inline static void 
r_get_field_names(::google::protobuf::Descriptor const *dp, std::string const &j, std::string const &prefix, std::vector<std::string> &buf, int start_level, int current_level) {
    for(int f = 0, ef = dp->field_count(); f < ef; ++f) {
        auto fd = dp->field(f);
        std::string name = prefix;
        if(current_level >= start_level) {
            if(name.length() > 0) name += j;
            name += fd->name();
        }
        if(fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) 
            r_get_field_names(fd->message_type(), j, name, buf, start_level, current_level+1);
        else 
            buf.push_back(name);
    }
}
inline static 
std::vector<std::string> get_field_names(::google::protobuf::Descriptor const *dp, std::string const &j="-", int start_level=0) {
    std::vector<std::string> buf;
    r_get_field_names(dp, j, "", buf, start_level, 0);
    return buf;
}
inline static 
void r_get_accessors(::google::protobuf::Descriptor const *dp, std::string const &prefix, std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> &buf) {
    for(int f = 0, ef = dp->field_count(); f < ef; ++f) {
        auto fd = dp->field(f);
        std::string name = prefix;
        if(name.length() > 0) name += ".";
        if(fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) 
            r_get_accessors(fd->message_type(), name+stru1::to_lower(fd->name()), buf);
        else if(fd->is_repeated()) 
            buf.push_back(std::make_pair(name+"add_"+stru1::to_lower(fd->name()), fd));
        else 
            buf.push_back(std::make_pair(name+"set_"+stru1::to_lower(fd->name()), fd));
    }
}
inline static 
std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> get_accessors(::google::protobuf::Descriptor const *dp) {
    std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> buf;
    r_get_accessors(dp, "", buf);
    return buf;
}
/*
 * Return the package name for a given .proto package
 */
inline static 
std::string get_package(::google::protobuf::FileDescriptor const *fdp) {
    std::string package = fdp->package();
    if(package.empty()) {
        package = fdp->name();
        if(package.length() > strlen(".proto") && package.substr(package.length()-strlen(".proto")) == ".proto")
            package = package.substr(0, package.length()-strlen(".proto"));
    }
    return package;
}
inline static
int get_index_depth(::google::protobuf::Descriptor const *dp) {
    int depth = 0;
    for(int i = 0, fc = dp->field_count(); i < fc; ++i) {
        auto fp = dp->field(i);
        int ldepth = fp->is_repeated()? 1: 0;
        auto fdp = fp->message_type();
        if(fdp != nullptr) ldepth += get_index_depth(fdp);
        depth = std::max(depth, ldepth);
    }
    return depth;
}
inline static
bool is_message(::google::protobuf::FieldDescriptor const *fd) {
    return fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE;
}
inline static
bool is_enum(::google::protobuf::FieldDescriptor const *fd) {
    return fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_ENUM;
}
#endif

