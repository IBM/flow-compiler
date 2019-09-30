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

using namespace stru1;

#define INDENT std::string((indent_size), ' ')
#define TRACE if(trace_on) std::cerr

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

enum op { 
    COMM,           // comment 
    MTHD,           // method begin
    END,            // method end
    BSTG,           // stage begin
    ESTG,           // stage end
    BNOD,           // node begin
    NSET,           // node setup
    CHK,            // check condition
    ENOD,           // node end
    BPRP,           // result preparation begin
    EPRP,           // result preparation end
    LOOP,           // begin loop
    ELP,            // end loop
    SET,            // set field with another field
    SETI,           // set field with integer constant
    SETS,           // set field with string constant
    SETE,           // set field with enum constant
    SETF,           // set field with float constant
    COPY,           // copy field
    CALL,           // call method
    INDX            // index data
};
struct fop {
    op code;
    std::string arg1, arg2;
    Descriptor const *d1 = nullptr;
    Descriptor const *d2 = nullptr;
    MethodDescriptor const *m1 = nullptr;
    std::vector<int> arg;
    fop(op c=COMM): code(c) {}
    fop(op c, std::string const &a1, std::string const &a2=""): code(c), arg1(a1), arg2(a2) {}
    fop(op c, std::string const &a1, std::string const &a2, Descriptor const *ad1, Descriptor const *ad2=nullptr): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2) {}
    fop(op c, std::string const &a1, Descriptor const *ad1, int i): code(c), arg1(a1), d1(ad1)  {arg.push_back(i);}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1): code(c), arg1(a1), arg2(a2), m1(am1) {}
    fop(op c, std::string const &a1, std::string const &a2, MethodDescriptor const *am1, Descriptor const *ad1, Descriptor const *ad2): code(c), arg1(a1), arg2(a2), d1(ad1), d2(ad2), m1(am1) {}
    fop(op c, int i): code(c) { arg.push_back(i); }
    fop(op c, int i, int j): code(c) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j); }
    fop(op c, std::string const &a1, int i, int j, int k): code(c), arg1(a1) { arg.push_back(i); arg.push_back(j);  arg.push_back(k); }
};
std::ostream &operator<< (std::ostream &out, fop const &fop);

struct node_info {
    int node;
    std::string name;
    int port;
    std::string group;
    std::string runtime;
    int order;              // Order in which the nodes are evaluated, 0 is last
    std::string external_endpoint;
    std::string image_name;
    std::vector<int> mounts;
    // Environment 
    std::map<std::string, std::string> environment;

    node_info(int no=0, std::string const &na="", int po=0, int o=0):node(no), name(na), port(po), order(o) {
    }
    std::string label() const {
        return to_lower(to_identifier(name));
    }
};
inline static std::ostream &operator<< (std::ostream &out, node_info const &n) {
    out << "(@" << n.node << ": " << n.name << " group: " << n.group << " port: " << n.port << " order: " << n.order << ")";
    return out;
};

struct mount_info {
    int node;
    int node_node;
    std::string name;
    std::vector<std::string> paths;
    std::string local;
    std::string pvc;
    std::string cos;
    std::string artifactory;
    std::string secret;
    bool read_only;

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
    std::string rest_image, rest_node_label;
    // Orchestrator image name and tag
    std::string orchestrator_image;
    std::string orchestrator_tag;
    // Orchestrator image name with repository (as it appears in composer or kube config)
    std::string orchestrator_pull_image;
    // Default repository path for all images
    std::string default_repository;
    // Repository to push the orchestrator image to
    std::string push_repository;
    // Image pull secret names (for kube deployment)
    std::set<std::string> image_pull_secrets;
    // Default port values
    int base_port, rest_image_port, gui_image_port, custom_gui_image_port;
    // Default timeout values 
    int default_node_timeout;
    int default_entry_timeout;
    // Maximum number of concurrent calls made to a service when 'replicas' is not set
    int default_maxcc;              
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
    std::map<std::string, std::pair<std::string, int>> names;

    // block ast-node flow graph by entry block ast-node
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
        return sfmt() << "//** "<< main_file << ":" << token.line+1 << ":" << token.column+1 << " ** ";
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
    * The method can be spcified as [package.][service.]method_name
    * The package is either the label defined with the package directive or the basename of the .proto file
    */
    MethodDescriptor const *find_service_method(std::string const &method_name, std::set<std::string> *matches=nullptr) const;
    /**
     * Return formatted list of all methods for message display purposes
     */
    std::string format_full_name_methods(std::string const &sep, std::string const &last, std::string const &begin, std::string const &prefix, std::string const &suffix) const;
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
    int compile_block(int blck_node, std::string const &output_nvn="", int *exp_node=nullptr);
    MethodDescriptor const *check_method(std::string &method, int error_node);
    int compile_if_import(int stmt_node);
    int compile_stmt(int stmt_node);
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

    // Get all the nodes of this name, in eval order
    std::vector<int> all_nodes(std::string const &name) const;
    std::vector<int> all_nodes(int node) const;
    // Get the node nodes referenced by the fldm node
    void get_fldm_node_refs(std::set<int> &noset, int fdlm_node, bool include_input=false);
    void get_fldm_node_refs(std::map<int, std::set<std::string>> &noset, int fldm_node, bool include_input=false);
    // Get all the node nodes referenced in this conditional expression
    void get_bexp_node_refs(std::set<int> &noset, int bexp_node, bool include_input=false);
    void get_bexp_node_refs(std::map<int, std::set<std::string>> &noset, int bexp_node, bool include_input=false);
    // Get the node referenced by either FTK_oexp, FTK_bexp or both (0) for this blck_node
    std::set<int> &get_node_refs_o(std::set<int> &noset, int blck_node, int type=0, bool include_input=false);
    std::map<int, std::set<std::string>> &get_node_refs(std::map<int, std::set<std::string>> &noset, int blck_node, int type=0, bool include_input=false);
    // Search a blk node for a rexp or oexp entry and return the arg node
    int get_arg_node(int blck_node) const;
    int build_flow_graph(int entry_blk_node);

    int check_assign(int node, Descriptor const *left, Descriptor const *right);
    int check_assign(int node, Descriptor const *left, FieldDescriptor const *right);
    int check_assign(int node, FieldDescriptor const *left, Descriptor const *right);
    int check_assign(int node, FieldDescriptor const *left, FieldDescriptor const *right);
    int find_max_index_depth(int right_value_node, std::map<int, int> const &node_ip, int max_allowed);
    int populate_message(std::string const &lv_name, Descriptor const *lvd, FieldDescriptor const *lvfd, std::map<int, std::pair<std::string, Descriptor const *>> const &locals, int arg_node, std::map<int, int> &node_ip);
    int compile_flow_graph(int entry_blck_node, std::vector<std::set<int>> const &node_stages, std::set<int> const &node_set);
    int fop_compare(fop const &left, fop const &right) const;
    void dump_code(std::ostream &out) const;
    void print_graph(std::ostream &out, int entry=-1);
    int get_blck_timeout(int blck, int default_timeout);

    // Code generation for orchestrator server
    std::ostream &gc_bexp(std::ostream &out, std::map<std::string, std::string> const &generated_nodes, std::map<std::string, int> const &rs_dims, int bexp, int op) const;
    int gc_server_method(std::ostream &out, std::string const &entry_dot_name, int blck_entry);
    int gc_server(std::ostream &out);

    // Name generators for case sensitive languagages C, Python, etc.
    // [prefix_]label[_stage][_suffix]                when the label is given
    // [prefix_]I_input label[_stage][_suffix]        for the input node
    // [prefix_]E_methid_name(node)[_stage][_suffix]  for the entry node
    // [prefix_]node_label[_stage][_suffix]           for node nodes      
    std::string cs_name(std::string const &prefix, std::string const &label, int stage=0, std::string const &suffix="") const {
        std::ostringstream buf;
        if(!prefix.empty()) buf << prefix << '_';
        buf << label;
        //if(stage > 0) buf << '_' << stage;
        if(!suffix.empty()) buf << '_' << suffix;
        return buf.str();
    }
    std::string get_node_label(int node) const {
        if(node == 0) 
            return std::string("I_") + to_lower(to_identifier(input_label)); 
        if(type(node) == "entry") 
            return std::string("E_") + to_lower(to_identifier(method_descriptor(node)->name()));
        return referenced_nodes.find(node)->second.label();
    }
    std::string cs_name(std::string const &prefix, int node, int stage=0, std::string const &suffix="") const {
        return cs_name(prefix, get_node_label(node), stage, suffix);
    }

    int compile(std::set<std::string> const &targets);

    int genc_client(std::ostream &out);
    int get_block_value(std::vector<int> &values, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);
    int get_block_value(int &value, int blck, std::string const &name, bool required, std::set<int> const &accepted_types);
    int genc_kube(std::ostream &out);
    int genc_composer(std::ostream &out);
    int genc_composer_driver(std::ostream &outs, std::string const &composer_yaml);
    int genc_kube_driver(std::ostream &outs, std::string const &kubernetes_yaml);

    // Main call sequence
    int process(std::string const &input_filename, std::string const &orchestrator_name, std::set<std::string> const &targets, helpo::opts const &opts);
};

#endif
