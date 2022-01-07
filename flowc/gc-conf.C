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
    
int flow_compiler::get_mount_info(std::map<std::string, std::string> &info, int mntn) {
    int error_count = 0;
    MASSERT(at(mntn).type == FTK_MOUNT) << "node " << mntn << " is not a mount hash\n";
    info["name"] = name(mntn);
    int dsc = 0, vn = 0; std::string value = "ro";
    error_count += get_block_value(vn, mntn, "access", false, {FTK_STRING});
    if(vn > 0) {
        value = to_lower(get_string(vn));
        bool ro = value == "ro" || value == "read-only"  || value == "readonly";
        bool rw = value == "rw" || value == "read-write" || value == "readwrite";
        if(ro == rw) {
            ++error_count;
            pcerr.AddError(main_file, at(vn), sfmt() << "access for \"" << name(mntn) << "\" must be either 'read-only' or 'read-write'");
        }
        value = ro? "ro": "rw";
    }
    info["access"] = value;
    vn = 0;
    error_count += get_block_value(vn, mntn, "url", false, {FTK_STRING});
    if(vn == 0)
        error_count += get_block_value(vn, mntn, "cos", false, {FTK_STRING});
    if(vn == 0)
        error_count += get_block_value(vn, mntn, "artifactory", false, {FTK_STRING});
    if(vn > 0) {
        info["url"] = get_string(vn);
        ++dsc;
    }
    vn = 0;
    error_count += get_block_value(vn, mntn, "secret", false, {FTK_STRING});
    info["secret"] = vn > 0? get_string(vn): std::string();
    vn = 0;
    error_count += get_block_value(vn, mntn, "pvc", false, {FTK_STRING});
    if(vn > 0) {
        ++dsc;
        info["pvc"] = get_string(vn);
    }
    vn = 0;
    error_count += get_block_value(vn, mntn, "path", false, {FTK_STRING});

    info["path"] = vn > 0? get_string(vn): std::string();
    if(info["path"].empty()) 
        pcerr.AddError(main_file, at(mntn), sfmt() << "undefined or empty path for \"" << name(mntn) << "\"");
    vn = 0;
    error_count += get_block_value(vn, mntn, "local", false, {FTK_STRING});
    if(vn > 0) {
        info["local"] =  get_string(vn);
        ++dsc;
    } 
    /* TODO: allow for multiple paths?
       std::vector<int> paths; 
       error_count += get_block_value(paths, v, "path", true, {FTK_STRING});
       for(auto v: paths) {
        std::string path = get_string(v);
        if(path.empty()) 
        pcerr.AddWarning(main_file, at(v), "empty \"path\" value");
        }
    */
    return error_count;
}
static std::string check_for_file_ref(std::string const &vv) {
    if(starts_with(vv, "{{@") && ends_with(vv, "}}"))
        return vv.substr(3, vv.length()-5);
    return "";
}
int flow_compiler::get_environment(std::vector<std::pair<std::string, std::string>> &env, int n, std::map<int, std::string> const &node_port, bool group_mode) {
    int error_count = 0;
    DEBUG_ENTER;
    std::map<std::string, std::vector<std::string>> env_vars;
    std::string ngroup;
    if(group_mode)
        error_count += get_block_s(ngroup, n, "group", {FTK_STRING, FTK_INTEGER}, "");

    for(auto np: node_port) {
        int m = np.first;
        std::string const en = type(m);
        std::string const nf = name(m);
        if((en == nf && type(n) != en) || nf == name(n)) {
            std::string host = to_lower(to_option(to_identifier(nf)));
            if(group_mode) {
                std::string mgroup;
                error_count += get_block_s(mgroup, m, "group", {FTK_STRING, FTK_INTEGER}, "");
                if(ngroup == mgroup) {
                    // If the this node is part of this group, it is accessible through localhost
                    host = "localhost";
                } else {
                    // This node is accessible through service in this group
                    host = to_lower(to_option(sfmt() << get(global_vars, "NAME") << "-" << mgroup));
                }
            }
            append(env_vars, en+".port", np.second);
            append(env_vars, en+".host", host);
            append(env_vars, en, host);
        }
    }
    std::map<std::string, int> nvnenv;
    error_count += get_nv_block(nvnenv, n, "environment", {FTK_STRING, FTK_INTEGER, FTK_FLOAT});
    for(auto const &nv: nvnenv) {
        std::string value = get_text(nv.second);
        std::string file_ref = check_for_file_ref(value);
        if(!file_ref.empty()) {
            std::string name;
            std::string value = sfmt() << "=\"$(cat " << file_ref << ")\"";
            for(auto gv: global_vars["GLOBAL_TEMP_VARS"]) if(stru1::ends_with(gv, value)) {
                name = stru1::remove_suffix(gv, value);
                break;
            }
            if(name.empty()) {
                name =  sfmt() << "flow_local_TMP" << (1 + global_vars["GLOBAL_TEMP_VARS"].size());
                append(global_vars, "GLOBAL_TEMP_VARS", name+value);
            }
            env.push_back(std::make_pair(nv.first, std::string(sfmt() << "$" << name)));
        } else {
            std::ostringstream os;
            vex::expand(os, value, vex::make_smap(env_vars));
            env.push_back(std::make_pair(nv.first, os.str()));
        }
    }
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_dcs_conf(std::ostream &out, std::map<std::string, std::vector<std::string>> &local_vars) {
    int error_count = 0;
    DEBUG_ENTER;
    int base_port = this->base_port;
    std::set<int> cc_nodes; // all client nodes and containers

    for(int n: *this)
        if((at(n).type == FTK_NODE && method_descriptor(n) != nullptr) || at(n).type == FTK_CONTAINER)
            cc_nodes.insert(n);

    std::map<int, std::string> ports;
    int nc = 0, pv;
    for(int n: cc_nodes) {
        ++nc; pv = 0;
        std::string const &nn = name(n);
        error_count += get_block_i(pv, n, "port", 0);
        if(pv == 0) pv = base_port + nc;
        ports[n] = std::to_string(pv);
        append(local_vars, "NODE_PORT", std::to_string(pv));
        append(local_vars, "IMAGE_PORT", std::to_string(pv));
    }
    int tmp_count = 0;
    for(int n: cc_nodes) {
        std::string const &nn = name(n);
        int blck = get_ne_block_node(n);

        std::vector<std::string> buf;
        std::vector<std::pair<std::string, std::string>> env;
        error_count += get_environment(env, n, ports);
        for(auto nv: env)
            buf.push_back(json_escape(sfmt() << nv.first << "=" << nv.second));
        append(local_vars, "NODE_ENVIRONMENT", join(buf, ", ", "", "environment: [", "", "", "]"));
        error_count += node_info(n, local_vars);
        std::vector<std::string> mts;
        bool have_rw_mounts = false;
        std::map<std::string, std::string> minfo;
        for(int mn: subtree(n)) if(at(mn).type == FTK_MOUNT) {
            minfo.clear();
            error_count += get_mount_info(minfo, mn);
            mts.push_back(sfmt() << minfo["name"] << ":" << minfo["path"] << ":" << minfo["access"]);
            have_rw_mounts = have_rw_mounts | minfo["access"] == "rw";
        }
        append(local_vars, "HAVE_RW_VOLUMES", have_rw_mounts? "": "#");
        append(local_vars, "NODE_MOUNTS", join(mts, ", ", "", "volumes: [", "\"", "\"", "]"));
    }
    if(DEBUG_GENC) {
        std::ofstream outg(output_filename("dc-yaml-global.json"));
        stru1::to_json(outg, global_vars);
        std::ofstream outj(output_filename("dc-yaml-local.json"));
        stru1::to_json(outj, local_vars);
    }
    extern char const *template_docker_yaml;
    auto local_smap = vex::make_smap(local_vars);
    auto global_smap = vex::make_smap(global_vars);
    vex::expand(out, template_docker_yaml, vex::make_cmap(local_smap, global_smap));
    DEBUG_LEAVE;
    return error_count;
}
int flow_compiler::genc_k8s_conf(std::ostream &out) {
    int error_count = 0;
    DEBUG_ENTER;
    std::set<int> cc_nodes; // all client nodes and containers
    std::map<std::string, std::set<int>> groups; // per group node sets
    for(int n: *this)
        if((at(n).type == FTK_NODE && method_descriptor(n) != nullptr) || at(n).type == FTK_CONTAINER) {
            cc_nodes.insert(n);
            groups[group(n)].insert(n);
        }

    extern char const *template_kubernetes_yaml;
    extern char const *template_kubernetes_group_yaml;

    std::map<int, std::string> ports;
    // grab all pre-defined ports
    for(int n: cc_nodes) {
        int port = 0;
        error_count += get_block_i(port, n, "port", 0);
        if(port != 0)
            ports[n] = std::to_string(port);
    }
    // allocate ports for nodes without one
    int c = 0;
    for(int n: cc_nodes) if(!cot::contains(ports, n)) {
        std::string image_name;
        error_count += get_block_s(image_name, n, "image", "");
        if(!image_name.empty()) {
            while(cot::has_value(ports, std::to_string(base_port + ++c)));
            ports[n] = std::to_string(base_port + c);
        }
    }
    std::map<std::string, std::map<std::string, std::vector<std::string>>> g_group_vars;
    std::map<std::string, std::vector<std::string>> alln_vars;
    for(auto group: groups) for(int n: group.second) {
        std::string const &nn = name(n);
        int blck = get_ne_block_node(n);
        std::vector<std::string> buf;
        std::vector<std::pair<std::string, std::string>> env;
        auto &local_vars = g_group_vars[group.first];
        error_count += get_environment(env, n, ports, true);
        for(auto nv: env)
            buf.push_back(sfmt() << "{name: "<< nv.first << ", value: " << json_escape(nv.second) << "}");

        append(alln_vars, "NODE_ENVIRONMENT", join(buf, ", ", "", "env: [", "", "", "]"));
        append(local_vars, "G_NODE_ENVIRONMENT", join(buf, ", ", "", "env: [", "", "", "]"));
        append(alln_vars, "NODE_PORT", ports[n]);
        append(local_vars, "G_NODE_PORT", ports[n]);
        append(alln_vars, "IMAGE_PORT", ports[n]);
        error_count += node_info(n, alln_vars);
        error_count += node_info(n, local_vars, "G_");

        buf.clear();
        for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) {
            std::map<std::string, std::string> minfo;
            error_count += get_mount_info(minfo, m);
            buf.push_back(sfmt() << "{name: scratch-" << to_option(minfo["name"]) << ", mountPath: " << c_escape(minfo["path"]) << ", readOnly: " << (minfo["access"] == "ro"? "true": "false") << "}");
            append(local_vars, "G_VOLUME_NAME", minfo["name"]);
            append(local_vars, "G_VOLUME_LOCAL", minfo["local"]);
            append(local_vars, "G_VOLUME_COS", minfo["url"]);
            append(local_vars, "G_VOLUME_SECRET", minfo["secret"]);
            append(local_vars, "G_VOLUME_PVC", minfo["pvc"]);
            append(local_vars, "G_VOLUME_ISRO", minfo["access"] == "ro"? "1": "0");
            append(local_vars, "G_VOLUME_ACCESS", minfo["access"]);
            if(description.has(get_previous_sibling(n))) {
                append(local_vars, "G_VOLUME_COMMENT", description(get_previous_sibling(n)));
            } else {
                append(local_vars, "G_VOLUME_COMMENT", "");
            }
        }
        append(local_vars, "G_NODE_MOUNTS", join(buf, ", ", "", "volumeMounts: [", "", "", "]"));

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
            append(local_vars, "INIT_CONTAINER", join(buf, ", ", "", "{", "", "", "}"));
        }
        DEBUG_CHECK("in group" << group.first << " ic: "<< init_count);
        if(init_count > 0)
            set(local_vars, "HAVE_INIT_CONTAINERS", "");
    }
    auto global_smap = vex::make_smap(global_vars);
    auto local_smap = vex::make_smap(alln_vars);
    auto gg_smap = vex::make_smap(g_group_vars[main_group_name]);
    vex::expand(out, template_kubernetes_yaml, vex::make_cmap(gg_smap, local_smap, global_smap));
    if(DEBUG_GENC) {
        std::ofstream outg(output_filename("k8s-yaml-global.json"));
        stru1::to_json(outg, global_vars);
        std::ofstream outj(output_filename("k8s-yaml-local.json"));
        stru1::to_json(outj, g_group_vars[main_group_name]);
    }
    for(auto const &g: groups) if(g.first != main_group_name) {
        if(DEBUG_GENC) {
            std::ofstream outj(output_filename(sfmt() << g.first << "-k8s-yaml-local.json"));
            stru1::to_json(outj, g_group_vars[g.first]);
        }
        auto gg_smap = vex::make_smap(g_group_vars[g.first]);
        vex::expand(out, template_kubernetes_group_yaml, vex::make_cmap(gg_smap, local_smap, global_smap));
    }

    DEBUG_LEAVE;
    return 0;
}

int flow_compiler::genc_deployment_driver(std::string const &deployment_script) {
    int error_count = 0;
    DEBUG_ENTER;
    OFSTREAM_SE(out, deployment_script);
    decltype(global_vars) local_vars;
    error_count += group_info(0, local_vars);

    std::ostringstream yaml;
    error_count += genc_dcs_conf(yaml, local_vars);
    set(local_vars, "DOCKER_COMPOSE_YAML",  yaml.str());
    extern char const *rr_keys_sh;
    set(local_vars, "RR_KEYS_SH", rr_keys_sh);
    extern char const *rr_get_sh;
    yaml.str("");
    vex::expand(yaml, rr_get_sh, vex::make_smap(local_vars));
    set(local_vars, "RR_GET_SH",  yaml.str());
    yaml.str("");
    error_count += genc_k8s_conf(yaml);
    set(local_vars, "KUBERNETES_YAML", yaml.str());

    bool have_volumes = false;
    for(int n: *this)
        if(((at(n).type == FTK_NODE && method_descriptor(n) != nullptr) || at(n).type == FTK_CONTAINER) && group(n) == main_group_name) 
            for(int m: subtree(n)) if(at(m).type == FTK_MOUNT) 
                have_volumes = true;
    if(have_volumes)
        set(local_vars, "HAVE_MAIN_GROUP_VOLUMES", "");

    if(DEBUG_GENC) {
        std::string ofj = deployment_script + "-g.json";
        OFSTREAM_SE(outj, ofj);
        stru1::to_json(outj, global_vars);
    }
    if(DEBUG_GENC) {
        std::string ofj = deployment_script + "-l.json";
        OFSTREAM_SE(outj, ofj);
        stru1::to_json(outj, local_vars);
    }
    extern char const *template_driver_sh; 
    auto local_smap = vex::make_smap(local_vars);
    auto global_smap = vex::make_smap(global_vars);
    vex::expand(out, template_driver_sh, vex::make_cmap(local_smap, global_smap));

    if(error_count == 0) {
        out.close();
        filu::chmodx(deployment_script);
    }
    DEBUG_LEAVE;
    return error_count;
}
