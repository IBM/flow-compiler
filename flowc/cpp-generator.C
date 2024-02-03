#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>

#include "flow-comp.H"
#include "stru.H"

namespace fc {
struct cpp_data {
    std::set<std::string> refd_nf;
    std::set<std::string> events;
    std::set<std::string> process;
    std::map<std::string, std::set<std::string>> triggers;
    std::string input_name;
    std::vector<std::string> entry;
    std::map<std::string, std::vector<std::string>> cases;
};
}
std::ostream &operator<< (std::ostream &out, struct fc::cpp_data const &ed);
std::string to_string(struct fc::cpp_data const &ed) {
    std::ostringstream os;
    os << ed;
    return os.str();
}

static std::string ev(std::string action, std::string label) {
    return action + "_" + label;
}
namespace fc {

int compiler::cpp_initiate_nf(std::string family, cpp_data &data) const {
    auto is_family = [this, &family](int n) { return node_text(child(n, 0)) == family; };
    auto get_unid = [this](int n) { return unid(n); };
    std::vector<int> nodes  = getp("//flow/NODE", is_family);
    //out << "--process initiate " << family << " // nodes: " << stru::joint(nodes, get_unid, ", ") << "\n";
    auto *body = &data.cases[ev("initiate", family)];
    body->push_back(stru::sfmt() << "if(f_" << ev("initiate", family) << ") break;");
    data.process.insert(ev("initiate", family));
    int def_n = 0;
    for(int n: nodes) if(node_type(last_child(n)) == FTK_valx) {
        data.events.insert(ev("setup", unid(n)));
        int n_cvalx = last_child(n);
        if(const_level(n_cvalx) > 0) { // can evaluate in this body
            body->push_back(stru::sfmt() << "bool c_" << unid(n) << " = XPR(" << n_cvalx << ");");
            body->push_back(stru::sfmt() << "if(c_" << unid(n) << ") evq.push(" << ev("setup", unid(n)) << ");");
            continue;
        }
        auto refd_nf = get_referenced_families(n_cvalx);
        refd_nf.erase(data.input_name);
        for(auto nf: refd_nf) { 
            data.events.insert(ev("initiate", nf));
            body->push_back(stru::sfmt() << "evq.push(" << ev("initiate", nf) << ");");
            if(data.refd_nf.count(nf) == 0) {
                data.refd_nf.insert(nf);
                cpp_initiate_nf(nf, data);
            }
        }
        data.events.insert(ev("condition", unid(n)));
        data.triggers[ev("condition", unid(n))] = refd_nf;
        data.process.insert(ev("condition", unid(n)));
        body = &data.cases[ev("condition", unid(n))];
    } else {
        def_n = n;
    }
    if(def_n != 0) {
        // out << "--event setup " << unid(def_n) << "\n";
        data.events.insert(ev("setup", unid(def_n)));
        body->push_back(stru::sfmt() << "evq.push(" << ev("setup", unid(def_n)) << ");");
    }

    for(int n: nodes) {
        //out << "--process setup " << unid(n) << "\n";
        data.process.insert(ev("setup", unid(n)));
        body = &data.cases[ev("setup", unid(n))];
        std::set<std::string> refd_nf;
        int action_node = 0;
        for(int an: get("//(ERRCHK|RETURN|OUTPUT)", n)) {
            //out << "//// node " << unid(n) << " has " << tk_to_string(node_type(an)) << "\n";
            for(int vx: at(an).children) {
                auto rn = get_referenced_families(vx);
                refd_nf.insert(rn.begin(), rn.end());
            }
            refd_nf.erase(data.input_name);
            action_node = an;
        }
        if(refd_nf.size() != 0) for(auto nf: refd_nf) {
            data.events.insert(ev("initiate", nf));
            body->push_back(stru::sfmt() << "evq.push(" << ev("initiate", nf) << ");");
            if(data.refd_nf.count(nf) == 0) {
                data.refd_nf.insert(nf);
                cpp_initiate_nf(nf, data);
            }
            data.triggers[ev("ready", unid(n))] = refd_nf;
            data.events.insert(ev("ready", unid(n)));
            data.process.insert(ev("ready", unid(n)));
            body = &data.cases[ev("ready", unid(n))];
        }
        body->push_back(stru::sfmt() << "auto r_" << unid(n) << " = ACTXPR(" << action_node << ");");
        // eval node n
        switch(node_type(action_node)) {
            case FTK_OUTPUT:
                data.events.insert(ev("available", node_text(first_child(n))));
                body->push_back(stru::sfmt() << "queue_call(r_" << unid(n) << ", " << ev("available",  node_text(first_child(n))) << ");");
                break;
            case FTK_RETURN:
                data.events.insert(ev("available", node_text(first_child(n))));
                body->push_back(stru::sfmt() << "evq.push(" << ev("available", node_text(first_child(n))) << ");");
                break;
            case FTK_ERRCHK:
                body->push_back(stru::sfmt() << "gen_error(r_" << unid(n) << ");");
                break;
        }
    }

    return 0;
}

int compiler::cpp_generator(std::ostream &out_stream) const {
    stru::indented_stream out(out_stream, 0);

    for(int en: get("//ENTRY"))  {
        cpp_data data;

        auto ins = get("//INPUT", en);
        if(ins.size() == 0) ins = get("//flow/INPUT");
        data.input_name = node_text(child(ins[0], 0));
        out << "// entry " << en << " \"" << rpc(en) << "\"  with " << ins[0] << " \"" << data.input_name << "\"\n";
        ++out;

        // it was already verified that there is one and only one RETURN/valx subsequence
        int en_rvalx = get("//RETURN/valx", en)[0];
        auto refd_nf = get_referenced_families(en_rvalx);
        refd_nf.erase(data.input_name);
        data.refd_nf = refd_nf;

        if(refd_nf.size() == 0) {
            data.entry.push_back(stru::sfmt() << "return XPR(" << en_rvalx << ");");
        } else {
            for(auto nf: refd_nf) {
                data.entry.push_back(stru::sfmt() << "evq.push(" << ev("initiate", nf) << ");");
                data.events.insert(ev("initiate", nf));
                cpp_initiate_nf(nf, data);
            }
            data.events.insert("done");
            data.process.insert("done");
            data.triggers["done"] = refd_nf;
            data.cases["done"].push_back(stru::sfmt() << "return XPR(" << en_rvalx << ");");
        }
        std::set<std::string> triggers;
        std::map<std::string, std::set<std::string>> traps;
        for(auto &te: data.triggers) {
            std::set<std::string> evs;
            for(auto nf: te.second)
                evs.insert(ev("available", nf));
            te.second = evs;
            if(data.process.count(te.first) == 0) 
                out << "//########### event \"" << te.first << "\" is not processed!\n";
            data.events.insert(te.first);
            triggers.insert(evs.begin(), evs.end());
            for(auto e: evs)
                traps[e].insert(te.first);
        }
        for(auto pe: data.process) 
            if(data.events.count(pe) == 0 && data.triggers.find(pe) == data.triggers.end()) {
                out << "//########### event \"" << pe << "\" is never generated!\n";
            }

        out << "// nodes needed: " << data.refd_nf << "\n"; 
        //out << "// events generated: " << data.events << "\n";
        out << "// event processors: " << data.process << "\n";
        out << "// triggers: " << data.triggers << "\n";
        out << "enum entry_" << en << "_event {\n";
        ++out;
        for(auto ev: data.events)
            out << ev << ",\n";
        out << "event_count\n";
        --out;
        out << "};\n";
        out << stru::join(triggers, ",", ",", "bool", " f_", " = false", ";") << "\n";
        out << "std::queue<entry_" << en << "_event> evq;\n";
        out << "grpc::CompletionQueue acq;\n";
        out << "\n";
        for(auto l: data.entry)
            out << l << "\n";
        out << "\n";
        out << "for(; !evq.empty(); evq.pop()) switch(evq.front()) {\n";
        ++out;
        for(auto e: data.events) {
            out << "case " << e << ":\n";
            ++out;
            if(traps.find(e) != traps.end()) {
                out << "f_" << e << " = true;\n"; 
                for(auto tre: traps.find(e)->second) {
                    auto deps = data.triggers[tre];
                    deps.erase(e);
                    if(deps.size() > 0) {
                        out << "if(" << stru::join(deps, " && ", " && ", "", "f_") << ")\n";
                        ++out;
                        out << "evq.push(" << tre << ");\n";
                        --out;
                    } else {
                        out << "evq.push(" << tre<< ");\n";
                    }
                }
                out << "\n";
            }
            if(data.cases.find(e) != data.cases.end()) 
                for(auto l: data.cases[e])
                    out << l << "\n";
            --out;
            out << "break;\n";
        }
        --out;
        out << "}\n";
        --out;
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
