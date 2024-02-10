#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>

#include "flow-comp.H"
#include "stru.H"
#include "filtstream.H"


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

struct cpp_gen {
    // the AST
    fc::compiler const &ast;
    // temp store - names of family nodes processed so far
    std::set<std::string> refd_nf;
    // all the dependent families
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
    // flag variable names for each node 
    std::set<std::string> flagvars;
    // keep track of the number of errors generated
    int error_count;

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
    int rexpr_method(int node, std::string name);
    std::string inline_expr(int valx_node);
    std::string field_reference(int ndid_node);
    
};

cpp_gen::cpp_gen(fc::compiler const &a_ast, std::string a_input_name): 
    ast(a_ast), input_name(a_input_name), error_count(0) {
}
int cpp_gen::entry_body(int entry_node) {
    // it was already verified that there is one and only one RETURN/valx subsequence
    int en_rvalx = ast.get("//RETURN/valx", entry_node)[0];
    auto refd_nf = ast.get_referenced_families(en_rvalx);
    refd_nf.erase(input_name);
    this->refd_nf = refd_nf;

    if(refd_nf.size() == 0) {
        entry.push_back(stru::sfmt() << "return SETM_" << en_rvalx << "();");
    } else {
        for(auto nf: refd_nf) {
            auto event_i = ev(ev_initiate, nf);
            entry.push_back(stru::sfmt() << "evq.push(" << event_i << ");");
            node_family(nf);
        }
        auto event_d = ev(ev_done, "");
        events.insert(event_d);
        process.insert(event_d);
        triggers[event_d] = ev(ev_available, refd_nf);
        cases[event_d].push_back(stru::sfmt() << "return SETM_" << en_rvalx << "();");
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

    // if the current conditional is a local constant we can evaluate it
    // in the current event (i.e. case block) and avoid creating another event
    if(ast.const_level(n_cvalx) > 0) { 
        auto cond_var_name = cv(family);
        auto cond_xpr_label = rx(unid, n_cvalx);
        rexpr_method(n_cvalx, cond_xpr_label);
        body->push_back(stru::sfmt() << cond_var_name << " = " << cond_xpr_label << "()? " << n_cvalx << ": 0;");
        body->push_back(stru::sfmt() << "if(" << cond_var_name << " != 0) {\n    evq.push(" << event_s << ");\n    break;\n}");
        return body_event;
    } else {
        // otherwise get the dependent node and add them to the intiate list
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
        }
    }
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
    switch(ast.node_type(action_node)) {
        case FTK_OUTPUT:
            body->push_back(stru::sfmt() << "auto r_" << unid << " = smsg_" << unid << "_" << action_node << "();");
            body->push_back(stru::sfmt() << "queue_call(r_" << unid << ", f_" << event_a << ");");
            break;
        case FTK_RETURN:
            body->push_back(stru::sfmt() << "auto r_" << unid << " = smsg_" << unid << "_" << action_node << "();");
            body->push_back(stru::sfmt() << "f_" << event_a << " = true;");
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_check_flags, "") << ");");
            break;
        case FTK_ERRCHK:
            body->push_back(stru::sfmt() << "gen_error(rxpr_" << unid << ");");
            break;
    }
    return body_event;
}
int cpp_gen::node_family(std::string family) {
    int fam_count = 1;

    auto is_family = [this, &family](int n) { return ast.node_text(ast.child(n, 0)) == family; };
    std::vector<int> nodes  = ast.getp("//flow/NODE", is_family);
    auto event_i = ev(ev_initiate, family);
    auto event_a = ev(ev_available, family);
    events.insert(event_i);
    process.insert(event_i);
    std::string body_event = event_i;
    auto *body = &cases[body_event];

    body->push_back(stru::sfmt() << "if(f_" << event_i << ") break;");
    body->push_back(stru::sfmt() << "f_" << event_i << " = true;");

    int def_n = 0, cond_events = 0;
    for(int n: nodes) if(ast.node_type(ast.last_child(n)) == FTK_valx) {
        std::string prev_body_event = body_event;
        body_event = condition_event(family, n, body_event);
        if(prev_body_event != body_event) {
            ++cond_events;
            body = &cases[body_event];
        }
    } else {
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
    // process the dependents
    for(auto nf: dep_families) 
        if(refd_nf.count(nf) == 0) {
            refd_nf.insert(nf);
            fam_count += node_family(nf);
        }
    return fam_count;
}
std::string cpp_gen::inline_expr(int valx_node) {
    std::ostringstream out;
    int first_c = ast.first_child(valx_node);
    switch(ast.at(first_c).type) {
        case FTK_HASH: 
            out << "fci::length(";
            out << inline_expr(ast.child(valx_node, 1));
            out << ")";
            break;

        case FTK_BANG: case FTK_MINUS: 
            if(ast.at(valx_node).children.size() == 2)
                out << ast.node_text(first_c);
                out << inline_expr(ast.child(valx_node, 1));
                break;

        case FTK_AND: case FTK_COMP: case FTK_EQ: case FTK_GT: case FTK_LE: 
        case FTK_LT: case FTK_NE: case FTK_OR: case FTK_PERCENT: 
        case FTK_PLUS: case FTK_SLASH: case FTK_STAR: 
            out << inline_expr(ast.child(valx_node, 1));
            out << " " << ast.node_text(first_c) << " ";
            out << inline_expr(ast.child(valx_node, 2));
            break;

        case FTK_QUESTION:
            out << inline_expr(ast.child(valx_node, 1));
            out << "? ";
            out << inline_expr(ast.child(valx_node, 2));
            out << ": ";
            out << inline_expr(ast.child(valx_node, 3));
            break;

        case FTK_SHL: case FTK_SHR:
            out << ast.node_text(first_c) << "(";
            out << inline_expr(ast.child(valx_node, 1));
            out << ", ";
            out << inline_expr(ast.child(valx_node, 2));
            out << ")";
            break;

        case FTK_fun:
            out << "fci::" << ast.node_text(ast.first_child(first_c)) << "(";
            for(int c = 1, e = ast.at(first_c).children.size(); c < e; ++c) {
                if(c > 2) out << ", ";
                out << inline_expr(ast.child(first_c, c));
            }
            out << ")";
            break;

        case FTK_did:
            out << field_reference(first_c); 
            break;

        case FTK_ndid:
            out << field_reference(first_c);
            break;

        default:
            break;
    }
    return std::move(out).str();
}
    
std::string cpp_gen::field_reference(int ndid_node) {
    std::vector<std::string> fields;
    for(int c: ast.at(ndid_node).children)
        fields.push_back(ast.node_text(c));
    return stru::join(fields, ".");
}

int cpp_gen::rexpr_method(int node, std::string name) {
    std::ostringstream out_sstream;
    stru::indent_ostream out(out_sstream);

    out << "bool c" << name << "() {\n";
    ++out;
    std::ostringstream asts; 
    ast.print_ast(asts, node);
    out << "/*\n" << asts.str() << "*/\n\n";

    out << "auto val = ";
    out << inline_expr(node);
    out << ";\n";

    out << "return val;\n";
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
        out << "nodes needed: " << data.refd_nf << "\n"; 
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
        for(auto rx: data.rexpr) {
            out << rx.second << "\n";
        }
        out << "grpc::Status " << rpc(en) << "(grpc::ServerContext *pcontext, const *pinput, *poutput) {  // with " << ins[0] << " \"" << data.input_name << "\"\n";
        ++out;
        out << stru::join(data.flagvars, ", ", ", ", "bool ", "f_", " = false", "") << ";\n";
        out << stru::join(data.conditions, ",", ",", "int", " c_", " = 0", ";") << "\n";
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
    }
    return error_count;
}

}
