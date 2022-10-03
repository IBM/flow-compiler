#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

#include "flow-compiler.H"
#include "stru1.H"


static std::pair<std::string, std::string> make_labels(std::set<std::string> const &fields, google::protobuf::Descriptor const *d=nullptr)  {
    std::string ftip(stru1::join(fields, "\\n"));
    if(d != nullptr) {
        std::vector<std::string> all_fieldsv = get_field_names(d, ".");
        std::set<std::string> all_fields(all_fieldsv.begin(), all_fieldsv.end()); 
        if(cot::includes(fields, all_fields)) 
            return std::make_pair(std::string(), ftip);
    }
    return std::make_pair(stru1::join(fields, ",\\n"), ftip);
}

static std::string make_label(std::set<std::string> const &fields, google::protobuf::Descriptor const *d=nullptr)  {
    auto l = make_labels(fields, d);
    if(l.second.empty()) return l.first;
    return l.first + "\" edgetooltip=\"" + l.second;
}

void flow_compiler::print_graph(std::ostream &out, int entry) {
    if(entry < 0) {
        for(int e = 0; e  < flow_graph.size(); ++e) 
            print_graph(out, e);
        return;
    }
    auto fgp = flow_graph.find(entry);
    if(fgp == flow_graph.end()) return;

    std::string ename(stru1::c_escape(std::string("<")+method_descriptor(entry)->name()+">"));
    std::set<std::pair<int, int>> incoming;
    std::map<std::string, std::set<std::string>> edges;

    out << "digraph " << stru1::c_escape(method_descriptor(entry)->full_name()) << " {\n";
    out << "{ node [shape=invtriangle]; \"" << input_label << "\"; node [shape=plaintext];\n";
    out << "\"[i]\"";
    for(int s = 0; s < fgp->second.size(); ++s) {
        out << " -> \"" << (s+1) << "\"";
    }
    out << " -> \"[o]\";\n";
    out << "}\n";
    out << "{ rank = same; \"" << input_label << "\"; \"[i]\"; };\n";
    out << "node [shape=ellipse];\n";
    int s = 0, order = 0;
    for(auto const &n: fgp->second) {
        ++s;
        if(n.size()  > 0) {
            out << "{ rank = same;\n";
            for(int nn: n) {
                ++order;
                int cono = get_ne_condition(nn);
                if(cono != 0)
                    out << stru1::c_escape(name(nn)) << "[label=<" << name(nn) << "<sup><font point-size=\"7\">" << order << "</font></sup><br/><font point-size=\"7\" face=\"Courier\">" << stru1::html_escape(to_text(cono)) << "</font>>]; ";
                else
                    out << stru1::c_escape(name(nn)) << "[label=<" << name(nn) << ">]; ";
            }
            out << s << ";\n};\n";
        }

        for(auto nn: n) {
            std::string dot_node(stru1::c_escape(name(nn)));
            // Get all incoming edges
            incoming.clear(); edges.clear();
            if(get_ne_action(nn) != 0) 
                get_field_refs(incoming, get_ne_action(nn));
            for(auto i: incoming)
                edges[get_dotted_id(i.first, 0, 1)].insert(get_dotted_id(i.first, 1));
            for(auto e: edges) {
                std::string dot_i = stru1::join(e.second, ",\\n");
                out << e.first << " -> " << dot_node << " [fontsize=9,style=bold"<<(e.first == input_label?",color=forestgreen":"")<< ",label=\"" << dot_i << "\"];\n";
            }
            incoming.clear(); edges.clear();
            if(get_ne_condition(nn) != 0) 
                get_field_refs(incoming, get_ne_condition(nn));
            for(auto i: incoming)
                edges[get_dotted_id(i.first, 0, 1)].insert(get_dotted_id(i.first, 1));
            for(auto e: edges) {
                std::string dot_i = stru1::join(e.second, ",\\n");
                out << e.first << " -> " << dot_node << " [fontsize=9,style=dashed"<<(e.first == input_label?",color=forestgreen":"")<< ",label=\"" << dot_i << "\"];\n";
            }
        }
    }
    out << "node [shape=invtriangle];\n";
    out << "{ rank = same; " << ename << "[label=" << stru1::c_escape(method_descriptor(entry)->name()) << "]; \"[o]\"; };\n";
    incoming.clear(); edges.clear();
    get_field_refs(incoming, get_ne_action(entry)); 
    for(auto i: incoming)
        edges[get_dotted_id(i.first, 0, 1)].insert(get_dotted_id(i.first, 1));
    for(auto e: edges) {
        std::string dot_i = stru1::join(e.second, ",\\n");
        out << e.first << " -> " << ename << " [fontsize=9,style=bold"<<(e.first == input_label?",color=forestgreen":",color=dodgerblue2")<< ",label=\"" << dot_i << "\"];\n";
    }
    out << "}\n";
}
