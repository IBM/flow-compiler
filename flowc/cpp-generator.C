#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>

#include "flow-comp.H"
#include "stru.H"
#include "ind-stream.H"

#include <iostream>
namespace {
    struct expression;
}
static std::ostream &operator << (std::ostream &out, expression const &x);
namespace {

enum event_type {
    ev_initiate,
    ev_condition, 
    ev_setup, 
    ev_ready,
    ev_available,
    ev_done,
    ev_check_flags,
};

std::map<event_type, std::string> event_prefix = {
    {ev_initiate, "ea_init" },
    {ev_condition, "eb_cond" },
    {ev_setup, "ec_setup" },
    {ev_ready, "ed_ready" },
    {ev_available, "ef_available" },
    {ev_done, "eg_done" },
    {ev_check_flags, "eh_check_flags" },
};

/** label generators 
 */
std::string ev(event_type e, std::string label) {
    return label.empty()? event_prefix[e]:
        event_prefix[e] + "_" + label;
}
std::set<std::string> ev(event_type e, std::set<std::string> labels) {
    std::set<std::string> r;
    for(auto label: labels) 
        r.insert(ev(e, label));
    return r;
}
std::string cv(std::string label) {
    return std::string("c_") + label;
}
std::string rx(std::string label, int node) {
    return stru::sfmt() << "rx_" << label << "_" << node;
}
struct expression {
    std::string src;
    //std::set<std::pair<std::string, int>> fam_refs;
    std::map<int, std::string> fam_refs;
    std::string unid;
    std::string family;
    int node;
    char type;

    expression(char t, int n, std::string f, std::string u):
        unid(u), family(f), node(n), type(t) {}
};
struct cpp_gen {
    // the AST
    fc::compiler const &ast;
    // families referenced only by other nodes
    std::set<std::string> dep_families;
    // all event names
    std::set<std::string> events;
    std::set<std::string> process;
    // events and their dependent variables
    std::map<std::string, std::set<std::string>> triggers;
    // input node name
    std::string input_name;
    // process info for each event
    std::map<std::string, std::vector<std::string>> cases;
    // process info for the entry 
    std::vector<std::string> entry;
    // node families that need to evaluate a condition expression
    std::set<std::string> conditions;
    // right expressions
    std::map<std::string, std::string> rexpr;
    // message set expressions
    std::map<std::string, std::string> mexpr;
    // flag variable names for each node 
    std::set<std::string> flagvars;
    // node family processing order
    std::vector<std::string> family_process_order;
    // node family size (dimension)
    std::map<std::string, int> family_dim;
    // all expressions
    std::vector<expression> exprs;

    cpp_gen(fc::compiler const &ast, std::string input_name);
    /***
     * Generate the code block for a condition for a <node> in <family>.
     * Return the event name where processing can continue.
     */
    std::string condition_event(std::string family, int node, std::string body_event);
    /***
     * Generate the code block for a <node> in <family>.
     * Return the event name where processing can continue.
     */
    std::string node_event(std::string family, int node, std::string body_event);
    /***
     * Generate the code for the entire family and recurse for its dependents.
     * Return the number of families processed.
     */
    int node_family(std::string family);
    /** Generate the code for an <entry> node.
     * Return the eror_count.
     */
    int entry_body(int entry);
    /***
     * Generate a right expession method <name> at valx <node> 
     */
    std::string conditional_expr_method(int node, std::string name);
    int status_expr_method(int node, std::string name);
    int mexpr_method(int node, std::string name);
    //std::set<std::pair<std::string, int>> ienfref;
    std::map<int, std::string> ienfref;
    void inline_expr(std::ostringstream &out, int valx_node, int precedence);
    std::string inline_expr(int valx_node);
    std::string field_reference(int ndid_node);
    int checkset_dim2(int left_base_dim, int node);
    int checkset_dim(int left_base_dim, int node);

    // keep track of the number of errors generated
    int error_count;
};

bool check_assign(int left_dim, fc::value_type left, fc::value_type right) {
    bool can_assign =  right.can_assign_to(fc::value_type(left_dim, left));
    if(!can_assign) {
        std::cerr << "CANNOT ASSIGN: " << right << "\n";
        std::cerr << "           TO: " << left << "\n";
    }
    return can_assign;
}

cpp_gen::cpp_gen(fc::compiler const &a_ast, std::string a_input_name): 
    ast(a_ast), input_name(a_input_name), error_count(0) {
    family_dim[input_name] = 0;
}
int cpp_gen::checkset_dim(int left_base_dim, int valx_node) {
    std::cerr << "AT " << valx_node << " x " << left_base_dim << " ";
    int dim = left_base_dim;
    switch(ast.atc(valx_node, 0).type) {
        case FTK_msgexp:
            std::cerr << "got msgexp\n";
            // valx's vtype has the expected type for this message.
            // It was either explicitly set or deducted from the node/entry type.
            std::cerr << "LVT: " << ast.vtype(valx_node) << "\n";
            break;
        case FTK_ndid:
            std::cerr << "got ndid -- node alias -- need to check all node families\n";
            std::cerr << "LVT: " << ast.vtype(valx_node) << "\n";
            std::cerr << "The family is ***" << ast.node_text(ast.path(valx_node, "cc", 0, 0)) << "***\n";
            std::cerr << "The family is ***+ " << ast.get("0/0", valx_node) << " +***\n";
            std::cerr << "The family is ***" << ast.node_text(ast.getf("0/0", valx_node)) << "***\n";
            for(int fam_node: ast.node_family(ast.node_text(ast.path(valx_node, "cc", 0, 0)))) {
                bool can_assign = check_assign(left_base_dim, ast.vtype(valx_node), ast.vtype(fam_node));
                std::cerr << "int dim = checkset_dim("<< left_base_dim << ", node="<<  fam_node << ")\n";
                // one and only one action node
                // TODO deal with ERROR
                int action_valx = 0;
                for(int a: ast.get("//(RETURN|OUTPUT)/valx", fam_node)) {
                    action_valx = a; break;
                }
                assert(action_valx != 0);
                std::cerr << "int dim = checkset_dim("<< left_base_dim << ", "<<  action_valx << ")\n";
                dim = checkset_dim(left_base_dim, action_valx);
                std::cerr << "GOT DIM " << dim << " CAN ASSIGN? " << (can_assign? "YES": "NO") << "\n";
            }
            break;
        default:
            std::cerr << "hit default with " << ast.tk_to_string(ast.atc(valx_node, 0).type);
    }
    std::cerr << "\n";
    return dim;
}
int cpp_gen::checkset_dim2(int left_base_dim, int valx_node) {
    std::cerr << "entering checkset_dim(base_dim: " << left_base_dim << ", valx_node: " << valx_node << ") >>> {\n";
    int node_dim = -1; 
    bool done = true;
    switch(ast.atc(valx_node, 0).type) {
        case FTK_msgexp:
            done = false;
            break;
        case FTK_ndid:  // node alias
            if(ast.atc(valx_node, 0).children.size() == 1) {
                std::cerr << "node alias case\n";
                std::string family = ast.node_text(ast.child(ast.child(valx_node, 0), 0));
                if(family_dim.find(family) != family_dim.end()) { 
                    node_dim = family_dim.find(family)->second;
                    if(node_dim != left_base_dim) {
                        std::cerr << "******* bubu node_dim " << node_dim << " is not the expected " << left_base_dim << "!\n";
                    }
                } else {
                    family_dim[family] = left_base_dim;
                    for(int node_node: ast.node_family(family)) {
                        std::cerr << "getting action for node " << node_node << "\n";
                        for(int action_node: ast.get("//(ERRCHK|RETURN|OUTPUT)", node_node)) 
                            for(int vx_node: ast.at(action_node).children) {
                                int a_node_dim = checkset_dim(left_base_dim, vx_node);
                                if(node_dim == -1) {
                                    node_dim = a_node_dim;
                                } else if(a_node_dim != node_dim) {
                                    // mismatch
                                    ++error_count;
                                }
                            }
                    }
                }
                break;
            }
            // fall through if this is actually a field expression
        default:
            
            ++error_count;
            std::cerr << "ooopsy at node " << valx_node << "!\n";
            break;
    }
    if(!done) {
        auto type = ast.vtype(valx_node);
        int list_node = ast.last_child(ast.first_child(valx_node));
        std::cerr << "msgexp case, list node " << list_node << "\n";
        std::cerr << "setting to type " << type << "\n";
        for(int fassgn_node: ast.at(list_node).children) {
            auto field_name = ast.node_text(ast.first_child(fassgn_node));
            auto field_type = type.field_type(field_name);
            int left_dim = left_base_dim + field_type.dimension();
            std::cerr << "assign " << fassgn_node << " " << field_name << " of " << field_type;
            std::cerr << "\n";
            int xpr_dim = checkset_dim(left_dim, ast.last_child(fassgn_node));

        }
    }
    std::cerr << "exiting  checkset_dim(base_dim: " << left_base_dim << ", valx_node: " << valx_node << "): " << node_dim << " <<< }\n";
    return node_dim;
}
int cpp_gen::entry_body(int entry_node) {
    // it was already verified that there is one and only one RETURN/valx subsequence
    int en_rvalx = ast.get("//RETURN/valx", entry_node)[0];
    auto refd_nf = ast.get_referenced_families(en_rvalx);
    refd_nf.erase(input_name);

    checkset_dim(0, en_rvalx);
    if(refd_nf.size() == 0) {
        // result doesn't depend on any nodes
        entry.push_back(stru::sfmt() << "return mrx_" << en_rvalx << "();");
    } else {
        // check availability of nodes, initiate processing if needed
        for(auto nf: refd_nf) {
            auto event_i = ev(ev_initiate, nf);
            entry.push_back(stru::sfmt() << "evq.push(" << event_i << ");");
            node_family(nf);
        }
        auto event_d = ev(ev_done, "");
        events.insert(event_d);
        process.insert(event_d);
        triggers[event_d] = ev(ev_available, refd_nf);
        cases[event_d].push_back(stru::sfmt() << "return mrx_" << en_rvalx << "();");
    }
    for(auto &te: triggers) {
        events.insert(te.first);
        flagvars.insert(te.second.begin(), te.second.end());
    }
    if(triggers.size() > 0)
        events.insert(ev(ev_check_flags, ""));
    return error_count;
}
std::string cpp_gen::condition_event(std::string family, int node, std::string body_event) {
    auto unid = ast.unid(node);
    auto *body = &cases[body_event];
    conditions.insert(family);
    auto event_s = ev(ev_setup, unid);
    events.insert(event_s);
    int n_cvalx = ast.last_child(node);

    // if the current conditional requires other nodes to be evaluated
    // add them to the dependency list and continue processing when available
    if(ast.const_level(n_cvalx) == 0) { 
        // otherwise get the dependent nodes and add them to the intiate list
        auto refd_nf = ast.get_referenced_families(n_cvalx);
        refd_nf.erase(input_name);
        for(auto nf: refd_nf) { 
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_initiate, nf) << ");");
            dep_families.insert(nf);
        }
        // create an event triggered when all the dependents have been gathered and 
        // continue processing there
        if(refd_nf.size() > 0) {
            auto event_c = ev(ev_condition, unid);
            body->push_back(stru::sfmt() << "f_" << event_c << " = true;");
            events.insert(event_c);
            triggers[event_c] = ev(ev_available, refd_nf);
            triggers[event_c].insert(event_c);
            process.insert(event_c);
            body_event = event_c;

            body = &cases[body_event];
        }
    }
    // the current event condition can be evaluated now
    auto cond_var_name = cv(family);
    auto cond_xpr_label = rx(unid, n_cvalx);
    exprs.push_back(expression('c', node, family, unid));
    exprs.back().src = conditional_expr_method(n_cvalx, cond_xpr_label);
    exprs.back().fam_refs = ienfref;
    body->push_back(stru::sfmt() << cond_var_name << " = c" << cond_xpr_label << "()? " << n_cvalx << ": 0;");
    body->push_back(stru::sfmt() << "if(" << cond_var_name << " != 0) {\n    evq.push(" << event_s << ");\n    break;\n}");
    return body_event;
}
std::string cpp_gen::node_event(std::string family, int node, std::string body_event) {
    auto unid = ast.unid(node);
    auto *body = &cases[body_event];

    process.insert(body_event);
    events.insert(body_event);

    std::set<std::string> refd_nf;
    int action_node = 0;
    for(int an: ast.get("//(ERRCHK|RETURN|OUTPUT)", node)) {
        for(int vx: ast.at(an).children) {
            auto rn = ast.get_referenced_families(vx);
            refd_nf.insert(rn.begin(), rn.end());
        }
        refd_nf.erase(input_name);
        action_node = an;
    }
    if(refd_nf.size() > 0) {
        for(auto nf: refd_nf) {
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_initiate, nf) << ");");
            dep_families.insert(nf);
        }
        auto event_r = ev(ev_ready, unid);
        body->push_back(stru::sfmt() << "f_" << event_r << " = true;");
        events.insert(event_r);
        process.insert(event_r);
        triggers[event_r] = ev(ev_available, refd_nf);
        triggers[event_r].insert(event_r);
        body_event = event_r;
        body = &cases[body_event];
    }
    auto event_a = ev(ev_available, family);
    auto msgset_xpr_label = rx(unid, action_node);

    switch(ast.node_type(action_node)) {
        case FTK_OUTPUT:
            mexpr_method(action_node, msgset_xpr_label);
            body->push_back(stru::sfmt() << "auto r_" << unid << " = mrx_" << unid << "_" << action_node << "();");
            body->push_back(stru::sfmt() << "queue_call(r_" << unid << ", f_" << event_a << ");");
            break;
        case FTK_RETURN:
            mexpr_method(action_node, msgset_xpr_label);
            body->push_back(stru::sfmt() << "auto r_" << unid << " = mrx_" << unid << "_" << action_node << "();");
            body->push_back(stru::sfmt() << "f_" << event_a << " = true;");
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_check_flags, "") << ");");
            break;
        case FTK_ERRCHK:
            body->push_back(stru::sfmt() << "reactor->Finish(erx_" << unid << "());");
            break;
    }
    return body_event;
}
int cpp_gen::node_family(std::string family) {
    // return if we already processed this family
    if(std::find(family_process_order.begin(), family_process_order.end(), family) != family_process_order.end()) 
        return 0;

    int fam_count = 1;
    family_process_order.push_back(family);

    // get all the nodes in this family in tree order
    auto is_family = [this, &family](int n) { return ast.node_text(ast.child(n, 0)) == family; };
    std::vector<int> nodes  = ast.getp("//flow/NODE", is_family);

    auto event_i = ev(ev_initiate, family);
    auto event_a = ev(ev_available, family);
    events.insert(event_i);
    process.insert(event_i);
    std::string body_event = event_i;
    auto *body = &cases[body_event];

    // allow for events to be requested more than once, but process only once. 
    body->push_back(stru::sfmt() << "if(f_" << event_i << ") break;");
    body->push_back(stru::sfmt() << "f_" << event_i << " = true;");
    body->push_back(stru::sfmt() << "/** will do " << nodes << " */");

    int def_n = 0, cond_events = 0;
    for(int n: nodes) if(ast.node_type(ast.last_child(n)) == FTK_valx) {
        // process any condition for the node
        body->push_back(stru::sfmt() << "/** condition for " << n << " */");
        
        std::string prev_body_event = body_event;
        body_event = condition_event(family, n, body_event);
        if(prev_body_event != body_event) {
            ++cond_events;
            body = &cases[body_event];
        }
    } else {
        // remember the node without condition (can be at most one)
        def_n = n;
    }
    if(def_n != 0) {
        if(cond_events) {
            events.insert(ev(ev_setup, ast.unid(def_n)));
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_setup, ast.unid(def_n)) << ");");
        }
    } else {
        body->push_back(stru::sfmt() << "f_" << event_a << " = true;");
        body->push_back(stru::sfmt() << "evq.push(" << ev(ev_check_flags, "") << ");");
        process.insert(event_i);
    }
    // process the default node first
    for(int pass = 1; pass < 3; ++pass) for(int n: nodes) if(pass == 1 && n == def_n || pass == 2 && n != def_n) {
        if(pass != 1) 
            body_event = ev(ev_setup, ast.unid(n));
        body_event = node_event(family, n, body_event);
    }
    // process the dependents before returning
    for(auto nf: dep_families) 
        fam_count += node_family(nf);
        
    return fam_count;
}
std::string cpp_gen::inline_expr(int valx_node) {
    std::ostringstream out;
    inline_expr(out, valx_node, 16);
    return std::move(out).str();
}
void cpp_gen::inline_expr(std::ostringstream &out, int valx_node, int precedence) {
    int op_node = ast.first_child(valx_node);
    int op_precedence = ast.precedence(valx_node);
    if(op_precedence > precedence)
        out << "(";

    switch(ast.at(op_node).type) {
        case FTK_HASH:
            out << "fci::length(";
            inline_expr(out, ast.child(valx_node, 1), 17);
            out << ")";
            break;

        case FTK_BANG: case FTK_MINUS: 
            if(ast.at(valx_node).children.size() == 2)
                out << ast.node_text(op_node);
                inline_expr(out, ast.child(valx_node, 1), op_precedence);
                break;

        case FTK_AND: case FTK_COMP: case FTK_EQ: case FTK_GT: case FTK_LE: 
        case FTK_LT: case FTK_NE: case FTK_OR: case FTK_PERCENT: 
        case FTK_PLUS: case FTK_SLASH: case FTK_STAR: 
            inline_expr(out, ast.child(valx_node, 1), op_precedence);
            out << " " << ast.node_text(op_node) << " ";
            inline_expr(out, ast.child(valx_node, 2), op_precedence);
            break;

        case FTK_QUESTION:
            inline_expr(out, ast.child(valx_node, 1), op_precedence);
            out << "? ";
            inline_expr(out, ast.child(valx_node, 2), op_precedence);
            out << ": ";
            inline_expr(out, ast.child(valx_node, 3), op_precedence);
            break;

        case FTK_SHL: case FTK_SHR:
            inline_expr(out, ast.child(valx_node, 1), op_precedence);
            out << " " << ast.node_text(op_node) << " ";
            inline_expr(out, ast.child(valx_node, 2), op_precedence);
            break;

        case FTK_fun:
            out << "fci::" << ast.node_text(ast.first_child(op_node)) << "(";
            for(int c = 1, e = ast.at(op_node).children.size(); c < e; ++c) {
                if(c > 2) out << ", ";
                inline_expr(out, ast.child(op_node, c), 17);
            }
            out << ")";
            break;

        case FTK_did:
            out << field_reference(op_node); 
            break;

        case FTK_ndid:
            ienfref[out.str().length()]=
                        ast.node_text(ast.first_child(op_node));
            /*
            ienfref.insert(std::make_pair(
                        ast.node_text(ast.first_child(op_node)),
                        out.str().length()));
            ienfref.push_back(std::make_pair(
                        ast.node_text(ast.first_child(op_node)),
                        out.str().length()));
                        */
            out << field_reference(op_node);
            break;

        case FTK_INTEGER: case FTK_FLOAT:
            out << ast.node_text(op_node);
            break;

        case FTK_STRING: 
            out << ast.node_text(op_node);
            break;

        default:
            out << " /* " << op_node << ": " << ast.node_text(op_node) << " */ ";
            break;
    }
    if(op_precedence > precedence)
        out << ")";
}
    
std::string cpp_gen::field_reference(int ndid_node) {
    std::vector<std::string> fields;
    for(int c: ast.at(ndid_node).children)
        fields.push_back(ast.node_text(c));
    return stru::join(fields, ".");
}

std::string cpp_gen::conditional_expr_method(int node, std::string name) {
    std::ostringstream out_sstream;
    stru::indent_ostream out(out_sstream);
    out.set_wrap_length(0);

    out << "bool c" << name << "() {\n";
    ++out;
    std::ostringstream asts; 
    ast.print_ast(asts, node);
    out << "/*\n" << asts.str() << "*/\n\n";

    out << "/* calling inline_expr(" << node << ") */\n";
    out << "auto val = ";
    ienfref.clear();
    out << stru::nowrap << inline_expr(node) << ";\n" << stru::wrap;
    if(ienfref.size() > 0)
        out << " /**** nfrefs: \n" << ienfref << "\n*/\n";
    out << "return val;\n";
    --out;
    out << "};\n";

    rexpr[name] = std::move(out_sstream).str();
    std::cerr << rexpr[name] << "\n";
    return iexpr;
}

int cpp_gen::mexpr_method(int node, std::string name) {
    std::ostringstream out_sstream;
    stru::indent_ostream out(out_sstream);

    // both OUTPUT and RETURN have one valx of type msgexp or ndid
    int valx_node = ast.child(node, 0);
    int msgexp_node = ast.child(valx_node, 0);
    out << "void m" << name << "(" << ast.cmsg(msgexp_node) << " &msg_" << msgexp_node << ") {\n";
    ++out;

    std::ostringstream asts; 
    ast.print_ast(asts, node);
    out << "/*\n" << asts.str() << "*/\n\n";

    if(ast.at(msgexp_node).type == FTK_ndid) {
        // direct node reference    
        std::string refd_node = ast.node_text(ast.child(msgexp_node, 0));   
        out << "msg_" << msgexp_node << ".copyFrom(result_" << refd_node << ");\n";
    } else {
        // setup message
        int list_node = ast.last_child(msgexp_node);
        out << "// list at " << list_node << "\n";
    }

    out << "return;\n";
    --out;
    out << "};\n";
    rexpr[name] = std::move(out_sstream).str();
    return 0;
}

}

namespace fc {
int compiler::cpp_generator(std::ostream &out_stream) const {
    int error_count = 0;
    stru::indent_ostream out(out_stream, 0);

    for(int en: get("//ENTRY"))  {
        auto ins = get("//INPUT", en);
        if(ins.size() == 0) ins = get("//flow/INPUT");

        cpp_gen data(*this, node_text(child(ins[0], 0)));

        error_count += data.entry_body(en);
        for(auto pe: data.process) 
            if(data.events.count(pe) == 0 && data.triggers.find(pe) == data.triggers.end()) 
                out << "//########### event \"" << pe << "\" is never generated!\n";

        for(auto &te: data.triggers) 
            if(data.process.count(te.first) == 0) 
                out << "//########### event \"" << te.first << "\" is not processed!\n";
            
        out << "/***\n";
        ++out;
        out << "process order: " << data.family_process_order << "\n";
        out << "node sizes: " << data.family_dim << "\n";
        out << "event processors: " << data.process << "\n";
        out << "triggers: " << data.triggers << "\n";
        --out;
        out << "*/\n";
        out << "enum entry_" << en << "_event {\n";
        ++out;
        for(auto ev: data.events)
            out << ev << ", ";
        out << "event_count\n";
        --out;
        out << "};\n";
        auto wrap_length = out.set_wrap_length(0);
        for(auto rx: data.rexpr) {
            out << rx.second << "\n";
        }
        out.set_wrap_length(wrap_length);
        out << "grpc::Status " << rpc(en) << "(grpc::ServerContext *pcontext, const *pinput, *poutput) {  // with " << ins[0] << " \"" << data.input_name << "\"\n";
        ++out;
        out << stru::join(data.flagvars, ", ", ", ", "bool ", "f_", "=false", "") << ";\n";
        out << stru::join(data.conditions, ",", ",", "int", " c_", "=0", ";") << "\n";
        out << "std::queue<entry_" << en << "_event> evq;\n";
        out << "grpc::CompletionQueue acq;\n";
        out << "grpc::Status status;\n";
        out << "\n";
        for(auto l: data.entry)
            out << l << "\n";
        out << "\n";
        out << "for(; !evq.empty(); evq.pop()) switch(evq.front()) {\n";
        ++out;
        for(auto e: data.events) {
            out << "case " << e << ":\n";
            ++out;
            if(data.cases.find(e) != data.cases.end()) 
                for(auto l: data.cases[e])
                    out << l.c_str() << "\n";
            if(e == ev(ev_check_flags, "")) for(auto te: data.triggers) {
                out << "if(" << stru::join(te.second, " && ", " && ", "", "f_") << ")\n";
                ++out <<  "evq.push(" << te.first << ");\n"; --out;
            }
            out << "break;\n";
            --out;
        }
        --out;
        out << "}\n";
        out << "return status;\n";
        --out;
        out << "};\n";
        out << "\n";
        for(auto x: data.exprs)
            out_stream << x << "\n";
    }
    return error_count;
}
}
std::ostream &operator << (std::ostream &out, expression const &x) {
    out << x.type << " " << x.type << "__" << x.unid << "() {\n";
    out << "    /* " << x.fam_refs << " */\n";
    out << "    return " << x.src << ";\n";
    out << "}\n";
    return out;
}
