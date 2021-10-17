#include <iostream>
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
    std::map<int, std::set<std::string>> incoming;

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
    int s = 0;
    for(auto const &n: fgp->second) {
        ++s;
        if(n.size()  > 0) {
            out << "{ rank = same;\n";
            /*
            for(auto nn: n) {
                auto const &inf = referenced_nodes.find(nn)->second;
                if(condition.has(nn))
                    out << c_escape(inf.xname) << "[label=<" << name(nn) << "<sup><font point-size=\"7\">" << inf.order << "</font></sup><br/><font point-size=\"7\" face=\"Courier\">" << html_escape(to_text(condition(nn))) << "</font>>]; ";
                else
                    out << c_escape(inf.xname) << "[label=<" << name(nn) << ">]; ";
            }
            */
            out << s << ";\n};\n";
        }
        for(auto nn: n) {
            std::string dot_node(stru1::c_escape(name(nn)));
            // Get all incoming edges
            incoming.clear();
            /*
            for(auto i: get_referenced_nodes_action(incoming, nn))
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.xname)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=bold,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
                }
            
            incoming.clear();
            for(auto i: get_node_condition_refs(incoming, nn)) 
                if(i.first == 0) {
                    out << input_label << " -> " << dot_node << " [fontsize=9,style=dashed,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
                } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
                    std::string dot_i(c_escape(j.second.xname)); 
                    out << dot_i << " -> " << dot_node << " [fontsize=9,style=dashed,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
                }
                */
        }
    }
    out << "node [shape=invtriangle];\n";
    out << "{ rank = same; " << ename << "[label=" << stru1::c_escape(method_descriptor(entry)->name()) << "]; \"[o]\"; };\n";
    incoming.clear();
    /*
    for(auto i: get_node_body_refs(incoming, entry)) 
        if(i.first == 0) {
            out << input_label << " -> " << ename << " [fontsize=9,style=bold,color=forestgreen,label=\"" << make_label(i.second, input_dp) << "\"];\n";
        } else for(auto j: referenced_nodes) if(name(i.first) == name(j.first)) {
            std::string dot_i(c_escape(j.second.xname)); 
            out << dot_i << " -> " << ename << " [fontsize=9,style=bold,color=dodgerblue2,label=\"" << make_label(i.second, message_descriptor(i.first)) << "\"];\n";
        }
        */
    out << "}\n";
}

