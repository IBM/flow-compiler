#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>

#include "flow-comp.H"
//#include "stru.H"
#include "filtstream.H"

namespace fc {

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

struct cpp_data {
    // temp store - names of family nodes processed so far
    std::set<std::string> refd_nf;
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
};
static std::string ev(event_type e, std::string label) {
    return label.empty()? event_prefix[e]:
        event_prefix[e] + "_" + label;
}
static std::set<std::string> ev(event_type e, std::set<std::string> labels) {
    std::set<std::string> r;
    for(auto label: labels) 
        r.insert(ev(e, label));
    return r;
}
static std::string cv(std::string label) {
    return std::string("c_") + label;
}
static std::string rx(std::string label, int node) {
    return stru::sfmt() << "rx_" << label << "_" << node;
}
}
std::ostream &operator<< (std::ostream &out, struct fc::cpp_data const &ed);
std::string to_string(struct fc::cpp_data const &ed) {
    std::ostringstream os;
    os << ed;
    return os.str();
}
namespace fc {

int compiler::cpp_rexpr(cpp_data &data, int node, std::string name) const {
    std::ostringstream out_sstream;
    stru::indent_ostream out(out_sstream, 0);

    out << "bool " << name << "() {\n";
    ++out;
    std::ostringstream asts; 
    print_ast(asts, node);
    out << "/*\n" << asts.str() << "*/\n\n";
    out << "return true;\n";
    --out;
    out << "};\n";

    data.rexpr[name] = out_sstream.str();
    return 0;
}

int compiler::cpp_initiate_nf(cpp_data &data, std::string family) const {
    int err_count = 0;
    std::set<std::string> dep_families;

    auto is_family = [this, &family](int n) { return node_text(child(n, 0)) == family; };
    std::vector<int> nodes  = getp("//flow/NODE", is_family);
    auto event_i = ev(ev_initiate, family);
    auto event_a = ev(ev_available, family);
    auto *body = &data.cases[event_i];
    body->push_back(stru::sfmt() << "if(f_" << event_i << ") break;");
    body->push_back(stru::sfmt() << "f_" << event_i << " = true;");
    data.events.insert(event_i);
    data.process.insert(event_i);
    int def_n = 0, cond_events = 0;
    for(int n: nodes) if(node_type(last_child(n)) == FTK_valx) {
        data.conditions.insert(family);
        auto event_s = ev(ev_setup, unid(n));
        data.events.insert(event_s);
        int n_cvalx = last_child(n);
        if(const_level(n_cvalx) > 0) { // can evaluate in this body
            auto cond_var_name = cv(family);
            auto cond_xpr_label = rx(unid(n), n_cvalx);
            err_count += cpp_rexpr(data, n_cvalx, cond_xpr_label);
            body->push_back(stru::sfmt() << cond_var_name << " = " << cond_xpr_label << "()? " << n_cvalx << ": 0;");
            body->push_back(stru::sfmt() << "if(" << cond_var_name << " != 0) {\n    evq.push(" << event_s << ");\n    break;\n}");
            continue;
        }
        auto refd_nf = get_referenced_families(n_cvalx);
        refd_nf.erase(data.input_name);
        for(auto nf: refd_nf) { 
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_initiate, nf) << ");");
            dep_families.insert(nf);
        }
        if(refd_nf.size() > 0) {
            auto event_c = ev(ev_condition, unid(n));
            body->push_back(stru::sfmt() << "f_" << event_c << " = true;");
            data.events.insert(event_c);
            data.triggers[event_c] = ev(ev_available, refd_nf);
            data.triggers[event_c].insert(event_c);
            data.process.insert(event_c);
            body = &data.cases[event_c];
            ++cond_events;
        }
    } else {
        def_n = n;
    }
    if(def_n != 0) {
        if(cond_events) {
            data.events.insert(ev(ev_setup, unid(def_n)));
            body->push_back(stru::sfmt() << "evq.push(" << ev(ev_setup, unid(def_n)) << ");");
        }
    } else {
        body->push_back(stru::sfmt() << "f_" << event_a << " = true;");
        body->push_back(stru::sfmt() << "evq.push(" << ev(ev_check_flags, "") << ");");
        data.process.insert(event_i);
    }
    // process the default node first
    for(int pass = 1; pass < 3; ++pass) for(int n: nodes) if(pass == 1 && n == def_n || pass == 2 && n != def_n) {
        auto event_s = ev(ev_setup, unid(n));
        if(pass != 1) {
            data.process.insert(event_s);
            body = &data.cases[event_s];
            data.events.insert(event_s);
        }

        std::set<std::string> refd_nf;
        int action_node = 0;
        for(int an: get("//(ERRCHK|RETURN|OUTPUT)", n)) {
            for(int vx: at(an).children) {
                auto rn = get_referenced_families(vx);
                refd_nf.insert(rn.begin(), rn.end());
            }
            refd_nf.erase(data.input_name);
            action_node = an;
        }
        if(refd_nf.size() != 0) {
            for(auto nf: refd_nf) {
                body->push_back(stru::sfmt() << "evq.push(" << ev(ev_initiate, nf) << ");");
                dep_families.insert(nf);
            }
            auto event_r = ev(ev_ready, unid(n));
            body->push_back(stru::sfmt() << "f_" << event_r << " = true;");
            data.events.insert(event_r);
            data.process.insert(event_r);
            data.triggers[event_r] = ev(ev_available, refd_nf);
            data.triggers[event_r].insert(event_r);
            body = &data.cases[event_r];
        }
        body->push_back(stru::sfmt() << "auto r_" << unid(n) << " = SETM_" << unid(n) << "_" << action_node << "();");
        switch(node_type(action_node)) {
            case FTK_OUTPUT:
                body->push_back(stru::sfmt() << "queue_call(r_" << unid(n) << ", f_" << event_a << ");");
                break;
            case FTK_RETURN:
                body->push_back(stru::sfmt() << "f_" << event_a << " = true;");
                body->push_back(stru::sfmt() << "evq.push(" << ev(ev_check_flags, "") << ");");
                break;
            case FTK_ERRCHK:
                body->push_back(stru::sfmt() << "gen_error(r_" << unid(n) << ");");
                break;
        }
    }
    // process the dependents
    for(auto nf: dep_families) 
        if(data.refd_nf.count(nf) == 0) {
            data.refd_nf.insert(nf);
            cpp_initiate_nf(data, nf);
        }
    return err_count;
}

int compiler::cpp_generator(std::ostream &out_stream) const {
    stru::indent_ostream out(out_stream, 0);

    for(int en: get("//ENTRY"))  {
        cpp_data data;

        auto ins = get("//INPUT", en);
        if(ins.size() == 0) ins = get("//flow/INPUT");
        data.input_name = node_text(child(ins[0], 0));

        // it was already verified that there is one and only one RETURN/valx subsequence
        int en_rvalx = get("//RETURN/valx", en)[0];
        auto refd_nf = get_referenced_families(en_rvalx);
        refd_nf.erase(data.input_name);
        data.refd_nf = refd_nf;

        if(refd_nf.size() == 0) {
            data.entry.push_back(stru::sfmt() << "return SETM_" << en_rvalx << "();");
        } else {
            for(auto nf: refd_nf) {
                auto event_i = ev(ev_initiate, nf);
                data.entry.push_back(stru::sfmt() << "evq.push(" << event_i << ");");
                cpp_initiate_nf(data, nf);
            }
            auto event_d = ev(ev_done, "");
            data.events.insert(event_d);
            data.process.insert(event_d);
            data.triggers[event_d] = ev(ev_available, refd_nf);
            data.cases[event_d].push_back(stru::sfmt() << "return SETM_" << en_rvalx << "();");
        }
        std::set<std::string> flagvars;
        for(auto &te: data.triggers) {
            if(data.process.count(te.first) == 0) 
                out << "//########### event \"" << te.first << "\" is not processed!\n";
            data.events.insert(te.first);
            flagvars.insert(te.second.begin(), te.second.end());
        }
        if(data.triggers.size() > 0)
            data.events.insert(ev(ev_check_flags, ""));
        for(auto pe: data.process) 
            if(data.events.count(pe) == 0 && data.triggers.find(pe) == data.triggers.end()) {
                out << "//########### event \"" << pe << "\" is never generated!\n";
            }

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
        out << stru::join(flagvars, ", ", ", ", "bool ", "f_", " = false", "") << ";\n";
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
    return 0;
}

int compiler::generate(std::ostream &out_stream, int en, int input_node, bool debug_on) {
    return 0;
}
}
std::ostream &operator<< (std::ostream &out, fc::cpp_data const &ed) {
    return out;
}
