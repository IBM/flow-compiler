#include <google/protobuf/compiler/importer.h>
#include "grpcu.H"
#include "stru.H"
#include "filu.H"

using namespace google::protobuf;

namespace {
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
