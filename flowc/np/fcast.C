#include "fcast.H"

namespace fc {

void ast::error(int node, std::string message) {
    syntax_errors.push_back(std::make_pair(node, message));
}

std::string const &ast::get_id(int node) const {
    //MASSERT(at(node).type == FTK_ID) << " node " << node << " [" << at(node).token.text << "] is of type " << node_name(at(node).type) << ", expected ID\n";
    assert(at(node).type == FTK_ID);
    return at(node).token.text;
}

/** Lookup statement level keyword for node 'in'
 */
int ast::stmt_kwd(int in) const {
    static std::map<std::string, int> keywords = {
        {"node", FTK_NODE}, {"container", FTK_CONTAINER}, {"entry", FTK_ENTRY}, 
        {"import", FTK_IMPORT}, {"error", FTK_ERROR}, {"option", FTK_OPTION}, 
        {"input", FTK_INPUT}};
    auto ki = keywords.find(get_id(in));
    int k = ki == keywords.end()? FTK_DEFINE: ki->second;
    return k;
}
/** Lookup block level keyword for node 'in'
*/
int ast::blck_kwd(int in, int def_type) const {
    static std::map<std::string, int> keywords = {
        {"headers", FTK_HEADERS}, {"environment", FTK_ENVIRONMENT}, 
        {"mount", FTK_MOUNT}, {"init", FTK_INIT}, {"limits", FTK_LIMITS},
        {"output", FTK_OUTPUT}, {"return", FTK_RETURN} 
    };
    auto ki = keywords.find(get_id(in));
    int k = ki == keywords.end()? def_type: ki->second;
    return k;
}

void ast::expect(int node, std::set<int> const &types, std::string const &errm) {
    if(types.find(at(node).type) == types.end()) 
        error(node, errm);
}
void ast::expect(int node, int type, std::string const &errm) {
    expect(node, std::set<int>({type}), errm);
}
int ast::find(int node, int type, std::string const &id) const {
    for(auto p = begin(node); !p.at_end(); p.next()) {
        if(p->type == type && get_id(at((int) p).children[0]) == id)
            return (int) p; 
    }
    return 0;
}

}
