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

int flow_compiler::genc_protobuf() {
    int error_count = 0;
    for(auto fdp: fdps) {
        std::string file(fdp->name());
        GeneratorOD context;
        compiler::cpp::CppGenerator gencc;
        std::string error;
        if(!gencc.Generate(fdp, "", &context, &error)) {
            pcerr.AddError(file, -1, 0, error);
            ++error_count;
        }
    }
    return error_count;
}
int flow_compiler::genc_grpc() { 
    int error_count = 0;
    for(auto fdp: fdps) if(fdp->service_count() > 0) {
        std::string file(fdp->name());
        std::string protocc = grpccc + file;
        if(system(protocc.c_str()) != 0) {
            pcerr.AddError(file, -1, 0, "failed to gerenate gRPC code");
            ++error_count;
        }
    }
    return error_count;
}

void static enum_as_strings(std::ostream &buf, ::google::protobuf::EnumDescriptor const *ep) {
    for(int v = 0, vc = ep->value_count(); v != vc; ++v) {
        auto vp = ep->value(v);
        if(v > 0) buf << ",";
        buf << "\"" << vp->name() << "\"";
    }
}

void static json_schema_buf(std::ostream &buf, ::google::protobuf::Descriptor const *dp) {
    buf << "\"type\":\"object\",";

    buf << "\"properties\":{";
    for(int f = 0, fc = dp->field_count(); f != fc; ++f) {
        FieldDescriptor const *fd = dp->field(f);
        std::string ftitle = decamelize(fd->name());
        std::string fdescription;
        bool is_repeated = fd->is_repeated();

        ::google::protobuf::SourceLocation sl;
        if(fd->GetSourceLocation(&sl)) {
            // ignore detatched comments
            fdescription = strip(sl.leading_comments, "\t\r\a\b\v\f\n");
            if(!sl.leading_comments.empty() && !sl.trailing_comments.empty())
                fdescription += "\n";
            fdescription += strip(sl.trailing_comments, "\t\r\a\b\v\f\n");
        }

        if(f > 0) buf << ",";
        buf << "\"" << fd->json_name() << "\":{";

        if(is_repeated) {
            buf << "\"type\":\"array\",\"title\":" << c_escape(ftitle) << ",";
            if(!fdescription.empty()) buf << "\"description\":" << c_escape(fdescription) << ",";
            buf << "\"items\":{";
            ftitle.clear(); fdescription.clear();
        }
        if(!ftitle.empty()) buf << "\"title\":" << c_escape(ftitle) << ",";
        if(!fdescription.empty()) buf << "\"description\":" << c_escape(fdescription) << ",";

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
                json_schema_buf(buf, fd->message_type());
                break;
            default:
                buf << "null";
                break;
        }
        if(is_repeated) buf << "}";
        buf << "}";
    }
    buf << "}";
}

std::string json_schema(::google::protobuf::Descriptor const *dp, std::string const &title, std::string const &description) {
    std::ostringstream buf;
    buf << "{";
    if(!title.empty()) buf << "\"title\":" << c_escape(title) << ",";
    if(!description.empty()) buf << "\"description\":" << c_escape(description) << ",";
    json_schema_buf(buf, dp);
    buf << "}";
    return buf.str();
}
