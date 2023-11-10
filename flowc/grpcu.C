#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <vector>

#include <google/protobuf/compiler/importer.h>
#include "grpcu.H"
#include "stru.H"
#include "filu.H"
#include "ansi-escapes.H"
#include "flow-comp.H"

using namespace google::protobuf;

namespace {
class FErrorPrinter: public google::protobuf::compiler::MultiFileErrorCollector {
    google::protobuf::compiler::DiskSourceTree *source_tree;
    void ShowLine(std::string const filename, int line, int column);
public:    
    std::ostream *outs;
    FErrorPrinter(std::ostream &outsr, google::protobuf::compiler::DiskSourceTree *st): outs(&outsr), source_tree(st) {
    }
    void AddMessage(std::string const &type, ANSI_ESCAPE color, std::string const &filename, int line, int column, std::string const &message);
    virtual void AddError(std::string const &filename, int line, int column, std::string const &message);
};
void FErrorPrinter::ShowLine(std::string const filename, int line, int column) {
    std::string disk_file;
    if(source_tree == nullptr) 
        disk_file = filename;
    else
        source_tree->VirtualFileToDiskFile(filename, &disk_file);

    std::ifstream sf(disk_file.c_str());
    if(sf.is_open()) {
        std::string lines;
        for(int i = 0; i <= line; ++i) std::getline(sf, lines);
        *outs << lines << "\n";
        if(column > 1) *outs << std::string(column, ' ');
        *outs << "^" << "\n";
    }
}
void FErrorPrinter::AddMessage(std::string const &type, ANSI_ESCAPE color, std::string const &filename, int line, int column, std::string const &message) {
    *outs << ANSI_BOLD;
    *outs << filename;
    if(line >= 0) *outs << "(" << line+1;
    if(line >= 0 && column >= 0) *outs << ":" << column+1;
    if(line >= 0) *outs << ")";
    *outs << ": " << color << type << ": ";
    *outs << ANSI_RESET;
    *outs << ansi::emphasize(message, ANSI_BOLD+ANSI_GREEN);
    *outs << "\n";
}
void FErrorPrinter::AddError(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("error", ANSI_RED, filename, line, column, message);
    if(!filename.empty() && line >= 0 && column >= 0)
        ShowLine(filename, line, column);
}
compiler::DiskSourceTree source_tree;
FErrorPrinter fep(std::cerr, &source_tree);
compiler::Importer importer(&source_tree, &fep);

FieldDescriptor const *find_field_descriptor(Descriptor const *md, std::string field_name) {
    std::string ffn, tail;
    stru::split(&ffn, &tail, field_name, ".");
    if(md != nullptr && ffn != "") {
        auto fd = md->FindFieldByName(ffn);
        if(fd == nullptr) fd = md->FindFieldByLowercaseName(ffn);
        if(fd == nullptr) fd = md->FindFieldByCamelcaseName(ffn);
        if(fd != nullptr && tail != "")
            return find_field_descriptor(fd->message_type(), tail);
        return fd;
    }
    return nullptr;
}
fc::value_type d_to_value_type(Descriptor const *md);
fc::value_type fd_to_value_type(FieldDescriptor const *fd) {
    fc::value_type ft;
    if(fd != nullptr) {
        switch(fd->cpp_type()) {
            case FieldDescriptor::CppType::CPPTYPE_INT32:
            case FieldDescriptor::CppType::CPPTYPE_INT64:
            case FieldDescriptor::CppType::CPPTYPE_UINT32:
            case FieldDescriptor::CppType::CPPTYPE_UINT64:
            case FieldDescriptor::CppType::CPPTYPE_BOOL:
                ft = fc::value_type(fc::fvt_int);
                break;
            case FieldDescriptor::CppType::CPPTYPE_STRING:
                ft = fc::value_type(fc::fvt_str);
                break;
            case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
            case FieldDescriptor::CppType::CPPTYPE_FLOAT:
                ft = fc::value_type(fc::fvt_flt);
                break;
            case FieldDescriptor::CppType::CPPTYPE_ENUM:
                ft = fc::value_type(fc::fvt_enum, fd->enum_type()->full_name());
                break;
            case FieldDescriptor::CppType::CPPTYPE_MESSAGE:
                ft = d_to_value_type(fd->message_type());
                break;
        }
        if(fd->is_repeated())
            ft = fc::value_type(fc::fvt_array, {ft});
        ft.fname = fd->name();
    }
    return ft;
}
fc::value_type d_to_value_type(Descriptor const *md) {
    if(md == nullptr)
        return fc::value_type();
    fc::value_type vt(fc::fvt_struct, md->full_name());
    for(int i = 0, c = md->field_count(); i < c; ++i) { 
        vt.add_type(fd_to_value_type(md->field(i)));
    }
    return vt;
}

}

namespace grpcu {
/**
 * Add the directory to the proto path
 * Return 0 on success or 1 if can't access directory
 */
int store::add_to_proto_path(std::string directory, std::string mapped_to) {
    if(!filu::is_dir(directory))
        return 1;
    source_tree.MapPath(mapped_to, directory);
    grpccc += "-I"; grpccc += directory; grpccc += " ";
    protocc += "-I"; protocc += directory; protocc += " ";
    return 0;
}
int store::import_file(std::string file, bool add_to_path) {
    auto fdp = importer.Import(file);
    if(fdp == nullptr && add_to_path != 0) {
        add_to_proto_path(filu::dirname(file));
        fdp = importer.Import(filu::basename(file));
    }
    if(fdp == nullptr) 
        return 1;
#if GOOGLE_PROTOBUF_VERSION < 4000000
    if(fdp->syntax() != FileDescriptor::Syntax::SYNTAX_PROTO3) {
        fep.AddError(file, -1, 0, "syntax must be proto3");
        return 1;
    }
#endif
    std::set<void const *> included(file_descriptors.begin(), file_descriptors.end());
    if(included.find((void const *) fdp) != included.end())
        return 0;
    included.insert((void const *)fdp);
    file_descriptors.push_back((void const *)fdp);
    for(int i = 0, e = fdp->dependency_count(); i != e; ++i) {
        auto fdq = fdp->dependency(i);
        if(included.find((void const *)fdq) == included.end()) {
            file_descriptors.push_back((void const *)fdq);
            included.insert((void const *)fdq);
        }
    }
    return 0;
}

static std::map<std::string, int> grpc_error_codes = {
{"OK", 0},
{"CANCELLED", 1},
{"UNKNOWN", 2},
{"INVALID_ARGUMENT", 3},
{"DEADLINE_EXCEEDED", 4},
{"NOT_FOUND", 5},
{"ALREADY_EXISTS", 6},
{"PERMISSION_DENIED", 7},
{"RESOURCE_EXHAUSTED", 8},
{"FAILED_PRECONDITION", 9},
{"ABORTED", 10},
{"OUT_OF_RANGE", 11},
{"UNIMPLEMENTED", 12},
{"INTERNAL", 13},
{"UNAVAILABLE", 14},
{"DATA_LOSS", 15},
{"UNAUTHENTICATED", 16},
};

int store::error_code(std::string id) {
    auto f = grpc_error_codes.find(id);
    return f ==  grpc_error_codes.end()? -1: f->second;
}
int store::lookup(std::string &match, std::vector<std::string> did, std::set<std::string> *allmp, bool match_methods, bool match_messages, bool match_enums) const {
    std::set<std::string> allm;
    if(allmp == nullptr) allmp = &allm;
    std::string dids = stru::join(did, ".");

    if(match_methods) for(auto vp: find_methods(dids)) 
        allmp->insert(((MethodDescriptor const *) vp)->full_name());

    if(match_messages) for(auto vp: find_messages(dids)) 
        allmp->insert(((Descriptor const *) vp)->full_name());

    if(match_enums) for(auto vp: find_enum_values(dids)) 
        allmp->insert(((EnumValueDescriptor const *) vp)->full_name());

    if(allmp->size() == 1)
        match = *allmp->begin();
    else 
        match.clear();
    return int(allmp->size());
}
std::string store::field_full_name(std::string message_name, std::string field_name) const {
    auto md = importer.pool()->FindMessageTypeByName(message_name);
    auto fd = find_field_descriptor(md, field_name);
    return fd == nullptr? std::string(): fd->full_name();
}
std::set<void const *> store::find_messages(std::string name) const {
    std::set<void const *> ptrs;
    auto p = importer.pool()->FindMessageTypeByName(name);
    if(p != nullptr) 
        ptrs.insert((void *)p);
    if(ptrs.size() == 0) for(auto p: file_descriptors) {
        auto fd = (FileDescriptor const *)p;
        for(int i = 0, e = fd->message_type_count(); i < e; ++i) {
            if(fd->message_type(i)->name() == name || 
               fd->message_type(i)->full_name() == name) 
                ptrs.insert((void const *) fd->message_type(i));
        }
    }
    if(ptrs.size() == 0) {
        ptrs.insert((void *)p);
    }
    return ptrs;
}
std::set<void const *> store::find_methods(std::string name) const {
    std::set<void const *> ptrs;
    auto p = importer.pool()->FindMethodByName(name);
    if(p != nullptr) 
        ptrs.insert((void *)p);
    if(ptrs.size() == 0) for(auto p: file_descriptors) {
        auto fd = (FileDescriptor const *)p;
        for(int i = 0, e = fd->service_count(); i < e; ++i) 
            for(int j = 0, f = fd->service(i)->method_count(); j < f; ++j) 
                if(fd->service(i)->method(j)->name() == name || 
                   fd->service(i)->method(j)->full_name() == name || 
                   fd->service(i)->name() + "." +  fd->service(i)->method(j)->name() == name) 
                    ptrs.insert((void const *) fd->service(i)->method(j));
    }
    return ptrs;
}
std::set<void const *> store::find_enum_values(std::string name) const {
    std::set<void const *> ptrs;
    auto p = importer.pool()->FindEnumValueByName(name);
    if(p != nullptr) 
        ptrs.insert((void *)p);
    if(ptrs.size() == 0) for(auto p: file_descriptors) {
        auto fd = (FileDescriptor const *)p;
        for(int i = 0, e = fd->enum_type_count(); i < e; ++i) 
            for(int j = 0, f = fd->enum_type(i)->value_count(); j < f; ++j) 
                if(fd->enum_type(i)->value(j)->name() == name ||
                   fd->enum_type(i)->value(j)->full_name() == name) 
                    ptrs.insert((void const *) fd->enum_type(i)->value(j));
    }
    return ptrs;
}
std::string store::method_output_full_name(std::string name) const {
    auto found = find_methods(name);
    if(found.size() != 1) return "";
    return ((MethodDescriptor const *) *found.begin())->output_type()->full_name();
}
std::string store::method_input_full_name(std::string name) const {
    auto found = find_methods(name);
    if(found.size() != 1) return "";
    return ((MethodDescriptor const *) *found.begin())->input_type()->full_name();
}
std::string store::method_full_name(std::string name) const {
    auto found = find_methods(name);
    if(found.size() != 1) return "";
    return ((MethodDescriptor const *) *found.begin())->full_name();
}
std::vector<std::string> store::all_method_full_names(std::string name) const {
    std::vector<std::string> names(3);
    auto found = find_methods(name);
    if(found.size() == 1) {
        names[2] = ((MethodDescriptor const *) *found.begin())->input_type()->full_name();
        names[1] = ((MethodDescriptor const *) *found.begin())->full_name();
        names[0] = ((MethodDescriptor const *) *found.begin())->output_type()->full_name();
    }
    return names;
}
std::string store::enum_value_name(std::string name) const {
    auto found = find_enum_values(name);
    if(found.size() != 1) return "";
    return ((EnumValueDescriptor const *) *found.begin())->name();
}
std::string store::enum_value_full_name(std::string name) const {
    auto found = find_enum_values(name);
    if(found.size() != 1) return "";
    return ((EnumValueDescriptor const *) *found.begin())->full_name();
}
std::string store::enum_full_name_for_value(std::string name) const {
    auto found = find_enum_values(name);
    if(found.size() != 1) return "";
    return ((EnumValueDescriptor const *) *found.begin())->type()->full_name();
}
int store::enum_value(std::string name) const { 
    auto found = find_enum_values(name);
    if(found.size() != 1) return -1;
    return ((EnumValueDescriptor const *) *found.begin())->number();
}
fc::value_type store::message_to_value_type(std::string name) const {
    auto found = find_messages(name);
    if(found.size() != 1) 
        return fc::value_type();
    return d_to_value_type((Descriptor const *) *found.begin());
}
fc::value_type store::field_to_value_type(std::string message_name, std::string field_name) const {
    auto md = importer.pool()->FindMessageTypeByName(message_name);
    return fd_to_value_type(find_field_descriptor(md, field_name));
}
}
