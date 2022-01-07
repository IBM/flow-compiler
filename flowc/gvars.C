#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sys/stat.h>
#include <vector>

#include "cot.H"
#include "filu.H"
#include "flow-compiler.H"
#include "grpc-helpers.H"
#include "stru1.H"
#include "vex.H"

using namespace stru1;

int flow_compiler::set_cli_active_node_vars(decltype(global_vars) &vars, int cli_node) {
    int error_count = 0;
    assert(refcount(cli_node) > 0);
    std::string const &node_name = name(cli_node);

    clear(global_vars, "ALT_ENTRY_NAME");

    append(vars, "HAVE_ACTIVE_NODE", "");
    append(vars, "MAIN_ENTRY_DESCRIPTION", description(cli_node));
    append(vars, "MAIN_ENTRY_NAME", node_name);
    append(vars, "MAIN_ENTRY_URL", sfmt() << "/-node/" << node_name);
    auto mdp = method_descriptor(cli_node);
    append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
    append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
    std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(cli_node), true, true);
    append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
    std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), node_name, description(cli_node), true, true);
    append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
    append(vars, "MAIN_ENTRY_METHOD_NAME", mdp->name());
    append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(cli_node, default_node_timeout)));
    append(vars, "MAIN_ENTRY_PROTO", gen_proto(mdp));
    return error_count;
}
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
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(rn), true, false);
        append(vars, "CLI_OUTPUT_SCHEMA_JSON", output_schema);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), name(rn), description(rn), true, false);
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
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), description(entry_node), true, true);
        std::map<std::string, int> defsn;
        error_count += get_nv_block(defsn, block, "defaults", {FTK_STRING, FTK_FLOAT, FTK_INTEGER});
        std::map<std::string, std::string> defs;
        for(auto d: defsn)
            defs[d.first] = get_value(d.second);
        std::string input_schema = json_schema(defs, mdp->input_type(), to_upper(to_option(main_name)), main_description, true, true);
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
        std::vector<int> values;
        std::string hidden_fields;
        error_count += get_block_value(values, block, "hide", false, {FTK_STRING});
        for(int n: values) hidden_fields = hidden_fields + (hidden_fields.empty()? "": ", ") + get_string(n);
        append(vars, "ENTRY_HIDDEN_FIELDS", hidden_fields);
        values.clear();
        std::string hidden_labels;
        error_count += get_block_value(values, block, "hide_label", false, {FTK_STRING});
        for(int n: values) hidden_labels = hidden_labels + (hidden_labels.empty()? "": ", ") + get_string(n);
        append(vars, "ENTRY_HIDDEN_LABELS", hidden_labels);

        if(entry_count == 1) {
            append(vars, "MAIN_ENTRY_PROTO", gen_proto(mdp));
            append(vars, "MAIN_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "MAIN_ENTRY_NAME", mdp->name());
            append(vars, "MAIN_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "MAIN_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "MAIN_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "MAIN_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "MAIN_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(block, default_entry_timeout)));
            append(vars, "MAIN_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "MAIN_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "MAIN_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "MAIN_ENTRY_HIDDEN_FIELDS", hidden_fields);
            append(vars, "MAIN_ENTRY_HIDDEN_LABELS", hidden_labels);
        } else {
            append(vars, "ALT_ENTRY_PROTO", gen_proto(mdp));
            append(vars, "ALT_ENTRY_FULL_NAME", mdp->full_name());
            append(vars, "ALT_ENTRY_NAME", mdp->name());
            append(vars, "ALT_ENTRY_URL", sfmt() << "/" << mdp->name());
            append(vars, "ALT_ENTRY_SERVICE_NAME", get_full_name(mdp->service()));
            append(vars, "ALT_ENTRY_OUTPUT_TYPE", get_full_name(mdp->output_type()));
            append(vars, "ALT_ENTRY_INPUT_TYPE", get_full_name(mdp->input_type()));
            append(vars, "ALT_ENTRY_TIMEOUT", std::to_string(get_blck_timeout(block, default_entry_timeout)));
            append(vars, "ALT_ENTRY_OUTPUT_SCHEMA_JSON", output_schema);
            append(vars, "ALT_ENTRY_INPUT_SCHEMA_JSON", input_schema);
            append(vars, "ALT_ENTRY_DESCRIPTION", description(entry_node));
            append(vars, "ALT_ENTRY_HIDDEN_FIELDS", hidden_fields);
            append(vars, "ALT_ENTRY_HIDDEN_LABELS", hidden_labels);
        }
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
    for(auto mdp: entry_mdps) {
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), "", true, true);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), decamelize(mdp->input_type()->name()), "", true, true);
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
        append(vars, "MDP_IS_ENTRY", "1");
    }
    for(auto mdp: all_mdps) if(!cot::contains(entry_mdps, mdp)) {
        std::string output_schema = json_schema(std::map<std::string, std::string>(), mdp->output_type(), decamelize(mdp->output_type()->name()), "", true, true);
        std::string input_schema = json_schema(std::map<std::string, std::string>(), mdp->input_type(), decamelize(mdp->input_type()->name()), "", true, true);
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
        append(vars, "MDP_IS_ENTRY", "0");
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
    append(vars, prefix+"NODE_RUNTIME", runtime.empty()? runtime: c_escape(runtime));
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
        image_name = path_join(default_repository, image_name.substr(1));
    append(vars, prefix+"NODE_IMAGE", image_name);
    append(vars, prefix+"EXTERN_NODE", image_name.empty()? "#": "");
    if(!image_name.empty()) {
        append(vars, prefix+"IMAGE_NAME", image_name);
        append(vars, prefix+"IM_NODE_NAME", name(n));
    }

    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::set_def_vars(decltype(global_vars) &vars) { 
    int error_count = 0;
    DEBUG_ENTER;    
    clear(global_vars, "HAVE_DEFN");
    int defn_count = 0;
    for(int i: *this) if(at(i).type == FTK_DEFINE) {
        auto &defn = at(i);
        if(refcount(defn.children[1]) > 0) {
            append(global_vars, "DEFN", get_id(defn.children[0]));
            append(global_vars, "DEFV", get_value(defn.children[1]));
            append(global_vars, "DEFD", description(defn.children[0]));
            append(global_vars, "DEFT", at(defn.children[1]).type == FTK_STRING? "STRING": (at(defn.children[1]).type == FTK_INTEGER? "INTEGER": "FLOAT"));
            set(global_vars, "HAVE_DEFN", "");
            ++defn_count;
        } else {
            pcerr.AddWarning(main_file, defn, sfmt() << "ignoring unreferenced variable \"" << get_id(defn.children[0]) <<"\"");
        }
    }
    if(defn_count)
        set(global_vars, "DEFN_COUNT", std::to_string(defn_count));
    DEBUG_LEAVE;
    return error_count;
}