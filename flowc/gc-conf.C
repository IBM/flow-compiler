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
#include "cot.H"
#include "vex.H"
#include "grpc-helpers.H"

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
                    metadata.push_back(sfmt() << "(context).AddMetadata(" << c_escape(to_option(hnv.first)) << ", flowdef::" << name(hnv.second) << ");");
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
        error_count += get_block_s(svalue, block, "group", {FTK_STRING, FTK_INTEGER}, "");
        append(vars, "CLI_NODE_GROUP", svalue);
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

int flow_compiler::genc_kube(std::ostream &out) {
    int error_count = 0;
    extern char const *template_kubernetes_yaml;
    extern char const *template_kubernetes_group_yaml;
    std::map<std::string, std::vector<std::string>> env_vars;
    std::set<std::string> groups;
/*
    for(auto const &nn: referenced_nodes) 
        groups.insert(nn.second.group);
    for(auto const &g: groups) {
        int group_scale = 1;
        for(auto &nr: referenced_nodes) {
            auto &ni = nr.second;
            std::string const &nn = ni.xname;
            int pv = ni.port;
            std::string host;
            if(ni.group == g) {
                // If the this node is part of this group, it is accessible through localhost
                host = "localhost";
                group_scale = std::max(ni.scale, group_scale);

            } else {
                // This node is accessible through service in this group
                host = std::string("@") + to_lower(to_option(sfmt() << get(global_vars, "NAME") << "-" << ni.group));
            }
            //append(env_vars[g], nn+".host", host);
            //append(env_vars[g], nn+".port", port);

            if(g.empty()) {
                append(group_vars[g], "MAIN_ENVIRONMENT_KEY", sfmt() << to_upper(to_identifier(get(global_vars, "NAME"))) <<  "_NODE_" << to_upper(to_identifier(nn)) << "_ENDPOINT");
                if(ni.external_endpoint.empty()) 
                    append(group_vars[g], "MAIN_ENVIRONMENT_VALUE", sfmt() << host << ":" << pv);
                else 
                    append(group_vars[g], "MAIN_ENVIRONMENT_VALUE", ni.external_endpoint);
            }
        }
        append(group_vars[g], "GROUP_SCALE", std::to_string(group_scale));
    }
    for(auto &nr: referenced_nodes) if(!nr.second.no_call) {
        auto &ni = nr.second;
        std::string const &nn = ni.xname;
        int blck = nr.first;
        std::vector<std::string> buf;

        env_vars.clear();
        for(auto const &er: referenced_nodes) {
            int pv = er.second.port;
            std::string const &en = name(er.second.node);
            std::string const &nf = er.second.xname;
            std::string host;
            if(ni.group == er.second.group) {
                // If the this node is part of this group, it is accessible through localhost
                host = "localhost";
            } else {
                // This node is accessible through service in this group
                host = to_lower(to_option(sfmt() << get(global_vars, "NAME") << "-" << er.second.group));
            }
            if(nf == nn || (en == nf && name(nr.second.node) != en)) {
                append(env_vars, en+".port", std::to_string(pv));
                append(env_vars, en+".host", host);
                append(env_vars, en, to_lower(nf));
            }
        }
        //std::cerr << "*** " << nn << "/" << ni.group << " ****** envvars: \n" << join(env_vars, "\n") << "\n";

        for(auto const &nv: ni.environment) {
            std::ostringstream os;
            vex::expand(os, nv.second, vex::make_smap(env_vars));
            buf.push_back(sfmt() << "{name: "<< nv.first << ", value: " << c_escape(os.str()) << "}");
        }
        
        append(group_vars[ni.group], "G_NODE_ENVIRONMENT", join(buf, ", ", "", "env: [", "", "", "]"));

        buf.clear();
        for(int mi: ni.mounts) {
            auto const &mt = mounts.at(mi);
            for(unsigned p = 0, pe = mt.paths.size(); p != pe; ++p) {
                if(p == 0)
                    buf.push_back(sfmt() << "{name: scratch-" << to_option(mt.name) << ", mountPath: " << c_escape(mt.paths[p]) << ", readOnly: " << (mt.read_only? "true": "false") << "}");
                else 
                    buf.push_back(sfmt() << "{name: scratch" << p << "-" << to_option(mt.name) << ", mountPath: " << c_escape(mt.paths[p]) << ", readOnly: " << (mt.read_only? "true": "false") << "}");
            }
        }

        append(group_vars[ni.group], "G_NODE_MOUNTS", join(buf, ", ", "", "volumeMounts: [", "", "", "]"));

        buf.clear();
        int limits_blck = 0;
        error_count += get_block_value(limits_blck, blck, "limits", false, {FTK_blck});
        if(limits_blck != 0) for(int elem_node: at(limits_blck).children) { 
            auto const &elem = at(elem_node);
            if(!is_value_type(at(elem.children[1]).type)) {
                pcerr.AddWarning(main_file, elem, sfmt() << "ignoring \"limits\" entry \"" << get_id(elem.children[0]) << "\"");
                continue;
            }
            buf.push_back(sfmt() << get_id(elem.children[0]) <<  ": " << c_escape(get_value(elem.children[1])));
        }
        append(group_vars[ni.group], "G_NODE_MIN_MEMORY", ni.min_memory);
        append(group_vars[ni.group], "G_NODE_MIN_CPUS", std::to_string(ni.min_cpus));
        append(group_vars[ni.group], "G_NODE_MIN_GPUS", std::to_string(ni.min_gpus));
        append(group_vars[ni.group], "G_NODE_MAX_MEMORY", ni.max_memory);
        append(group_vars[ni.group], "G_NODE_MAX_CPUS", std::to_string(ni.max_cpus));
        append(group_vars[ni.group], "G_NODE_MAX_GPUS", std::to_string(std::max(ni.max_gpus, ni.min_gpus)));

        append(group_vars[ni.group], "G_NODE_HAVE_MIN_MAX", !ni.max_memory.empty() || ni.max_gpus > 0 || ni.max_cpus > 0 
                                             || !ni.min_memory.empty() || ni.min_gpus > 0 || ni.min_cpus > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MAX", !ni.max_memory.empty() || ni.max_gpus > 0 || ni.max_cpus > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MIN", !ni.min_memory.empty() || ni.min_gpus > 0 || ni.min_cpus > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MIN_MEMORY", !ni.min_memory.empty()? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MAX_MEMORY", !ni.max_memory.empty()? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MAX_CPUS", ni.max_cpus > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MIN_CPUS", ni.min_cpus > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MAX_GPUS", std::max(ni.max_gpus, ni.min_gpus) > 0? "": "#");
        append(group_vars[ni.group], "G_NODE_HAVE_MIN_GPUS", ni.min_gpus > 0? "": "#");
        append(group_vars[ni.group], "NODE_NAME", ni.xname);

        std::vector<int> init_blcks;
        int init_count = 0;
        error_count += get_block_value(init_blcks, blck, "init", false, {FTK_blck});
        for(int init_blck: init_blcks) {
            buf.clear();
            int command_value = 0;
            error_count += get_block_value(command_value, init_blck, "command", false, {FTK_STRING});
            if(command_value == 0) {
                pcerr.AddWarning(main_file, at(init_blck), sfmt() << "ignoring \"init\" block without \"command\"");
                continue;
            }
            buf.push_back(sfmt() << "command: [\"/bin/sh\", \"-c\", \"" << get_string(command_value) << "\"]");
            int image_value = 0;
            error_count += get_block_value(image_value, init_blck, "image", false, {FTK_STRING});
            if(image_value == 0) 
                buf.push_back("image: \"busybox:latest\"");
            else 
                buf.push_back(sfmt() << "image: \"" << get_string(image_value) << "\"");
            buf.push_back("securityContext: {privileged: true}");
            buf.push_back(sfmt() << "name: " << to_option(nn) << "-init-" << ++init_count);
            append(group_vars[ni.group], "G_INIT_CONTAINER", join(buf, ", ", "", "{", "", "", "}"));
        }
        if(init_count > 0 || group_volumes[ni.group].size() > 0)
            set(group_vars[ni.group], "HAVE_INIT_CONTAINERS", "");
    }
    for(auto const &g: groups)
        for(auto const &vn: group_volumes[g]) {
            auto cp = comments.find(vn);
            if(cp != comments.end()) 
                append(group_vars[g], "VOLUME_COMMENT", join(cp->second, " "));
            else 
                append(group_vars[g], "VOLUME_COMMENT", "");
            append(group_vars[g], "VOLUME_NAME", vn);
        }
#if 0
    std::cerr << "**************** kube ******************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(group_vars[""], "\n") << "\n";
    std::cerr << "****************************************\n";
#endif
    auto global_smap = vex::make_smap(global_vars);
    auto gg_smap = vex::make_smap(group_vars[""]);
    vex::expand(out, template_kubernetes_yaml, vex::make_cmap(gg_smap, global_smap));
    for(auto const &g: groups) if(!g.empty()) {
#if 0
    std::cerr << "**************** kube ******************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(group_vars[g], "\n") << "\n";
    std::cerr << "****************************************\n";
#endif
        auto gg_smap = vex::make_smap(group_vars[g]);
        vex::expand(out, template_kubernetes_group_yaml, vex::make_cmap(gg_smap, global_smap));
    }
*/
    return 0;
}
static std::string check_for_file_ref(std::string const &vv) {
    if(starts_with(vv, "{{@") && ends_with(vv, "}}"))
        return vv.substr(3, vv.length()-5);
    return "";
}
int flow_compiler::genc_composer(std::ostream &out, std::map<std::string, std::vector<std::string>> &local_vars) {
    int error_count = 0;
    DEBUG_ENTER;
    std::map<std::string, std::vector<std::string>> env_vars;
    int base_port = this->base_port;
    std::set<int> cc_nodes; // all client nodes and containers
    std::set<int> cli_nodes; // all client

    for(int n: *this)
        if(at(n).type == FTK_NODE && method_descriptor(n) != nullptr)
            cli_nodes.insert(n);
        else if(at(n).type == FTK_CONTAINER)
            cc_nodes.insert(n);
    cc_nodes.insert(cli_nodes.begin(), cli_nodes.end());
    
    std::map<int, std::string> ports;
    int nc = 0, pv;
    for(int n: cc_nodes) {
        ++nc; pv = 0;
        std::string const &nn = name(n);
        error_count += get_block_i(pv, n, "port", 0);
        if(pv == 0) pv = base_port + nc;
        ports[n] = std::to_string(pv);
        append(local_vars, "NODE_PORT", std::to_string(pv));
        if(at(n).type != FTK_NODE)
            continue;
        append(local_vars, "MAIN_EP_ENVIRONMENT_NAME",  sfmt() << to_upper(to_identifier(get(global_vars, "NAME"))) <<  "_NODE_"  << to_upper(to_identifier(nn)) << "_ENDPOINT");
        std::string ep; 
        error_count += get_block_s(ep, n, "endpoint", "");
        if(ep.empty()) {
            append(local_vars, "MAIN_EP_ENVIRONMENT_VALUE", sfmt() << "$"  << to_upper(to_identifier(get(global_vars, "NAME"))) <<  "_NODE_"  << to_upper(to_identifier(nn)) << "_ENDPOINT_DN:" << pv);
            append(local_vars, "MAIN_DN_ENVIRONMENT_VALUE", sfmt() << to_lower(nn));
        } else {
            append(local_vars, "MAIN_EP_ENVIRONMENT_VALUE", sfmt() << ep);
            append(local_vars, "MAIN_DN_ENVIRONMENT_VALUE", "");
        }
    }
    int tmp_count = 0;
    for(int n: cc_nodes) {
        std::string const &nn = name(n);
        int blck = get_ne_block_node(n);

        env_vars.clear();
        for(int m: cc_nodes) {
            std::string const en = type(n);
            std::string const nf = name(n);
            if((en == nf && type(n) != en) || nf == nn) {
                std::string h = to_lower(to_option(to_identifier(nf)));
                append(env_vars, en+".port", ports[m]);
                append(env_vars, en+".host", h);
                append(env_vars, en, h);
            }
        }
        std::vector<std::string> env;
        std::map<std::string, int> nvnenv;
        error_count += get_nv_block(nvnenv, blck, "environment", {FTK_STRING});
        for(auto const &nv: nvnenv) {
            std::string value = get_string(nv.second);
            std::string file_ref = check_for_file_ref(value);
            if(!file_ref.empty()) {
                ++tmp_count;
                append(local_vars, "GLOBAL_TEMP_VARS", sfmt() << "flow_local_TMP" << tmp_count << "=\"$(cat " << file_ref << ")\"");
                env.push_back(c_escape(sfmt() << nv.first << "=" << "$flow_local_TMP" << tmp_count));
            } else {
                std::ostringstream os;
                vex::expand(os, value, vex::make_smap(env_vars));
                env.push_back(c_escape(sfmt() << nv.first << "=" << os.str()));
            }
        }
        append(local_vars, "NODE_ENVIRONMENT", join(env, ", ", "", "environment: [", "", "", "]"));

        std::string runtime; 
        error_count += get_block_s(runtime, n, "runtime", "");
        append(local_vars, "SET_NODE_RUNTIME", runtime.empty()? "#": "");
        append(local_vars, "NODE_RUNTIME", runtime.empty()? runtime: c_escape(runtime));
        int scale, min_cpus, max_cpus, min_gpus, max_gpus;
        std::string min_memory, max_memory;
        error_count += get_block_s(min_memory, n, "min_memory", "");
        error_count += get_block_s(max_memory, n, "max_memory", "");
        error_count += get_block_i(min_cpus, n, "min_cpu", 0);
        error_count += get_block_i(min_gpus, n, "min_gpu", 0);
        error_count += get_block_i(max_cpus, n, "max_cpu", 0);
        error_count += get_block_i(max_gpus, n, "max_gpu", 0);
        error_count += get_block_i(scale, n, "scale", 0);

        append(local_vars, "NODE_SCALE", std::to_string(scale <= 0? 1: scale));
        append(local_vars, "NODE_MIN_CPUS", std::to_string(min_cpus <= 0? 0: min_cpus));
        append(local_vars, "NODE_MAX_CPUS", std::to_string(max_cpus <= 0? 0: max_cpus));
        append(local_vars, "NODE_MIN_GPUS", std::to_string(min_gpus <= 0? 0: min_gpus));
        append(local_vars, "NODE_MAX_GPUS", std::to_string(max_gpus <= 0? 0: max_gpus));
        append(local_vars, "NODE_MIN_MEMORY", min_memory);
        append(local_vars, "NODE_MAX_MEMORY", max_memory);

        append(local_vars, "NODE_HAVE_MIN_MAX", !max_memory.empty() || max_gpus > 0 || max_cpus > 0 
                                             || !min_memory.empty() || min_gpus > 0 || min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MAX", !max_memory.empty() || max_gpus > 0 || max_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN", !min_memory.empty() || min_gpus > 0 || min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_MEMORY", !min_memory.empty()? "": "#");
        append(local_vars, "NODE_HAVE_MAX_MEMORY", !max_memory.empty()? "": "#");
        append(local_vars, "NODE_HAVE_MAX_CPUS", max_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_CPUS", min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MAX_GPUS", max_gpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_GPUS", min_gpus > 0? "": "#");
        append(local_vars, "NODE_NAME", to_lower(to_option(to_identifier(nn))));

        std::vector<std::string> mts;
        bool have_rw_mounts = false;
        /*
        for(int t: ni.mounts) {
            auto const &minf = mounts.at(t);
            for(auto const &path: minf.paths) {
                mts.push_back(sfmt() << minf.name << ":" << path << ":" << (minf.read_only? "ro" : "rw"));
                have_rw_mounts = have_rw_mounts | !minf.read_only;
            }
        }
        */
        append(local_vars, "HAVE_RW_VOLUMES", have_rw_mounts? "": "#");
        append(local_vars, "NODE_MOUNTS", join(mts, ", ", "", "volumes: [", "\"", "\"", "]"));
    }
    if(DEBUG_GENC) {
        std::ofstream outg(output_filename("dc-yaml-global.json"));
        stru1::to_json(outg, global_vars);
        std::ofstream outj(output_filename("dc-yaml-local.json"));
        stru1::to_json(outj, local_vars);
    }
    extern char const *template_docker_compose_yaml;
    auto local_smap = vex::make_smap(local_vars);
    auto global_smap = vex::make_smap(global_vars);
    vex::expand(out, template_docker_compose_yaml, vex::make_cmap(local_smap, global_smap));
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_deployment_driver(std::ostream &outs, std::map<std::string, std::vector<std::string>> &local_vars) {
    int error_count = 0;
    extern char const *template_driver_sh; 

#if 0
    std::ofstream outg("driver-global.json");
    stru1::to_json(outg, global_vars);
    std::ofstream outj("driver-local.json");
    stru1::to_json(outj, local_vars);
#endif

    auto local_smap = vex::make_smap(local_vars);
    auto global_smap = vex::make_smap(global_vars);
    vex::expand(outs, template_driver_sh, vex::make_cmap(local_smap, global_smap));
    return error_count;
}
