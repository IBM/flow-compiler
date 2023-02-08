#include <unistd.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "ansi-escapes.H"
#include "filu.H"
#include "gru.H"
#include "stru1.H"

namespace fc {
FErrorPrinter::FErrorPrinter(google::protobuf::compiler::DiskSourceTree *st, bool use_stdout): source_tree(st) {
    outs = use_stdout? &std::cout: &std::cerr;
}
void FErrorPrinter::AddMessage(std::string const type, ANSI_ESCAPE color, std::string const filename, int line, int column, std::string const &message) {
    *outs << ANSI_BOLD;
    *outs << filename;
    if(line >= 0) *outs << "(" << line+1;
    if(line >= 0 && column >= 0) *outs << ":" << column+1;
    if(line >= 0) *outs << ")";
    *outs << ": ";
    *outs << color;
    *outs << type << ": ";
    *outs << ANSI_RESET;
    *outs << ansi::emphasize(message, ANSI_BOLD+ANSI_GREEN) << "\n";
}
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
void FErrorPrinter::AddError(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("error", ANSI_RED, filename, line, column, message);
    if(show_line_with_error && !filename.empty() && line >= 0 && column >= 0)
        ShowLine(filename, line, column);
    ++error_count;
}
void FErrorPrinter::AddWarning(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("warning", ANSI_MAGENTA, filename, line, column, message);
    if(show_line_with_warning && !filename.empty() && line >= 0 && column >= 0)
        ShowLine(filename, line, column);
    ++warning_count;
}
void FErrorPrinter::AddNote(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("note", ANSI_BLUE, filename, line, column, message);
}
proto_compiler::proto_compiler(): 
    pcerr(&source_tree), 
    importer(&source_tree, &pcerr) {
}
enum symbol_type {
    ENUM_TYPE = '%',
    ENUM_VALUE = '#',
    MESSAGE_TYPE = '&',
    FIELD = '$',
    METHOD = '^',
    SERVICE = '@',
    PACKAGE = '!'
};
int proto_compiler::add_to_proto_path(std::string directory, std::string const &mapped_to) {
    if(!filu::is_dir(directory))
        return 1;
    directory = filu::realpath(directory);
    source_tree.MapPath(mapped_to, directory);
    grpccc += "-I"; grpccc += directory; grpccc += " ";
    protocc += "-I"; protocc += directory; protocc += " ";
    return 0;
}
static void add_symbols(std::vector<std::string> &symbol_table, google::protobuf::EnumDescriptor const *d) {
    symbol_table.push_back(stru1::sfmt() << d->full_name() << char(ENUM_TYPE));
    for(int i = 0, e = d->value_count(); i < e; ++i) {
        auto v = d->value(i);
        symbol_table.push_back(stru1::sfmt() << v->full_name() << char(ENUM_VALUE));
    }
}
static void add_symbols(std::vector<std::string> &symbol_table, google::protobuf::Descriptor const *d) {
    symbol_table.push_back(stru1::sfmt() << d->full_name() << char(MESSAGE_TYPE));
    for(int i = 0, e = d->enum_type_count(); i < e; ++i) 
        add_symbols(symbol_table, d->enum_type(i));
    for(int i = 0, e = d->nested_type_count(); i < e; ++i) 
        add_symbols(symbol_table, d->nested_type(i));
    for(int i = 0, e = d->field_count(); i < e; ++i) 
        symbol_table.push_back(stru1::sfmt() << d->field(i)->full_name() << char(FIELD));
}
void proto_compiler::add_symbols(google::protobuf::FileDescriptor const *fd) {
    if(fds.find(fd) != fds.end())
        return;
    fds.insert(fd);
    std::string package = fd->package();
    if(!package.empty() && find_package(fd->package()).empty()) 
        symbol_table.push_back(package+char(PACKAGE));
    for(int	i = 0, e = fd->dependency_count(); i < e; ++i) {
        add_symbols(fd->dependency(i));
    }
    for(int	i = 0, e = fd->public_dependency_count(); i < e; ++i) {
        add_symbols(fd->public_dependency(i));
    }
    for(int i = 0, e = fd->service_count(); i < e; ++i) {
        auto sd = fd->service(i);
        symbol_table.push_back(stru1::sfmt() << sd->full_name() << char(SERVICE));
        for(int j = 0, f = sd->method_count(); j < f; ++j) {
            auto md = sd->method(j);
            symbol_table.push_back(stru1::sfmt() << md->full_name() << char(METHOD));
        }
    }
    for(int i = 0, e = fd->message_type_count(); i < e; ++i) 
        fc::add_symbols(symbol_table, fd->message_type(i));
    for(int i = 0, e = fd->enum_type_count(); i < e; ++i) 
        fc::add_symbols(symbol_table, fd->enum_type(i));
}
int proto_compiler::compile_proto(std::string file, bool map_file_dir) {
    std::string disk_file, vfile = file;
    if(!source_tree.VirtualFileToDiskFile(file, &disk_file)) {
        std::string file_dir = filu::dirname(file);
        if(map_file_dir) 
            add_to_proto_path(file_dir.empty()? std::string("."): file_dir);
        vfile = filu::basename(file);
    }
    auto fdp = importer.Import(vfile);
    if(fdp == nullptr) 
        return 1;
    if(fdp->syntax() != google::protobuf::FileDescriptor::Syntax::SYNTAX_PROTO3) {
        pcerr.AddError(file, -1, 0, "syntax must be proto3");
        return 1;
    }
    add_symbols(fdp);
    return 0;
}
static int find_symbol(std::vector<std::string> const &symbol_table, std::string const &symbol, std::vector<int> *alternates=nullptr) {
    std::vector<int> found;
    if(alternates == nullptr)
        alternates = &found;
    for(unsigned x = 0, e = symbol_table.size(); x < e; ++x) {
        auto s = symbol_table[x];
        if(s.length() < symbol.length())
            continue;
        if(s.length() == symbol.length()) {
            if(s == symbol)
                alternates->push_back(x+1);
            continue;
        }
        if(isalnum(s[s.length()-symbol.length()-1])) 
            continue;
        if(symbol == s.substr(s.length() - symbol.length())) 
            alternates->push_back(x+1);
    }
    return alternates->size() > 0? (*alternates)[0]: 0;
}
static std::string chop(std::string s) {
    if(s.length() > 0 && !isalnum(s.back()))
        s = s.substr(0, s.length()-1);
    auto cp = s.find_first_of(':');
    return cp == std::string::npos? s: s.substr(cp+1);
}
std::string proto_compiler::find_package(std::string const &name) const {
    auto x = find_symbol(symbol_table, name+char(PACKAGE));
    return x == 0? std::string(): chop(symbol_table[x-1]);
}
template <typename LM, char SYMBOL_TYPE>
static inline auto lookup_symbol(google::protobuf::DescriptorPool const *p, 
        std::vector<std::string> const &symbol_table, LM lookup_method, std::string const &entry) {

    std::vector<int> found;
    int s = find_symbol(symbol_table, entry+SYMBOL_TYPE, &found);
    auto dp = (*p.*lookup_method)(found.size() != 1? std::string(): chop(symbol_table[s-1]));
    return dp;
}
google::protobuf::Descriptor const *proto_compiler::find_message(std::string const &message) const {
    auto lm = &google::protobuf::DescriptorPool::FindMessageTypeByName;
    return lookup_symbol<decltype(lm), char(MESSAGE_TYPE)>(pool(), symbol_table, lm, message);
}
google::protobuf::FieldDescriptor const *proto_compiler::find_field(std::string const &field) const {
    auto lm = &google::protobuf::DescriptorPool::FindFieldByName;
    return lookup_symbol<decltype(lm), char(FIELD)>(pool(), symbol_table, lm, field);
}
google::protobuf::EnumValueDescriptor const *proto_compiler::find_enumv(std::string const &name) const {
    auto lm = &google::protobuf::DescriptorPool::FindEnumValueByName;
    return lookup_symbol<decltype(lm), char(ENUM_VALUE)>(pool(), symbol_table, lm, name);
}
google::protobuf::EnumDescriptor const *proto_compiler::find_enumt(std::string const &name) const {
    auto lm = &google::protobuf::DescriptorPool::FindEnumTypeByName;
    return lookup_symbol<decltype(lm), char(ENUM_TYPE)>(pool(), symbol_table, lm, name);
}
google::protobuf::MethodDescriptor const *proto_compiler::find_method(std::string const &entry) const {
    auto lm = &google::protobuf::DescriptorPool::FindMethodByName;
    return lookup_symbol<decltype(lm), char(METHOD)>(pool(), symbol_table, lm, entry);
}
google::protobuf::ServiceDescriptor const *proto_compiler::find_service(std::string const &name) const {
    auto lm = &google::protobuf::DescriptorPool::FindServiceByName;
    return lookup_symbol<decltype(lm), char(SERVICE)>(pool(), symbol_table, lm, name);
}
static
void get_dependencies(std::vector<std::string> &results, google::protobuf::Descriptor const *dp, std::set<google::protobuf::Descriptor const *> &visited) {
    if(dp == nullptr || visited.find(dp) != visited.end())
        return;
    visited.insert(dp);
    for(int f = 0, fc = dp->field_count(); f < fc; ++f) {
        auto fd = dp->field(f);
        get_dependencies(results, fd->message_type(), visited);
        auto ed = fd->enum_type();
        if(ed != nullptr) {
            if(ed->containing_type() == nullptr) {
                std::string enum_sym = ed->file()->package()+':'+ed->full_name()+char(ENUM_TYPE);
                if(std::find(results.begin(), results.end(), enum_sym) == results.end())
                    results.push_back(enum_sym);
            } else {
                get_dependencies(results, ed->containing_type(), visited);
            }
        }
    }
    if(dp->containing_type() == nullptr) {
        std::string msg_sym = dp->file()->package()+':'+ dp->full_name()+char(MESSAGE_TYPE);
        if(std::find(results.begin(), results.end(), msg_sym) == results.end()) 
            results.push_back(msg_sym);
    } else {
        get_dependencies(results, dp->containing_type(), visited);
    }
}
int proto_compiler::get_dependencies(std::vector<std::string> &results, std::vector<std::string> const &methods) const {
    std::set<std::string> services;
    std::set<google::protobuf::MethodDescriptor const *> mdds;
    std::set<google::protobuf::Descriptor const *> visited;
    for(auto m: methods) {
        auto md = find_method(m);
        fc::get_dependencies(results, md->input_type(), visited);
        fc::get_dependencies(results, md->output_type(), visited);
        services.insert(md->service()->full_name()+char(SERVICE));
        mdds.insert(md);
    }
    for(auto m: services) {
        auto sd = find_service(chop(m));
        results.push_back(sd->file()->package()+':'+sd->full_name()+char(SERVICE));
        for(int mi = 0, mc = sd->method_count(); mi < mc; ++mi) {
            auto md = sd->method(mi);
            if(mdds.find(md) != mdds.end()) 
                results.push_back(md->file()->package()+':'+md->full_name()+char(METHOD));
        }
    }
    return services.size();
}
int proto_compiler::get_proto_text(std::vector<std::string> &results, std::vector<std::string> const &syms, int file_index) const {
    std::ostringstream out;
    std::string curservice;
    std::vector<std::string> pkgstack;
    for(auto s: syms) {
        std::string package = s.substr(0, s.find_first_of(':'));
        if(pkgstack.size() == 0 || package != pkgstack.back()) {
            if(!curservice.empty()) 
                out << "}\n";
            curservice.clear();
            if(pkgstack.size() != 0) {
                results.push_back(out.str());
                out.str("");
            }
            out << "syntax = \"proto3\";\n";
            for(unsigned p = 0, pc = pkgstack.size(); p < pc; ++p)
                out << "import \"p" << (p+file_index) << ".proto\";\n";
            out << "\n";
            pkgstack.push_back(package); 
            if(!package.empty())
                out << "package " << package << ";\n\n";
        }
        switch(symbol_type(s.back())) {
            case ENUM_TYPE:
                out << find_enumt(chop(s))->DebugString() << "\n";
            break;
            case MESSAGE_TYPE:
                out << find_message(chop(s))->DebugString() << "\n";
            break;
            case SERVICE:
                if(!curservice.empty() && curservice != s) 
                    out << "}\n";
                curservice = s;
                out << "service " << find_service(chop(s))->name() << " {\n";
            break;
            case METHOD:
                out << "  " << find_method(chop(s))->DebugString();
            break;
            default:
            break;
        }
    }
    if(!curservice.empty())
        out << "}\n";
    results.push_back(out.str());
    return results.size();
}
}
