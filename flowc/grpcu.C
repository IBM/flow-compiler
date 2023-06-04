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
        fdp = importer.Import(file);
    }
    if(fdp == nullptr) 
        return 1;
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
int store::lookup(std::string &match, std::vector<std::string> did, std::set<std::string> *allmp, bool match_methods, bool match_messages, bool match_enums) const {
    std::set<std::string> allm;
    if(allmp == nullptr) allmp = &allm;
    match.clear();
    std::string dids = stru::join(did, ".");

    for(auto p: file_descriptors) {
        auto fd = (FileDescriptor const *)p;
        if(match_methods) {
            for(int i = 0, e = fd->service_count(); i < e; ++i) 
                for(int j = 0,  f = fd->service(i)->method_count(); j < f; ++j) {
                    std::cerr << fd->service(i)->method(j)->name() << " <<<<=====>>>> " << dids << "\n";
                    std::cerr << fd->service(i)->name() + "." +  fd->service(i)->method(j)->name() << " <<<<=====>>>> " << dids << "\n";
                    if(fd->service(i)->method(j)->name() == dids || fd->service(i)->name() + "." +  fd->service(i)->method(j)->name() == dids) {
                        std::cerr << "FOUND!\n";
                        allmp->insert(fd->service(i)->method(j)->full_name());
                    }
                }
        }
        if(match_messages) {
            for(int i = 0, e = fd->message_type_count(); i < e; ++i) {
                std::cerr << fd->message_type(i)->name() << " <<<<=====>>>> " << dids << "\n";
                std::cerr << fd->message_type(i)->full_name() << " <<<<=====>>>> " << dids << "\n";
                if(fd->message_type(i)->name() == dids || fd->message_type(i)->full_name() == dids) {
                    std::cerr << "FOUND!\n";
                    allmp->insert(fd->message_type(i)->full_name());
                }
            }
        }
        if(match_enums) {
            for(int i = 0, e = fd->enum_type_count(); i < e; ++i) {
                std::cerr << fd->enum_type(i)->name() << " <<<<=====>>>> " << dids << "\n";
                std::cerr << fd->enum_type(i)->full_name() << " <<<<=====>>>> " << dids << "\n";
                if(fd->enum_type(i)->name() == dids || fd->enum_type(i)->full_name() == dids) {
                    std::cerr << "FOUND!\n";
                    allmp->insert(fd->enum_type(i)->full_name());
                }
            }
        }
    }
    if(allmp->size() == 1)
        match = *allmp->begin();
    return allmp->size() == 1? 0: 1;
};

}
