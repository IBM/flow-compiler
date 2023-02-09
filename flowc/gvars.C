#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sys/stat.h>
#include <vector>

#include "cot.H"
#include "flow-compiler.H"
#include "grpc-helpers.H"
#include "stru1.H"
#include "filu.H"

using namespace stru1;
int flow_compiler::set_cli_node_vars(decltype(global_vars) &vars) {
    int error_count = 0, node_count = 0, cli_count = 0, cc_value = 0;
    std::set<std::string> all_nodes;
    std::map<std::string, int> headers;
    for(int rn: get_all_referenced_nodes()) {
        ++node_count;
        all_nodes.insert(to_upper(to_identifier(name(rn))));
        append(vars, "A_NODE_NAME", name(rn));
        if(method_descriptor(rn) == nullptr) 
            continue;
        ++cli_count;
        int block = get_ne_block_node(rn);

        std::string set_metadata;
        headers.clear();
        error_count += get_nv_block(headers, block, "headers", {FTK_STRING, FTK_FLOAT, FTK_INTEGER});
        if(headers.size() > 0) {
            std::vector<std::string> metadata;
            for(auto const &hnv: headers) {
                if(name.has(hnv.second)) 
                    metadata.push_back(sfmt() << "(context).AddMetadata(" << c_escape(to_option(hnv.first)) << ", flowdef::v" << name(hnv.second) << ");");
                else
                    metadata.push_back(sfmt() << "(context).AddMetadata(" << c_escape(to_option(hnv.first)) << ", " << c_escape(get_string(hnv.second)) << ");");
            }
            set_metadata = join(metadata, " ", " ", "{", "", "", "}");
        }
        append(vars, "CLI_NODE_METADATA", set_metadata);
        append(vars, "CLI_NODE_LINE", sfmt() << at(rn).token.line);
        append(vars, "CLI_NODE_DESCRIPTION", description(rn));
        append(vars, "CLI_NODE_NAME", name(rn));
        append(vars, "CLI_NODE", name(rn));
        append(vars, "CLI_NODE_URL", sfmt() << "/-node/" << node_name);
        auto mdp = method_descriptor(rn);
        append(vars, "CLI_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "CLI_GRPC_SERVICE_NAME", mdp->service()->name());
        append(vars, "CLI_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "CLI_INPUT_TYPE", get_full_name(mdp->input_type()));
        std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), description(rn));
        append(vars, "CLI_OUTPUT_SCHEMA_JSON", output_schema);
        std::string input_schema = json_schema(mdp->input_type(), name(rn), description(rn));
        append(vars, "CLI_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "CLI_PROTO", gen_proto(mdp));
        append(vars, "CLI_METHOD_NAME", mdp->name());
        append(vars, "CLI_NODE_TIMEOUT", std::to_string(get_blck_timeout(rn, default_node_timeout)));
        std::string svalue;
        error_count += get_block_s(svalue, block, "endpoint", {FTK_STRING}, "");
        append(vars, "CLI_NODE_ENDPOINT", svalue);
        append(vars, "CLI_NODE_GROUP", group(rn));
        cc_value = 0;
        error_count += get_block_i(cc_value, block, "replicas", default_maxcc);
        if(cc_value <= 0) {
            pcerr.AddWarning(main_file, at(rn), sfmt() << "ignoring invalid value for the number of concurrent clients: \"" << cc_value <<"\"");
            cc_value = default_maxcc;
        }
        append(vars, "CLI_NODE_MAX_CONCURRENT_CALLS", std::to_string(cc_value));
    }
    if(cli_count > 0) 
        set(vars, "HAVE_CLI", "");
    set(vars, "CLI_NODE_COUNT", sfmt() << cli_count);
    set(vars, "A_NODE_COUNT", sfmt() << node_count);
    std::string no_node_name = "NOTANODE";
    if(cot::contains(all_nodes, no_node_name))
        for(int i = 0; i < 10000; ++i) 
            if(cot::contains(all_nodes, no_node_name))
                no_node_name = sfmt() << "NOT_A_NODE_" << i;
    set(vars, "NO_NODE_NAME", no_node_name);
    return error_count;
}

int flow_compiler::set_entry_vars(decltype(global_vars) &vars) {
    int error_count = 0;
    std::set<int> entry_node_set;
    // Sort the entries in source order
    for(int n: *this) if(at(n).type == FTK_ENTRY)  
        entry_node_set.insert(n);
    
    ServiceDescriptor const *sdp = nullptr;
    int entry_count = 0;
    std::set<MethodDescriptor const *> entry_mdps, all_mdps;
    for(int entry_node: entry_node_set) {
        ++entry_count;
        MethodDescriptor const *mdp = method_descriptor(entry_node);
        if(sdp != nullptr && sdp != mdp->service()) {
            error_count += 1;
            pcerr.AddError(main_file, -1, 0, "all entries must be methods of the same service");
        }
        int block = get_ne_block_node(entry_node);
        sdp = mdp->service();

        int input_block = 0;
        std::string input_name = input_label;
        for(int n: *this) if(at(n).type == FTK_INPUT && name(n) == input_name) {
            input_block = at(n).children.back(); break;
        }
        auto get_inpfilprp = [&](std::string const &field_name, std::string const &prop_name) -> std::tuple<std::string, int, std::string> { 
                    int n = get_nblck_value(input_block, field_name, prop_name); 
                    return n? (
                        name.has(n)?
                            std::tuple<std::string, int, std::string>{get_value(n), at(n).type, name(n)}:
                            std::tuple<std::string, int, std::string>{get_value(n), at(n).type, ""}):
                        std::tuple<std::string, int, std::string>{"", 0, ""}; 
                };

        std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), description(entry_node));
        std::string input_schema = json_schema_p(mdp->input_type(), to_upper(to_option(main_name)), description(1), get_inpfilprp);

        entry_mdps.insert(mdp); all_mdps.insert(mdp);
        append(vars, "ENTRY_PROTO", gen_proto(mdp));
        append(vars, "ENTRY_FULL_NAME", mdp->full_name());
        append(vars, "ENTRY_NAME", mdp->name());
        append(vars, "ENTRY_URL", sfmt() << "/" << mdp->name());
        append(vars, "ENTRY_SERVICE_SHORT_NAME", get_name(mdp->service()));
        append(vars, "ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "ENTRY_OUTPUT_SHORT_TYPE", get_name(mdp->output_type()));
        append(vars, "ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "ENTRY_INPUT_SHORT_TYPE", get_name(mdp->input_type()));
        append(vars, "ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "ENTRY_TIMEOUT", std::to_string(get_blck_timeout(block, default_entry_timeout)));
        append(vars, "ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "ENTRY_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "ENTRY_DESCRIPTION", description(entry_node));
        append(vars, "ENTRY_ORDER", sfmt() << entry_count);
    }
    if(entry_count > 1)
        set(vars, "HAVE_ALT_ENTRY", "");
    set(vars, "ENTRY_COUNT", sfmt() << entry_count);
    set(vars, "ALT_ENTRY_COUNT", sfmt() << entry_count-1);
    set(vars, "ENTRIES_PROTO", gen_proto(entry_mdps));
    for(int rn: get_all_referenced_nodes()) {
        MethodDescriptor const *mdp = method_descriptor(rn);
        if(mdp != nullptr)
            all_mdps.insert(mdp);
    }
    std::vector<MethodDescriptor const *> mdpvec(entry_mdps.begin(), entry_mdps.end());
    for(auto mdp: all_mdps) if(!cot::contains(entry_mdps, mdp)) 
        mdpvec.push_back(mdp);
    for(unsigned i = 0, e = entry_mdps.size(), f = mdpvec.size(); i < f; ++i) {
        auto mdp = mdpvec[i];
        std::string output_schema = json_schema(mdp->output_type(), decamelize(mdp->output_type()->name()), "");
        std::string input_schema = json_schema(mdp->input_type(), decamelize(mdp->input_type()->name()), "");
        append(vars, "MDP_PROTO", gen_proto(mdp));
        append(vars, "MDP_FULL_NAME", mdp->full_name());
        append(vars, "MDP_NAME", mdp->name());
        append(vars, "MDP_SERVICE_SHORT_NAME", get_name(mdp->service()));
        append(vars, "MDP_SERVICE_NAME", get_full_name(mdp->service()));
        append(vars, "MDP_OUTPUT_SHORT_TYPE", get_name(mdp->output_type()));
        append(vars, "MDP_OUTPUT_TYPE", get_full_name(mdp->output_type()));
        append(vars, "MDP_INPUT_SHORT_TYPE", get_name(mdp->input_type()));
        append(vars, "MDP_INPUT_TYPE", get_full_name(mdp->input_type()));
        append(vars, "MDP_OUTPUT_SCHEMA_JSON", output_schema);
        append(vars, "MDP_INPUT_SCHEMA_JSON", input_schema);
        append(vars, "MDP_IS_ENTRY", i < e? "1": "0");
    }
    set(vars, "MDP_COUNT", sfmt() << all_mdps.size());
    set(vars, "ALL_NODES_PROTO", gen_proto(all_mdps));
    return error_count;
}
int flow_compiler::group_info(int gn, std::map<std::string, std::vector<std::string>> &vars, std::string const &prefix) {
    int error_count = 0;
    DEBUG_ENTER;    
    if(gn <= 0) {
        std::map<std::string, int> fgn;
        for(int n: *this)
            if((at(n).type == FTK_NODE && method_descriptor(n) != nullptr) || at(n).type == FTK_CONTAINER) {
                if(!cot::contains(fgn, group(n)))
                    fgn[group(n)] = n;
            }
         for(auto g: fgn) if(g.first != main_group_name || gn < 0)
             error_count += group_info(g.second, vars, prefix);
    } else {
        /*
        for(auto gp: groups) {
            for(int n: cc_nodes) {
                std::string host; int scale;
                if(cot::contains(gp.second, n)) {
                    host = "localhost"; 
                    error_count += get_block_i(scale, n, "scale", 0);
                    group_scale = std::max(scale, group_scale);
                } else {
                    host = std::string("@") + to_lower(to_option(to_identifier(sfmt() << get(global_vars, "NAME") << "-" << gp.first)));
                }
                if(gp.first != main_group_name) 
                    continue;

                std::string endpoint;
                error_count += get_block_s(endpoint, n, "endpoint", "");

                append(group_vars, "MAIN_ENVIRONMENT_KEY", sfmt() << to_upper(to_identifier(get(global_vars, "NAME"))) <<  "_NODE_" << to_upper(to_identifier(name(n))) << "_ENDPOINT");
                if(endpoint.empty()) 
                    append(group_vars, "MAIN_ENVIRONMENT_VALUE", sfmt() << host << ":" << ports[n]);
                else 
                    append(group_vars, "MAIN_ENVIRONMENT_VALUE", endpoint);
            }
            append(group_vars, "G_SCALE", std::to_string(group_scale));
            append(alln_vars, "GROUP", gp.first);
            append(alln_vars, "GROUP_SCALE", std::to_string(group_scale));
        }
        */
        append(vars, prefix+"GROUP", group(gn));
        int group_scale = 1;
        for(int n: *this)
            if(((at(n).type == FTK_NODE && method_descriptor(n) != nullptr) || at(n).type == FTK_CONTAINER) && group(n) == group(gn)) {
                // grab scale from nodes with image or endpoint with localhost, but not localhost ips
                std::string endpoint, image;
                error_count += get_block_s(endpoint, n, "endpoint", "") 
                    + get_block_s(image, n, "image", "");
                if(!image.empty() || endpoint.empty() || stru1::starts_with("localhost:", endpoint)) {
                    int scale;
                    error_count += get_block_i(scale, n, "scale", 0);
                    group_scale = std::max(scale, group_scale);
                }
            }
        append(vars, prefix+"GROUP_SCALE", std::to_string(group_scale));
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::node_info(int n, std::map<std::string, std::vector<std::string>> &vars, std::string const &prefix) {
    int error_count = 0;
    DEBUG_ENTER;    
    std::string runtime; 
    error_count += get_block_s(runtime, n, "runtime", "");
    append(vars, prefix+"SET_NODE_RUNTIME", runtime.empty()? "#": "");
    append(vars, prefix+"NODE_RUNTIME", runtime);
    int scale, min_cpus, max_cpus, min_gpus, max_gpus;
    std::string min_memory, max_memory;
    error_count += get_block_s(min_memory, n, "min_memory", "");
    error_count += get_block_s(max_memory, n, "max_memory", "");
    error_count += get_block_i(min_cpus, n, "min_cpu", 0);
    error_count += get_block_i(min_gpus, n, "min_gpu", 0);
    error_count += get_block_i(max_cpus, n, "max_cpu", 0);
    error_count += get_block_i(max_gpus, n, "max_gpu", 0);
    error_count += get_block_i(scale, n, "scale", 0);

    append(vars, prefix+"NODE_SCALE", std::to_string(scale <= 0? 1: scale));
    append(vars, prefix+"NODE_MIN_CPUS", std::to_string(min_cpus <= 0? 0: min_cpus));
    append(vars, prefix+"NODE_MAX_CPUS", std::to_string(max_cpus <= 0? 0: max_cpus));
    append(vars, prefix+"NODE_MIN_GPUS", std::to_string(min_gpus <= 0? 0: min_gpus));
    append(vars, prefix+"NODE_MAX_GPUS", std::to_string(max_gpus <= 0? 0: max_gpus));
    append(vars, prefix+"NODE_MIN_MEMORY", min_memory);
    append(vars, prefix+"NODE_MAX_MEMORY", max_memory);

    append(vars, prefix+"NODE_HAVE_MIN_MAX", !max_memory.empty() || max_gpus > 0 || max_cpus > 0 
            || !min_memory.empty() || min_gpus > 0 || min_cpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MAX", !max_memory.empty() || max_gpus > 0 || max_cpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MIN", !min_memory.empty() || min_gpus > 0 || min_cpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MIN_MEMORY", !min_memory.empty()? "": "#");
    append(vars, prefix+"NODE_HAVE_MAX_MEMORY", !max_memory.empty()? "": "#");
    append(vars, prefix+"NODE_HAVE_MAX_CPUS", max_cpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MIN_CPUS", min_cpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MAX_GPUS", max_gpus > 0? "": "#");
    append(vars, prefix+"NODE_HAVE_MIN_GPUS", min_gpus > 0? "": "#");
    append(vars, prefix+"NODE_NAME", to_lower(to_option(to_identifier(name(n)))));
    append(vars, prefix+"NODE_GROUP", to_lower(to_option(to_identifier(group(n)))));
    std::string endpoint, image_name;
    error_count += get_block_s(endpoint, n, "endpoint", "");
    error_count += get_block_s(image_name, n, "image", "");
    append(vars, prefix+"NODE_ENDPOINT", endpoint);
    if(!image_name.empty() && image_name[0] == '/')
        image_name = filu::path_join(default_repository, image_name.substr(1));
    append(vars, prefix+"NODE_IMAGE", image_name);
    std::string grpc_method;
    if(method_descriptor(n) != nullptr) grpc_method = method_descriptor(n)->full_name();
    append(vars, prefix+"NODE_GRPC", grpc_method);
    append(vars, prefix+"EXTERN_NODE", image_name.empty()? "#": "");
    if(!image_name.empty()) {
        append(vars, prefix+"IMAGE_NAME", image_name);
        append(vars, prefix+"IM_NODE_NAME", name(n));
    }
    // add the volumes for this node
     
    std::map<std::string, std::string> minfo;
    for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) {
        minfo.clear();
        error_count += get_mount_info(minfo, m);
        append(vars, prefix+"VOLUME_NAME", minfo["name"]);
        append(vars, prefix+"VOLUME_LOCAL", minfo["local"]);
        append(vars, prefix+"VOLUME_COS", minfo["url"]);
        append(vars, prefix+"VOLUME_SECRET", minfo["secret"]);
        append(vars, prefix+"VOLUME_PVC", minfo["pvc"]);
        append(vars, prefix+"VOLUME_ISRO", minfo["access"] == "ro"? "1": "0");
        append(vars, prefix+"VOLUME_ACCESS", minfo["access"]);
        if(description.has(get_previous_sibling(n))) {
            append(vars, prefix+"VOLUME_COMMENT", description(get_previous_sibling(n)));
        } else {
            append(vars, prefix+"VOLUME_COMMENT", "");
        }
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::set_def_vars(decltype(global_vars) &vars) { 
    int error_count = 0;
    DEBUG_ENTER;    
    // Global definitions
    clear(vars, "HAVE_DEFN");
    clear(vars, "DEFN_COUNT");
    int defn_count = 0;
    for(int i: *this) if(at(i).type == FTK_DEFINE) {
        auto &defn = at(i);
        if(refcount(defn.children[1]) > 0) {
            append(vars, "DEFN", get_id(defn.children[0]));
            append(vars, "DEFV", get_value(defn.children[1]));
            append(vars, "DEFD", description(defn.children[0]));
            append(vars, "DEFT", at(defn.children[1]).type == FTK_STRING? "STRING": (at(defn.children[1]).type == FTK_INTEGER? "INTEGER": "FLOAT"));
            set(vars, "HAVE_DEFN", "");
            ++defn_count;
        } else {
            pcerr.AddWarning(main_file, defn, sfmt() << "ignoring unreferenced variable \"" << get_id(defn.children[0]) <<"\"");
        }
    }
    if(defn_count)
        set(vars, "DEFN_COUNT", std::to_string(defn_count));
    // File references in the environment
    std::set<std::string> env_file_refs;
    for(int i: *this) if(at(i).type == FTK_NODE || at(i).type == FTK_CONTAINER) {
        std::map<std::string, int> nvnenv;
        error_count += get_nv_block(nvnenv, i, "environment", {FTK_STRING, FTK_INTEGER, FTK_FLOAT});
        for(auto const &nv: nvnenv) {
            std::string value = get_text(nv.second);
            if(starts_with(value, "{{@") && ends_with(value, "}}")) 
                env_file_refs.insert(value.substr(3, value.length()-5));
        }
    }
    clear(vars, "EFR_FILENAME");
    clear(vars, "EFR_ID");
    clear(vars, "EFR_COUNT");
    int efrc = 0;
    for(auto const &efr: env_file_refs) {
        append(vars, "EFR_FILENAME", efr);
        append(vars, "EFR_ID", std::to_string(++efrc));
    }
    if(efrc > 0)
        set(vars, "EFR_COUNT", std::to_string(efrc));

    DEBUG_LEAVE;
    return error_count;
}
