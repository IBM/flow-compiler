#ifndef H_FLOW_COMPILER_H
#define H_FLOW_COMPILER_H

#include <iostream>
#include <vector>
#include <set>
#include <map>

#include <google/protobuf/compiler/importer.h>

using namespace google::protobuf;

#include "grpc-helpers.H"
#include "flow-ast.H"
#include "stru1.H"
#include "helpo.H"
#include "flow-opcodes.H"

typedef std::map<std::string, std::vector<std::string>> var_store_t;
inline static 
var_store_t &append(var_store_t &vars, std::string const &name, std::string const &value) {
    vars[name].push_back(value); return vars;
}
inline static 
var_store_t &set(var_store_t &vars, std::string const &name, std::string const &value) {
    vars[name].clear();
    vars[name].push_back(value);
    return vars;
}
inline static 
var_store_t &clear(var_store_t &vars, std::string const &name) {
    vars[name].clear();
    return vars;
}
inline static 
std::string get(var_store_t const &vars, std::string const &name, int index=0) {
    auto vp = vars.find(name);
    if(vp == vars.end() || vp->second.size() <= index)
        return "";
    return vp->second[index];
}
inline static
std::vector<std::string> const &all(var_store_t &vars, std::string const &name) {
    return vars[name];
}
template <class CITER> inline static
var_store_t &set_all(var_store_t &vars, std::string const &name, CITER begin, CITER end) {
    vars[name].assign(begin, end);
    return vars;
}
/*
struct mount_info {
    int node;               // AST pointer        
    int node_node;          // Node (parent) AST pointer
    std::string name;       // Mount label 
    std::vector<std::string> paths;
    std::string local;      // Local path
    std::string pvc;        // Private volume claim
    std::string cos;        // Cloud object store URL
    std::string secret;     // Secret name for COS or Artifactory credentials
    bool read_only;         // Mount read only 

    mount_info(int n=0, int nn=0, std::string const &na="", bool ro=true): node(n), node_node(nn), name(na), read_only(ro) {
    }
};
*/
class flow_compiler: public flow_ast {
private:    
    FErrorPrinter pcerr;
    compiler::DiskSourceTree source_tree;
    compiler::Importer importer;
    // Command line option dependent grpc compiler commands
    std::string grpccc, protocc;
    // Data
    std::string main_file; 
    std::string input_label; 
    // Orchestrator image name and tag
    std::string orchestrator_image;
    std::string orchestrator_tag;
    bool orchestrator_debug_image;
    bool orchestrator_no_rest;
    std::string main_name;
    std::string main_group_name;
    // Main description, gleaned from comments
    std::string main_description;
    // Orchestrator image name with repository (as it appears in composer or kube config)
    std::string orchestrator_pull_image;
    // Default repository path for all images
    std::string default_repository;
    // Repository to push the orchestrator image to
    std::string push_repository;
    // Image pull secret names (for kube deployment)
    std::set<std::string> image_pull_secrets;
    // Default port values
    int base_port, rest_port;
    // Default timeout values 
    int default_node_timeout;
    int default_entry_timeout;
    // Maximum number of concurrent calls made to a service when 'replicas' is not set
    int default_maxcc;              
    // Label for the runtime - used to select the runtime base image 
    std::string runtime; 
    // Imported proto files
    std::vector<FileDescriptor const *> fdps;
    // Lookup table for enum ids
    std::set<EnumValueDescriptor const *> enum_value_set;
    // Block ast-node flow graph by entry block ast-node
    // for each entry a list of sets of overlapping nodes
    std::map<int, std::vector<std::set<int>>> flow_graph;
    // local vars for each group
    std::map<std::string, var_store_t> group_vars;
    // volume for each group
    //std::map<std::string, std::set<std::string>> group_volumes;
    var_store_t global_vars;
    Descriptor const *input_dp;
    std::vector<fop> icode;
    // Entry point in icode for each entry node
    std::map<int, int> entry_ip;
    // Global table of names that have to be unique
    std::map<std::string, int> glunna;
public: 
    bool trace_on, verbose;
    flow_compiler();
    void set_main_file(std::string const &a_file);
    std::string file_loc(flow_token const &token) const {
        return stru1::sfmt() << "//** "<< main_file << ":" << token.line+1 << ":" << token.column+1 << " ** ";
    }
    std::string file_loc(flow_ast_node const &n) const {
        return file_loc(n.token);
    }
    std::string file_loc(int node) const {
        return file_loc(at(node));
    }
    int add_to_proto_path(std::string const &directory);
    int compile_proto(std::string const &file);

    std::string output_filename(std::string const &filename);
    /**
     * Generate protobuf and grpc support code, by invoking protoc
     */
    int genc_protobuf();
    int genc_grpc();
    /**
     * Find a grpc method by name
     * The method can be spcified as [package.][service.]method
     * The package is either the label defined with the package directive or the basename of the .proto file
     */
    MethodDescriptor const *find_service_method(std::string const &dotted_id, std::set<std::string> *matches=nullptr) const;
    /**
     * Find a grpc message type by name
     * The method can be spcified as [package.]message
     * The package is either the label defined with the package directive or the basename of the .proto file
     */
    Descriptor const *find_message(std::string const &dotted_id, std::set<std::string> *matches=nullptr) const;
    /**
     * Return formatted list of all methods for message display purposes
     */
    std::string format_full_name_methods(std::string const &sep, std::string const &last, std::string const &begin, std::string const &prefix, std::string const &suffix) const;
    /**
     * Return formatted list of all message type names for message display purposes
     */
    std::string format_message_names(std::string const &sep, std::string const &last, std::string const &begin, std::string const &prefix, std::string const &suffix) const;
    /**
     * Preserve all comments that have @text referneces in them
     * and the ones that don't start with punctuation
     */
    void add_comments(std::string const &comment, int token);
    int parse();
    /**
     * Finds a labeled entry in the block
     * Starts searching at position pos
     * Returns 0 if not found
     * Must be called with a valid block node
     */
    int find_in_blck(int block_node, std::string const &name, int *pos=nullptr) const;

    int compile_string(std::string &str, int node, int node_type=FTK_STRING);
    int compile_id(std::string &str, int id_node);
    int compile_method(std::string &method, int mthd_node, int max_components=-1);
    // If exp_node is a valid pointer, the block is expected to contain one output/return definition
    int compile_block(int blck_node, std::set<std::string> const &output_nvn={}, int *exp_node=nullptr);
    MethodDescriptor const *check_method(std::string &method, int error_node);
    Descriptor const *check_message(std::string &dotted_id, int error_node);

    int fixup_node_ids();
    int fixup_proto_refs();
    int compile_if_import(int stmt_node);
    int compile_defines();
    int compile_expressions(int node=0);

    // Compile any basic type, field or enum reference and fldr
    int compile_expression(int node);
    // Compile fldr expressions
    int compile_fldr(int fldr_node);
    int fixup_vt(int node);
    // Compile all the filed mappings - this does not resolve fldx references
    int compile_fldm(int fldm_node, Descriptor const *dp);
    // The following should be called on a second pass (after all nodes have been compiled)
    int compile_fldx(int node);
    // The following should be called on a second pass (after all nodes have been compiled)
    // Fix up node only references
    int compile_node_ref(int id_node);
    // The following should be called on a second pass (after all nodes have been compiled)
    // Fix up node only references
    int compile_exp_id(int node);
    // The following should be called on a second pass (after all nodes have been compiled)
    // Fix up enum references
    int compile_enum(int node, EnumDescriptor const *context=nullptr);
    // Second pass fix up for number of indices needed
    int update_dimensions(int node);
    int update_noderef_dimension(int node);
    int update_noderef_dimension_d(int node);

    // Get all the nodes of this name, in eval order
    //std::vector<int> all_nodes(std::string const &name) const;
    //std::vector<int> all_nodes(int node) const;
    // Get the node nodes referenced by the fldm node
    // void get_fldm_node_refs(std::map<int, std::set<std::string>> &noset, int fldm_node);
    // Get all the node nodes referenced in this conditional expression
    // void get_bexp_node_refs(std::map<int, std::set<std::string>> &noset, int bexp_node);
    /*
    // Get the node referenced by either condition (1), body (2) or both (0) for this blck_node
    std::map<int, std::set<std::string>> &get_node_refs(std::map<int, std::set<std::string>> &noset, int blck_node, int type=0);
    std::map<int, std::set<std::string>> &get_node_condition_refs(std::map<int, std::set<std::string>> &noset, int blck_node) {
        return get_node_refs(noset, blck_node, 1);
    }
    std::map<int, std::set<std::string>> &get_node_body_refs(std::map<int, std::set<std::string>> &noset, int blck_node) {
        return get_node_refs(noset, blck_node, 2);
    }
    */
    int build_flow_graph(int entry_blk_node);

    // Type check if the the assignment left = right
    int check_assign(int error_node, struct lrv_descriptor const &left, struct lrv_descriptor const &right);

    std::string fldx_mname(int fldx_node, int context_dim) const;
    int encode_expression(int fldr_node, int expected_type, int dim_change);
    int populate_message(std::string const &lv_name, struct lrv_descriptor const &lvd, int arg_node, std::map<int, int> &node_ip, int loop_level);

    int get_field_refs(std::set<std::pair<int, int>> &refs, int expr_node, int lv_dim) const;

    int compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &node_stages, std::set<int> const &node_set);
    int fop_compare(fop const &left, fop const &right) const;
    void print_pseudocode(std::ostream &out) const;
    void print_graph(std::ostream &out, int entry=-1);
    int get_blck_timeout(int blck, int default_timeout);
    int get_mount_info(std::map<std::string, std::string> &info, int mntn);

    int gc_server_method(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry);
    int gc_server(std::ostream &out);
    int gc_local_vars(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) const;

    // ID that includes the node name
    std::string node_name_id(int node_n, std::string const &prefix="", std::string const &suffix="") const {
        std::string id = stru1::to_lower(stru1::to_identifier(name(node_n)));
        if(!prefix.empty()) id = prefix + "_" + id;
        if(!suffix.empty()) id = id + "_" + suffix;
        return id;
    }
    std::string node_type_id(int node_n, std::string const &prefix="", std::string const &suffix="") const {
        std::string id = stru1::to_lower(stru1::to_identifier(type(node_n)));
        if(!prefix.empty()) id = prefix + "_" + id;
        if(!suffix.empty()) id = id + "_" + suffix;
        return id;
    }

    // Name generators for case sensitive languagages C, Python, etc.
    // [prefix_]label[_stage][_suffix]                when the label is given
    // [prefix_]I_input label[_stage][_suffix]        for the input node
    // [prefix_]E_methid_name(node)[_stage][_suffix]  for the entry node
    // [prefix_]node_label[_stage][_suffix]           for node nodes      
    std::string cs_name_s(std::string const &prefix, std::string const &label, std::string const &suffix="") const {
        std::ostringstream buf;
        if(!prefix.empty()) buf << prefix << '_';
        buf << label;
        //if(stage > 0) buf << '_' << stage;
        if(!suffix.empty()) buf << '_' << suffix;
        return buf.str();
    }
    std::string get_node_label(int node) const {
        if(node == 0) 
            return std::string("I_") + stru1::to_lower(stru1::to_identifier(input_label)); 
        if(at(node).type == FTK_ENTRY) 
            return std::string("E_") + stru1::to_lower(stru1::to_identifier(method_descriptor(node)->name()));
        return stru1::to_lower(stru1::to_identifier(name(node)));
    }
    std::string cs_name(std::string const &prefix, int node, std::string const &suffix="") const {
        if(node == 0)
            return cs_name_s("RQ", get_node_label(node), suffix);
        return cs_name_s(prefix, get_node_label(node), suffix);
    }
    int compile(std::set<std::string> const &targets);

    // AST accessors to retrieve the first encoutered value, all values in tree order or a map.
    // <required> triggers error when no value is found. An error is also generated if the encoutered values 
    // do not match any of the accepted types.
    int get_block_value(int &value, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);
    int get_block_value(std::vector<int> &values, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);
    int get_nv_block(std::map<std::string, int> &nvs, int parent_block, std::string const &block_label, std::set<int> const &accepted_types);
    // get block value as string -- required
    int get_block_s(std::string &value, int blck, std::string const &name, std::set<int> const &accepted_types);
    int get_block_s(std::string &value, int blck, std::string const &name) {
        return get_block_s(value, blck, name, std::set<int>({FTK_STRING}));
    }
    // get block value as string -- optional
    int get_block_s(std::string &value, int blck, std::string const &name, std::set<int> const &accepted_types, std::string const &def);
    int get_block_s(std::string &value, int blck, std::string const &name,std::string const &def) {
        return get_block_s(value, blck, name, std::set<int>({FTK_STRING}), def);
    }
    // get block value as int -- required
    int get_block_i(int &value, int blck, std::string const &name);
    // get block value as int -- optional
    int get_block_i(int &value, int blck, std::string const &name, int def);
    // Code generation for:
    // C++ client
    int genc_client_source(std::string const &client_src);
    // C++ server
    int genc_server_source(std::string const &server_src);
    // Kubernetes deployment yaml (pods and services).
    int genc_k8s_conf(std::ostream &out);
    // Docker Composer and Swarm yaml
    int genc_dcs_conf(std::ostream &out, std::map<std::string, std::vector<std::string>> &local_vars);
    // Bash driver for deploying services - include both Kubernetes and Docker deployment configuration files.
    int genc_deployment_driver(std::ostream &outs, std::map<std::string, std::vector<std::string>> &local_vars);
    int genc_deployment_driver(std::string const &deployment_script); 
    // Grab all deployment related  node information
    int node_info(int node, std::map<std::string, std::vector<std::string>> &local_vars);
    // Create environment for node <n>
    int get_environment(std::vector<std::pair<std::string, std::string>> &env, int n, std::map<int, std::string> const &node_port,  bool group_mode = false);
    
    // Webapp interface
    int genc_www();
    // DOT and SVG files
    int genc_graph(bool gen_svgs);
    // Makefile
    int genc_makefile(std::string const &filename);
    int genc_dockerfile(std::string const &orchestrator_name);
    int genc_python_client(std::string const &client_bin);

    // Set code generation variables - all return the number of errors generated
    // Entry related info
    int set_entry_vars(decltype(global_vars) &vars);
    int set_cli_node_vars(decltype(global_vars) &vars);
    int set_cli_active_node_vars(decltype(global_vars) &vars, int cli_node);

    // Main call sequence
    int process(std::string const &input_filename, std::string const &orchestrator_name, std::set<std::string> const &targets, struct helpo::opts const &opts);

    static bool debug_enable;

    int get_unna(std::string &name, int hint=1);
};



#ifndef DEBUG_GENC
#define DEBUG_GENC flow_compiler::debug_enable
#endif

#define DEBUG_ENTER if(DEBUG_GENC) std::cerr << "----- enter " << __func__ << " "  << error_count << ", errors\n"
#define DEBUG_CHECKX(c) if(DEBUG_GENC) std::cerr << "----- check " << (c) << " " << __func__ << " "  << error_count << ", errors\n"
#define DEBUG_CHECK(c) if(DEBUG_GENC) std::cerr << "----- check " << std::string(sfmt() << c) << " " << __func__ << " "  << error_count << ", errors\n"
#define DEBUG_LEAVE if(DEBUG_GENC) std::cerr << "----- leave " << __func__ << " "  << error_count << ", errors\n"
#define OFSTREAM_SE(ofs, s)  std::ofstream ofs(s.c_str()); if(DEBUG_GENC) std::cerr << "write " << s << ", " << ofs.is_open() << "\n"; if(!ofs.is_open()) { ++error_count; pcerr.AddError(s, -1, 0, "cannont write file"); }

#endif
