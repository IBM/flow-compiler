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
using namespace stru1;

int flow_compiler::get_block_value(std::vector<int> &values, int blck, std::string const &name, bool required, std::set<int> const &accepted_types) {
    int error_count = 0;

    for(int p = 0, v = find_in_blck(blck, name, &p); v != 0; v = find_in_blck(blck, name, &p)) {
        if(accepted_types.size() > 0 && !contains(accepted_types, at(v).type)) {
            ++error_count;
            std::set<std::string> accepted;
            std::transform(accepted_types.begin(), accepted_types.end(), std::inserter(accepted, accepted.end()), node_name); 
            pcerr.AddError(main_file, at(v), sfmt() << "\"" << name << "\" must be of type " << join(accepted, ", ", " or "));
            continue;
        }
        values.push_back(v);
    }
    if(required && values.size() == 0) {
        ++error_count;
        pcerr.AddError(main_file, at(blck), sfmt() << "at least one \"" << name << "\" value must be set"); 
    }
    return error_count;
}
int flow_compiler::get_block_value(int &value, int blck, std::string const &name, bool required, std::set<int> const &accepted_types) {
    std::vector<int> values;
    value = 0;
    int error_count = get_block_value(values, blck, name, required, accepted_types);
    if(error_count == 0 && values.size() > 0) {
        if(values.size() != 1) for(unsigned i = 0; i + 1 < values.size(); ++i)
            pcerr.AddWarning(main_file, at(values[i]), sfmt() << "ignoring previously set \"" << name << "\" value");
        value = values.back();
    }
    return error_count;
}
int flow_compiler::genc_kube(std::ostream &out) {
    int error_count = 0;
    extern char const *template_kubernetes_yaml;
    extern char const *template_kubernetes_group_yaml;
    std::map<std::string, std::vector<std::string>> env_vars;
    std::set<std::string> groups;

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
                append(group_vars[g], "MAIN_ENVIRONMENT_KEY", sfmt() << to_upper(to_identifier(nn)) << "_ENDPOINT");
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

        for(auto const &nv: ni.environment)  
            buf.push_back(sfmt() << "{name: "<< nv.first << ", value: " << c_escape(render_varsub(nv.second, env_vars)) << "}");
        
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
        append(group_vars[ni.group], "NODE_UPPERID", to_upper(to_identifier(ni.xname)));

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
                append(group_vars[g], "VOLUME_COMMENT", to_line_comment(join(cp->second, " "), "# "));
            else 
                append(group_vars[g], "VOLUME_COMMENT", "");
            append(group_vars[g], "VOLUME_NAME", vn);
            append(group_vars[g], "VOLUME_UPPERID", to_upper(to_identifier(vn)));
            append(group_vars[g], "VOLUME_OPTION", to_option(vn));
        }
#if 0
    std::cerr << "**************** kube ******************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(group_vars[""], "\n") << "\n";
    std::cerr << "****************************************\n";
#endif
    render_varsub(out, template_kubernetes_yaml, global_vars, group_vars[""]);
    for(auto const &g: groups) if(!g.empty()) {
#if 0
    std::cerr << "**************** kube ******************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(group_vars[g], "\n") << "\n";
    std::cerr << "****************************************\n";
#endif
        render_varsub(out, template_kubernetes_group_yaml, global_vars, group_vars[g]);
    }

    return 0;
}
static std::string check_for_file_ref(std::string const &vv) {
    if(starts_with(vv, "{{@") && ends_with(vv, "}}"))
        return vv.substr(3, vv.length()-5);
    return "";
}
int flow_compiler::genc_composer(std::ostream &out, std::map<std::string, std::vector<std::string>> &local_vars) {
    int error_count = 0;
    std::map<std::string, std::vector<std::string>> env_vars;
    int base_port = this->base_port;

    // Make a list of all nodes and containers
    for(auto const &nr: referenced_nodes) if(!nr.second.no_call) {
        std::string const &nn = nr.second.xname;
        int pv = nr.second.port;
        append(local_vars, "NODE_PORT", std::to_string(++base_port));
        if(type(nr.first) == "node") {
            append(local_vars, "MAIN_EP_ENVIRONMENT_NAME",  sfmt() << to_upper(to_identifier(nn)) << "_ENDPOINT");
            if(nr.second.external_endpoint.empty()) {
                append(local_vars, "MAIN_EP_ENVIRONMENT_VALUE", sfmt() << "$" << to_upper(to_identifier(nn)) << "_ENDPOINT_DN:" << pv);
                append(local_vars, "MAIN_DN_ENVIRONMENT_VALUE", sfmt() << to_lower(nn));
            } else {
                append(local_vars, "MAIN_EP_ENVIRONMENT_VALUE", sfmt() << nr.second.external_endpoint);
                append(local_vars, "MAIN_DN_ENVIRONMENT_VALUE", "");
            }
        }
    }
    int tmp_count = 0;

    for(auto const &nr: referenced_nodes) if(!nr.second.no_call) {
        auto &ni = nr.second;
        std::string const &nn = ni.xname;
        int blck = nr.first;
        int old_value = 0;

        env_vars.clear();
        for(auto const &er: referenced_nodes) {
            int pv = er.second.port;
            std::string const &en = name(er.second.node);
            std::string const &nf = er.second.xname;
            if((en == nf && name(nr.second.node) != en) || nf == nn) {
                append(env_vars, en+".port", std::to_string(pv));
                append(env_vars, en+".host", to_lower(nf));
                append(env_vars, en, to_lower(nf));
            }
        }

        std::vector<std::string> env;
        for(auto const &nv: ni.environment) {
            std::string file_ref = check_for_file_ref(nv.second);
            if(!file_ref.empty()) {
                ++tmp_count;
                append(local_vars, "GLOBAL_TEMP_VARS", sfmt() << "flow_local_TMP" << tmp_count << "=\"$(cat " << file_ref << ")\"");
                env.push_back(c_escape(sfmt() << nv.first << "=" << "$flow_local_TMP" << tmp_count));
            } else {
                env.push_back(c_escape(sfmt() << nv.first << "=" << render_varsub(nv.second, env_vars)));
            }
        }

        append(local_vars, "NODE_ENVIRONMENT", join(env, ", ", "", "environment: [", "", "", "]"));
        append(local_vars, "SET_NODE_RUNTIME", ni.runtime.empty()? "#": "");
        append(local_vars, "NODE_RUNTIME", ni.runtime.empty()? ni.runtime: c_escape(ni.runtime));

        append(local_vars, "NODE_SCALE", std::to_string(ni.scale <= 0? 1: ni.scale));
        append(local_vars, "NODE_MIN_CPUS", std::to_string(ni.min_cpus <= 0? 0: ni.min_cpus));
        append(local_vars, "NODE_MAX_CPUS", std::to_string(ni.max_cpus <= 0? 0: ni.max_cpus));
        append(local_vars, "NODE_MIN_GPUS", std::to_string(ni.min_gpus <= 0? 0: ni.min_gpus));
        append(local_vars, "NODE_MAX_GPUS", std::to_string(ni.max_gpus <= 0? 0: ni.max_gpus));
        append(local_vars, "NODE_MIN_MEMORY", ni.min_memory);
        append(local_vars, "NODE_MAX_MEMORY", ni.max_memory);

        append(local_vars, "NODE_HAVE_MIN_MAX", !ni.max_memory.empty() || ni.max_gpus > 0 || ni.max_cpus > 0 
                                             || !ni.min_memory.empty() || ni.min_gpus > 0 || ni.min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MAX", !ni.max_memory.empty() || ni.max_gpus > 0 || ni.max_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN", !ni.min_memory.empty() || ni.min_gpus > 0 || ni.min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_MEMORY", !ni.min_memory.empty()? "": "#");
        append(local_vars, "NODE_HAVE_MAX_MEMORY", !ni.max_memory.empty()? "": "#");
        append(local_vars, "NODE_HAVE_MAX_CPUS", ni.max_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_CPUS", ni.min_cpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MAX_GPUS", ni.max_gpus > 0? "": "#");
        append(local_vars, "NODE_HAVE_MIN_GPUS", ni.min_gpus > 0? "": "#");
        append(local_vars, "NODE_UPPERID", to_upper(to_identifier(ni.xname)));

        std::vector<std::string> mts;
        bool have_rw_mounts = false;
        for(int t: ni.mounts) {
            auto const &minf = mounts.at(t);
            for(auto const &path: minf.paths) {
                mts.push_back(sfmt() << minf.name << ":" << path << ":" << (minf.read_only? "ro" : "rw"));
                have_rw_mounts = have_rw_mounts | !minf.read_only;
            }
        }
        append(local_vars, "HAVE_RW_VOLUMES", have_rw_mounts? "": "#");
        append(local_vars, "NODE_MOUNTS", join(mts, ", ", "", "volumes: [", "\"", "\"", "]"));
    }

    clear(local_vars, "VOLUME_OPTION");
    for(auto const &mip: mounts) {
        std::string const &vn = mip.second.name;

        append(local_vars, "VOLUME_OPTION", to_option(vn));
        append(local_vars, "VOLUME_NAME", vn);
        append(local_vars, "VOLUME_NAME_VAR", to_upper(vn));
        append(local_vars, "VOLUME_LOCAL", c_escape(mip.second.local));
        append(local_vars, "VOLUME_ARTIFACTORY", mip.second.artifactory);
        append(local_vars, "VOLUME_COS", mip.second.cos);
        append(local_vars, "VOLUME_IS_RO", mip.second.read_only? "1": "0");

        auto cp = comments.find(vn);
        if(cp != comments.end()) {
            append(local_vars, "VOLUME_HELP",  c_escape(join(cp->second, " ")));
            append(local_vars, "VOLUME_COMMENT", to_line_comment(join(cp->second, " "), "# "));
        } else {
            append(local_vars, "VOLUME_COMMENT", "");
            append(local_vars, "VOLUME_HELP", "");
        }
    }
#if 0
    std::cerr << "*************** compose ****************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(local_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
#endif
    extern char const *template_docker_compose_yaml;
    render_varsub(out, template_docker_compose_yaml, global_vars, local_vars);
    return error_count;
}
int flow_compiler::genc_composer_driver(std::ostream &outs, std::map<std::string, std::vector<std::string>> &local_vars) {
    int error_count = 0;
    extern char const *template_docker_compose_sh; 

#if 0
    std::cerr << "****************************************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(local_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
#endif

    render_varsub(outs, template_docker_compose_sh, global_vars, local_vars);
    return error_count;
}
int flow_compiler::genc_kube_driver(std::ostream &outs, std::string const &kubernetes_yaml) {
    int error_count = 0;
    extern char const *template_kubernetes_sh; 
    std::map<std::string, std::vector<std::string>> local_vars;

    clear(local_vars, "VOLUME_OPTION");
    for(auto const &mip: mounts) {
        std::string const &vn = mip.second.name;
        append(local_vars, "VOLUME_OPTION", to_option(vn));
        append(local_vars, "VOLUME_UPPERID", to_upper(to_identifier(vn)));
        append(local_vars, "VOLUME_LOCAL", c_escape(mip.second.local));
        append(local_vars, "VOLUME_COS", mip.second.cos);
        append(local_vars, "VOLUME_SECRET", to_option(mip.second.secret));
        append(local_vars, "VOLUME_PVC", to_option(mip.second.pvc));
        auto cp = comments.find(vn);
        if(cp != comments.end()) 
            append(local_vars, "VOLUME_HELP",  c_escape(join(cp->second, " ")));
        else 
            append(local_vars, "VOLUME_HELP", "");
    }
    clear(local_vars, "GROUP");
    clear(local_vars, "GROUP_UPPER");
    clear(local_vars, "GROUP_UPPERID");
    clear(local_vars, "GROUP_NODES");
    for(auto const &g: group_vars) if(!g.first.empty()) {
        append(local_vars, "GROUP", g.first);
        append(local_vars, "GROUP_UPPER", to_upper(g.first));
        append(local_vars, "GROUP_UPPERID", to_upper(to_underscore(g.first)));
        append(local_vars, "GROUP_NODES", join(all(group_vars[g.first], "G_NODE_NAME"), ", "));
        append(local_vars, "GROUP_SCALE", get(group_vars[g.first], "GROUP_SCALE"));
    } else {
        append(local_vars, "MAIN_GROUP_NODES", join(all(group_vars[g.first], "G_NODE_NAME"), ", "));
        append(local_vars, "MAIN_SCALE", get(group_vars[g.first], "GROUP_SCALE"));
    }

#if 0
    std::cerr << "****************************************\n";
    std::cerr << "********* global: \n" << join(global_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
    std::cerr << "********* local: \n" << join(local_vars, "\n") << "\n";
    std::cerr << "****************************************\n";
#endif

    set(local_vars, "KUBERNETES_YAML", kubernetes_yaml);

    render_varsub(outs, template_kubernetes_sh, global_vars, local_vars);
    return error_count;
}
