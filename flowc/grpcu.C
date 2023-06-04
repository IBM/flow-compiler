#include <google/protobuf/compiler/importer.h>
#include "grpcu.H"
#include "stru.H"
#include "filu.H"

using namespace google::protobuf;

namespace {
    /*
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
void FErrorPrinter::AddWarning(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("warning", ANSI_MAGENTA, filename, line, column, message);
    if(!filename.empty() && line >= 0 && column >= 0)
        ShowLine(filename, line, column);
}
void FErrorPrinter::AddNote(std::string const &filename, int line, int column, std::string const &message) {
    AddMessage("note", ANSI_BLUE, filename, line, column, message);
}
void ErrorPrinter::AddError(int line, int column, std::string const & message) {
    fperr.AddError(filename, line, column, message);
}
*/
compiler::DiskSourceTree source_tree;
compiler::Importer importer(&source_tree);
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

}
