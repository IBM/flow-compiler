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
#include "varsub.H"
#include "grpc-helpers.H"

using namespace varsub;

int flow_compiler::find_in_blck(int block_node, std::string const &name, int *pos) const {
    auto blp = block_store.find(block_node);
    if(blp == block_store.end()) return 0;
    int lpos = 0;
    if(pos == nullptr) pos = &lpos;
    for(int e = blp->second.size(); *pos < e; ++(*pos)) 
        if(name == blp->second[*pos].first) 
            return blp->second[(*pos)++].second;
    return 0;
}

static int cpp_descriptor(std::ostream &buff, Descriptor const *dp, int pos = 0) {
    for(int i = 0, c = dp->field_count(); i < c; ++i) {
        ++pos;
        FieldDescriptor const *fdp = dp->field(i); 
        char type = 'x';
        switch(grpc_type_to_ftk(fdp->type())) {
            case FTK_FLOAT: case FTK_INTEGER:
                type = 'N';
                break;
            case FTK_STRING:
                type = 'S';
                break;
            case FTK_fldm:
                type = 'M';
                break;
            case FTK_ID:
                type = 'E';
                break;
            default:
                break;
        }
        if(fdp->is_repeated()) type = tolower(type);
        buff << '{';
        if(i + 1 == c) buff << 0; else buff << pos;
        buff <<  ",'" << type << "',\"" << fdp->json_name() << "\"}";
        if(type == 'm' || type == 'M') {
            buff << ',';
            pos = cpp_descriptor(buff, fdp->message_type(), pos);
        }
        if(i + 1 != c) buff << ',';
    }
    return pos;
}

int flow_compiler::genc_client(std::ostream &out) {
    int error_count = 0;
    std::map<std::string, std::vector<std::string>> local_vars;
    std::vector<MethodDescriptor const *> methods;
    for(auto const &ep: names) if(ep.second.first == "entry")  
        methods.push_back(method_descriptor(ep.second.second));
    
    for(auto &rn: referenced_nodes) {
        auto cli_node = rn.first;
        if(type(cli_node) == "container") 
            continue;
        methods.push_back(method_descriptor(cli_node));
    }
    std::set<MethodDescriptor const *> mset;
    std::ostringstream buff;
    for(auto mdp: methods) {
        if(contains(mset, mdp)) continue;
        mset.insert(mdp);
        append(local_vars, "METHOD_NAME", mdp->name());
        std::string full_name(get_name(mdp->service()) + "." + mdp->name());
        append(local_vars, "METHOD_FULL_NAME", full_name);
        append(local_vars, "METHOD_FULL_UPPERID", to_upper(to_identifier(full_name)));
        append(local_vars, "METHOD_FULL_ID", to_identifier(full_name));
        append(local_vars, "SERVICE_NAME", get_full_name(mdp->service()));
        append(local_vars, "GRPC_SERVICE_NAME", mdp->service()->name());
        append(local_vars, "SERVICE_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(local_vars, "SERVICE_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(local_vars, "SERVICE_INPUT_ID", to_identifier(get_full_name(mdp->input_type())));
        append(local_vars, "SERVICE_OUTPUT_ID", to_identifier(get_full_name(mdp->output_type())));
        buff.str("");
        cpp_descriptor(buff, mdp->input_type());
        append(local_vars, "SERVICE_INPUT_DESC", buff.str());
        buff.str("");
        cpp_descriptor(buff, mdp->output_type());
        append(local_vars, "SERVICE_OUTPUT_DESC", buff.str());
       
    }
    if(methods.size() < 1) {
        pcerr.AddError(main_file, -1, 0, "no service entry or node found, cannot generate client");
        return 1;
    }
#if 0
    std::cerr << "** client * global **********************************\n";
    std::cerr << join(global_vars, "\n") << "\n";
    std::cerr << "** client * local ***********************************\n";
    std::cerr << join(local_vars, "\n") << "\n";
    std::cerr << "*****************************************************\n";
#endif
    extern char const *template_client_C;
    render_varsub(out, template_client_C, global_vars, local_vars);
    return error_count;
}
