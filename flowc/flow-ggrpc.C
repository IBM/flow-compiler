#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

#include "flow-compiler.H"
#include "stru1.H"
#include "grpc-helpers.H"

using namespace stru1;
int flow_compiler::genc_protobuf() { 
    int error_count = 0;
    DEBUG_ENTER;
    for(auto fdp: fdps) {
        std::string file(fdp->name());
        std::string ccc = protocc + file;
        DEBUG_CHECK(ccc);
        if(system(ccc.c_str()) != 0) {
            pcerr.AddError(file, -1, 0, "failed to gerenate protobuf code");
            ++error_count;
        }
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_grpc() { 
    int error_count = 0;
    DEBUG_ENTER;
    for(auto fdp: fdps) if(fdp->service_count() > 0) {
        std::string file(fdp->name());
        std::string ccc = grpccc + file;
        DEBUG_CHECK(ccc);
        if(system(ccc.c_str()) != 0) {
            pcerr.AddError(file, -1, 0, "failed to gerenate gRPC code");
            ++error_count;
        }
    }
    DEBUG_LEAVE;
    return error_count;
}

void static enum_as_strings(std::ostream &buf, ::google::protobuf::EnumDescriptor const *ep) {
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

void static json_schema_buf(std::ostream &buf, ::google::protobuf::Descriptor const *dp, bool extended_format, int indent, int level, std::string const &prefix, std::map<std::string, std::string> const &defaults) {
    std::string eos;
    if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');

    buf << "\"type\":\"object\",";
    buf << "\"properties\":{";

    for(int f = 0, fc = dp->field_count(); f != fc; ++f) {
        FieldDescriptor const *fd = dp->field(f);
        std::string ftitle = decamelize(fd->name());
        std::string fdescription = get_description(fd);
        bool is_repeated = fd->is_repeated();

        if(f > 0) buf << ",";
        buf << eos; 
        buf << "\"" << fd->json_name() << "\":{";

        ++level;
        if(indent > 0) eos = std::string("\n") + std::string(indent*level, ' ');

        buf << eos << "\"title\":" << c_escape(ftitle) << "," << eos;
        if(!fdescription.empty()) buf << "\"description\":" << c_escape(fdescription) << "," << eos;
        if(extended_format) {
            buf << "\"propertyOrder\":" << f << "," << eos;
            // find the default value
            auto dfp = defaults.find(prefix.empty()? std::string(fd->name()): prefix + "." + fd->name());
            if(dfp != defaults.end()) 
                buf << "\"default\":" << c_escape(dfp->second) << "," << eos;
        }

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
                json_schema_buf(buf, fd->message_type(), extended_format, indent, level+1, prefix.empty()? std::string(fd->name()): prefix + "." + fd->name(), defaults);
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

std::string json_schema(std::map<std::string, std::string> const &defaults, ::google::protobuf::Descriptor const *dp, std::string const &title, std::string const &description, bool extended_format, bool pretty) {
    std::ostringstream buf;
    buf << "{";
    if(pretty) buf << "\n";
    if(!title.empty()) buf << "\"title\":" << c_escape(title) << ",";
    std::string descr = description.empty()? get_description(dp): description;
    if(!descr.empty()) 
        buf << "\"description\":" << c_escape(descr) << ",";

    json_schema_buf(buf, dp, extended_format, pretty? 4: 0, 1, "", defaults);
    if(pretty) buf << "\n";
    buf << "}";
    return buf.str();
}
