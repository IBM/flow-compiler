#include <sstream>
#include <grpc++/grpc++.h>
#include "grpc-helpers.H"
#include "stru1.H"
#include "cot.H"

using namespace stru1;
using namespace google::protobuf;

static void gen_proto(std::ostream &pbuf, ::google::protobuf::Descriptor const *dp, std::set<std::string> &visited) {
    if(visited.find(get_name(dp)+"/d") != visited.end())
        return;
    visited.insert(get_name(dp)+"/d");
    for(int f = 0, fc = dp->field_count(); f != fc; ++f) {
        FieldDescriptor const *fd = dp->field(f);
        switch(fd->type()) {
            case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                gen_proto(pbuf, fd->message_type(), visited);
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                if(visited.find(get_name(fd->enum_type())+"/e") == visited.end()) {
                    pbuf << fd->enum_type()->DebugString() << "\n";
                    visited.insert(get_name(fd->enum_type())+"/e");
                }
                break; 
            default:
                break;
        }
    }
    pbuf << dp->DebugString() << "\n";
}

std::string gen_proto(::google::protobuf::MethodDescriptor const *mdp) {
    std::set<::google::protobuf::MethodDescriptor const *> mdps; 
    mdps.insert(mdp);
    return gen_proto(mdps);
}
std::string gen_proto(std::set<::google::protobuf::MethodDescriptor const *> const &mdps) {
    std::ostringstream pbuf;
    pbuf << "syntax = \"proto3\";\n\n";
    std::set<std::string> services;
    std::set<std::string> visited;
    for(auto mdp: mdps) {
        gen_proto(pbuf, mdp->input_type(), visited);
        gen_proto(pbuf, mdp->output_type(), visited);
    }
    for(auto mdp: mdps) {
        std::string service_name = get_name(mdp->service());
        if(cot::contains(visited, service_name))
            continue;
        visited.insert(service_name);
        pbuf << "service " << service_name << " {\n";
        for(auto p2: mdps) 
            if(get_name(p2->service()) == service_name) 
                pbuf << "  " << p2->DebugString();
        pbuf << "}\n";
    }
    return pbuf.str();
}
static void enum_as_strings(std::ostream &buf, ::google::protobuf::EnumDescriptor const *ep) {
    for(int v = 0, vc = ep->value_count(); v != vc; ++v) {
        auto vp = ep->value(v);
        if(v > 0) buf << ",";
        buf << "\"" << vp->name() << "\"";
    }
}
static std::string get_description(FieldDescriptor const *fd) {
    std::string fdescription;
    ::google::protobuf::SourceLocation sl;
    if(fd->GetSourceLocation(&sl)) {
        // ignore detatched comments
        fdescription = strip(sl.leading_comments, "\t\r\a\b\v\f\n");
        if(!sl.leading_comments.empty() && !sl.trailing_comments.empty())
            fdescription += "\n";
        fdescription += strip(sl.trailing_comments, "\t\r\a\b\v\f\n");
    }
    return fdescription;
}
static std::string get_description(::google::protobuf::Descriptor const *fd) {
    std::string fdescription;
    ::google::protobuf::SourceLocation sl;
    if(fd->GetSourceLocation(&sl)) {
        // ignore detatched comments
        fdescription = strip(sl.leading_comments, "\t\r\a\b\v\f\n");
        if(!sl.leading_comments.empty() && !sl.trailing_comments.empty())
            fdescription += "\n";
        fdescription += strip(sl.trailing_comments, "\t\r\a\b\v\f\n");
    }
    return fdescription;
}
static void json_schema_buf(std::ostream &buf, ::google::protobuf::Descriptor const *dp, std::string const &prefix, std::function<std::string (std::string const &field, std::string const &prop)> get_prop, int indent, int level=1) {
    std::string eos;
    if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');

    buf << "\"type\":\"object\",";
    buf << "\"properties\":{";

    for(int f = 0, fc = dp->field_count(); f != fc; ++f) {
        FieldDescriptor const *fd = dp->field(f);
        std::string ffname = prefix.empty()? std::string(fd->name()): prefix + "." + fd->name();
        std::string ftitle, fdescription;
        if(fdescription.empty()) fdescription = get_description(fd);
        bool is_repeated = fd->is_repeated();

        if(f > 0) buf << ",";
        buf << eos; 
        buf << "\"" << fd->json_name() << "\":{";

        ++level;
        if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');
        buf << eos;
        std::string fpv;
        if(get_prop) {
            fdescription = get_prop(ffname, "description");
            ftitle = get_prop(ffname, "label");
            fpv = get_prop(ffname, "order");
            if(!fpv.empty()) {
                buf << "\"propertyOrder\":" << fpv << "," << eos;
            } else {
                buf << "\"propertyOrder\":" << f << "," << eos;
            }
            fpv = get_prop(ffname, "format");
            if(!fpv.empty()) 
                buf << "\"format\":" << c_escape(fpv) << "," << eos;
        }
        if(ftitle.empty()) ftitle = decamelize(fd->name());
        buf << "\"title\":" << c_escape(ftitle) << "," << eos;
        if(fdescription.empty()) fdescription = get_description(fd);
        if(!fdescription.empty()) buf << "\"description\":" << c_escape(fdescription) << "," << eos;

        if(is_repeated) {
            buf << "\"type\":\"array\"," << "\"items\":{";
            ++level;
            if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');
            buf << eos;
        }
        switch(fd->type()) {
            case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                buf << "\"type\":\"number\"";
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
            case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
            case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
            case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
            case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
            case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                buf << "\"type\":\"integer\"";
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
                buf << "\"type\":\"string\"";
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
                buf << "\"type\":\"boolean\"";
                break;
            case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
                buf << "\"type\":\"string\",\"enum\":[";
                enum_as_strings(buf, fd->enum_type());
                buf << "]";
                break; 
            case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
                json_schema_buf(buf, fd->message_type(), ffname, get_prop, indent, level+1);
                break;
            default:
                buf << "null";
                break;
        }
        --level;
        if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');
        if(is_repeated) {
            buf << eos << "}"; 
            --level;
            if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');
        }
        buf << eos;
        buf << "}";
    }
    --level;
    if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');
    buf << eos << "}";
}
std::string json_schema_p(google::protobuf::Descriptor const *dp, std::string const &title, std::string const &description, std::function<std::string (std::string const &field, std::string const &prop)> get_prop, int indent) {
    std::ostringstream buf;
    buf << "{";
    if(!title.empty()) buf << "\"title\":" << c_escape(title) << ",";
    std::string descr = description.empty()? get_description(dp): description;
    if(!descr.empty()) 
        buf << "\"description\":" << c_escape(descr) << ",";
    json_schema_buf(buf, dp, "", get_prop, indent);
    buf << "}";
    return buf.str();
}
int grpc_type_to_ftk(google::protobuf::FieldDescriptor::Type t) {
    switch(t) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return FTK_FLOAT;

        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return FTK_enum;

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
            return 0;
    }
    return 0;
}
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
int get_grpc_type_size(::google::protobuf::FieldDescriptor const *dp) {
    switch(dp->type()) {
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            return 8;
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return 4;

        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            return 1;

        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
        case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
        case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
            return 100;
    }
    return 0;
}
std::string get_grpc_type_name(::google::protobuf::FieldDescriptor const *dp) {
    switch(dp->type()) {
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return get_full_name(dp->enum_type());
        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
            return get_full_name(dp->message_type());
        default:
            break;
    }
    return grpc_type_name(dp->type());
}
char const *grpc_type_name(google::protobuf::FieldDescriptor::Type t) {
    switch(t) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            return "double";
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return "float";
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return "enum";
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
            return "int64";
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
            return "uint64";
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
            return "int32";
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
            return "fixed64";
        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
            return "fixed32";
        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            return "bool";
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            return "uint32";
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
            return "sfixed32";
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
            return "sfixed64";
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
            return "sint32";
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
            return "sint64";

        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            return "string";
        case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
            return "bytes";
        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
            return "message";
        case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
            return "group";
    }
    return "";
}
char const *grpc_type_to_cc_type(google::protobuf::FieldDescriptor::Type t) {
    switch(t) {
        case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
            return "double";
        case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
            return "float";

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
            return "uint64_t";

        case google::protobuf::FieldDescriptor::Type::TYPE_SINT32:
        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
            return "int32_t";

        case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            return "uint32_t";

        case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::Type::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
            return "int64_t";

        case google::protobuf::FieldDescriptor::Type::TYPE_BOOL:
            return "bool";

        case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
            return "std::string";
        case google::protobuf::FieldDescriptor::Type::TYPE_BYTES:
            return "std::string";

        case google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE:
        case google::protobuf::FieldDescriptor::Type::TYPE_GROUP:
        case google::protobuf::FieldDescriptor::Type::TYPE_ENUM:
            return "??";
    }
    return "???";
}
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
std::string get_package(::google::protobuf::FileDescriptor const *fdp) {
    std::string package = fdp->package();
    if(package.empty()) {
        package = fdp->name();
        if(package.length() > strlen(".proto") && package.substr(package.length()-strlen(".proto")) == ".proto")
            package = package.substr(0, package.length()-strlen(".proto"));
    }
    return package;
}
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
std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> get_accessors(::google::protobuf::Descriptor const *dp) {
    std::vector<std::pair<std::string, ::google::protobuf::FieldDescriptor const *>> buf;
    r_get_accessors(dp, "", buf);
    return buf;
}
static void r_get_field_names(::google::protobuf::Descriptor const *dp, std::string const &j, std::string const &prefix, std::vector<std::pair<std::string, int>> &buf, int dim, bool json) {
    for(int f = 0, ef = dp->field_count(); f < ef; ++f) {
        auto fd = dp->field(f);
        std::string name = prefix;
        if(name.length() > 0) name += j;
        name += json? fd->json_name(): fd->name();

        if(fd->type() == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) 
            r_get_field_names(fd->message_type(), j, name, buf, dim + (fd->is_repeated()? 1: 0), json);
        else 
            buf.push_back(std::make_pair(name, dim + (fd->is_repeated()? 1: 0)));
    }
}
std::vector<std::string> get_field_names(::google::protobuf::Descriptor const *dp, std::string const &j, bool json) {
    std::vector<std::pair<std::string, int>> buf;
    r_get_field_names(dp, j, "", buf, 0, json);
    std::vector<std::string> rvl;
    for(auto p: buf) rvl.push_back(p.first);
    return rvl;
}
std::vector<std::pair<std::string, int>> get_fields(::google::protobuf::Descriptor const *dp, int dim, std::string const &j) {
    std::vector<std::pair<std::string, int>> buf;
    r_get_field_names(dp, j, "", buf, dim, false);
    return buf;
}
static std::map<std::string, int> gec = {
{"OK",  grpc::StatusCode::OK},	
{"CANCELLED",  grpc::StatusCode::CANCELLED},
{"UNKNOWN",  grpc::StatusCode::UNKNOWN},
{"INVALID_ARGUMENT",  grpc::StatusCode::INVALID_ARGUMENT},
{"DEADLINE_EXCEEDED",  grpc::StatusCode::DEADLINE_EXCEEDED},
{"NOT_FOUND",  grpc::StatusCode::NOT_FOUND},
{"ALREADY_EXISTS",  grpc::StatusCode::ALREADY_EXISTS},
{"PERMISSION_DENIED",  grpc::StatusCode::PERMISSION_DENIED},
{"RESOURCE_EXHAUSTED",  grpc::StatusCode::RESOURCE_EXHAUSTED},
{"FAILED_PRECONDITION",  grpc::StatusCode::FAILED_PRECONDITION},
{"ABORTED",  grpc::StatusCode::ABORTED},
{"OUT_OF_RANGE",  grpc::StatusCode::OUT_OF_RANGE},
{"UNIMPLEMENTED",  grpc::StatusCode::UNIMPLEMENTED},
{"INTERNAL",  grpc::StatusCode::INTERNAL},
{"UNAVAILABLE",  grpc::StatusCode::UNAVAILABLE},
{"DATA_LOSS",  grpc::StatusCode::DATA_LOSS},
{"UNAUTHENTICATED",  grpc::StatusCode::UNAUTHENTICATED}
};
std::string grpc_error_code(int i) {
    for(auto c: gec) if(c.second == i) return c.first;
    return "";
}
std::string grpc_error_code(std::string const &id) {
    std::string idp = stru1::to_upper(id);
    std::set<int> found;
    for(auto f: gec) if(stru1::starts_with(f.first, idp))
        found.insert(f.second);
    if(found.size() != 1)
        return "";
    return grpc_error_code(*found.begin());
}
