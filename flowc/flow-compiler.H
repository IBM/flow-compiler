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

struct node_info {
    int node;               // AST pointer
    std::string xname;      // Node or container name, derived from but not always the same as name(node)
    std::string id;         // Node id as decalred in the flow stanza
    int port;               // Assigned port for this service
    std::string group;      // Group label for this node
    std::string runtime;    // Runtime label for Docker
    int order;              // Order in which the nodes are evaluated, 0 is last
    bool no_call;           // If true, there is no gRPC service associated with this node
                            // Extra headers to be set as metadata 
    std::map<std::string, std::string> headers;
                            // External endpoint for this service 
    std::string external_endpoint;
    std::string image_name; // Docker tag for the image
    std::vector<int> mounts;
    std::map<std::string, std::string> environment;
    int scale;              // Number of images or groups to run at the same time

    int min_cpus, max_cpus; // CPU limits 
    int min_gpus, max_gpus; // GPU limits 
    string min_memory, max_memory; // memory limits 

    node_info(int no=0, std::string const &na="", bool nc=false):node(no), xname(na), port(0), order(0), no_call(nc), scale(0), min_cpus(0), max_cpus(0), min_gpus(0), max_gpus(0) {
    }
    std::string label() const {
        return stru1::to_lower(stru1::to_identifier(xname));
    }
};
inline static std::ostream &operator<< (std::ostream &out, node_info const &n) {
    out << "(@" << n.node << ": " << n.xname << " id: " << n.id << " group: " << n.group << " port: " << n.port << " order: " << n.order << " grpc: " << (n.no_call? "no": "yes") << ")";
    return out;
};

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

class flow_compiler: public flow_ast {
private:    
    FErrorPrinter pcerr;
    compiler::DiskSourceTree source_tree;
    compiler::Importer importer;
    // Command line option dependent grpc compiler command
    std::string grpccc;
    // Data
    std::string main_file; 
    std::string package_name, input_label; 
    // Orchestrator image name and tag
    std::string orchestrator_image;
    std::string orchestrator_tag;
    bool orchestrator_debug_image;
    std::string main_name;
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
    // Block data: a set of name value pairs in source file order
    // values are AST node ids of the value node
    typedef std::vector<std::pair<std::string, int>> block_data_t;
    // All blocks indexed by AST node
    std::map<int, block_data_t> block_store;
    // Table with nodes, containers and entries: names[name]: (type, block ast-node)
    std::map<std::string, std::pair<std::string, int>> named_blocks_w;
    decltype(named_blocks_w) const &named_blocks;
    std::set<int> node_set;
    std::set<int> entry_set;
    std::set<int> container_set;

    // Block ast-node flow graph by entry block ast-node
    // for each entry a list of sets of overlapping nodes
    std::map<int, std::vector<std::set<int>>> flow_graph;
    // All the referenced (used) nodes and the corresponding data
    std::map<int, node_info> referenced_nodes;
    // local vars for each group
    std::map<std::string, var_store_t> group_vars;
    // volume for each group
    std::map<std::string, std::set<std::string>> group_volumes;
    // All the volume names that need to be mounted
    //std::set<std::string> volumes;
    // All mount records indexed by AST node
    std::map<int, mount_info> mounts;
    // 
    var_store_t global_vars;

    Descriptor const *input_dp;
    std::vector<fop> icode;
    // Entry point in icode for each entry node
    std::map<int, int> entry_ip;
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
    MethodDescriptor const *get_method_descriptor_for_block(int blck_node);
    MethodDescriptor const *get_method_descriptor_for_node(std::string const &node);

    int get_fldm_for_blck(int blck_node) const;

    int compile_string(std::string &str, int node, int node_type=FTK_STRING);
    int compile_id(std::string &str, int id_node);
    int compile_method(std::string &method, int mthd_node, int max_components=-1);
    // If exp_node is a valid pointer, the block is expected to contain one output/return definition
    int compile_block(int blck_node, std::set<std::string> const &output_nvn={}, int *exp_node=nullptr);
    MethodDescriptor const *check_method(std::string &method, int error_node);
    Descriptor const *check_message(std::string &dotted_id, int error_node);
    int compile_if_import(int stmt_node);
    int compile_stmt(int stmt_node);
    // Compile function calls - fldx are not resolved 
    int compile_fldr(int fldr_node, FieldDescriptor const *left_dp, int left_type, int left_dim);
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
    // Fix up ID references
    int compile_id_ref(int node);
    // Second pass fix up for number of indices needed
    int update_dimensions(int node);
    int update_noderef_dimension(int node);

    // Get all the nodes of this name, in eval order
    std::vector<int> all_nodes(std::string const &name) const;
    std::vector<int> all_nodes(int node) const;
    // Get the node nodes referenced by the fldm node
    void get_fldm_node_refs(std::map<int, std::set<std::string>> &noset, int fldm_node);
    // Get all the node nodes referenced in this conditional expression
    void get_bexp_node_refs(std::map<int, std::set<std::string>> &noset, int bexp_node);
    // Get the node referenced by either FTK_oexp, FTK_bexp or both (0) for this blck_node
    std::map<int, std::set<std::string>> &get_node_refs(std::map<int, std::set<std::string>> &noset, int blck_node, int type=0);
    // Search a blk node for a rexp or oexp entry and return the arg node
    int get_arg_node(int blck_node) const;
    int build_flow_graph(int entry_blk_node);

    // Type check if the the assignment left = right
    int check_assign(int error_node, struct lrv_descriptor const &left, struct lrv_descriptor const &right);

    int populate_message(std::string const &lv_name, struct lrv_descriptor const &lvd, int arg_node, std::map<int, int> &node_ip);
    int compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &node_stages, std::set<int> const &node_set);
    int fop_compare(fop const &left, fop const &right) const;
    void dump_code(std::ostream &out) const;
    void print_graph(std::ostream &out, int entry=-1);
    int get_blck_timeout(int blck, int default_timeout);

    // Code generation for orchestrator server
    class stru1::indented_stream &gc_bexp(class stru1::indented_stream &out, std::map<std::string, std::string> const &generated_nodes, struct accessor_info const &rs_dims, int bexp, int op) const;
    int gc_server_method(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry);
    int gc_server(std::ostream &out);
    int gc_local_vars(std::ostream &out, std::string const &entry_dot_name, std::string const &entry_name, int blck_entry) const;

    // ID that includes the node name
    std::string node_name_id(int node_n, std::string const &prefix="", std::string const &suffix="") const {
        std::string id = stru1::to_lower(stru1::to_identifier(referenced_nodes.find(node_n)->second.xname));
        if(!prefix.empty()) id = prefix + "_" + id;
        if(!suffix.empty()) id = id + "_" + suffix;
        return id;
    }
    std::string node_type_id(int node_n, std::string const &prefix="", std::string const &suffix="") const {
        std::string id = stru1::to_lower(stru1::to_identifier(name(node_n)));
        if(!prefix.empty()) id = prefix + "_" + id;
        if(!suffix.empty()) id = id + "_" + suffix;
        return id;
    }

    // Name generators for case sensitive languagages C, Python, etc.
    // [prefix_]label[_stage][_suffix]                when the label is given
    // [prefix_]I_input label[_stage][_suffix]        for the input node
    // [prefix_]E_methid_name(node)[_stage][_suffix]  for the entry node
    // [prefix_]node_label[_stage][_suffix]           for node nodes      
    std::string cs_name(std::string const &prefix, std::string const &label, std::string const &suffix="") const {
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
        if(type(node) == "entry") 
            return std::string("E_") + stru1::to_lower(stru1::to_identifier(method_descriptor(node)->name()));
        return referenced_nodes.find(node)->second.label();
    }
    std::string cs_name(std::string const &prefix, int node, std::string const &suffix="") const {
        if(node == 0)
            return cs_name("RQ", get_node_label(node), suffix);
        return cs_name(prefix, get_node_label(node), suffix);
    }


    int compile(std::set<std::string> const &targets);

    int genc_client(std::ostream &out);
    int get_block_value(std::vector<int> &values, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);
    int get_block_value(int &value, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);

    int get_nv_block(std::map<std::string, std::string> &nvs, int parent_block, std::string const &block_label, std::set<int> const &accepted_types);

    int genc_kube(std::ostream &out);
    int genc_composer(std::ostream &out, std::map<std::string, std::vector<std::string>> &local_vars);
    int genc_deployment_driver(std::ostream &outs, std::map<std::string, std::vector<std::string>> &local_vars);

    // Set code generation variables - all return the number of errors generated
    
    // Entry related info
    int set_entry_vars(decltype(global_vars) &vars);
    int set_cli_node_vars(decltype(global_vars) &vars);
    int set_cli_active_node_vars(decltype(global_vars) &vars, int cli_node);

    // Main call sequence
    int process(std::string const &input_filename, std::string const &orchestrator_name, std::set<std::string> const &targets, struct helpo::opts const &opts);
};

#endif
