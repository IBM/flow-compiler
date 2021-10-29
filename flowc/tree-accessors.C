#include "flow-compiler.H"

int flow_compiler::find_in_blck(int block_node, std::string const &name, int *pos) const {
    int lpos = 0;
    if(pos == nullptr) pos = &lpos;
    if(block_node != 0) {
        auto const &mn = at(block_node);
        for(int e = mn.children.size(); *pos < e; ++(*pos))
            if(get_id(at(mn.children[*pos]).children[0]) == name)
                return at(mn.children[(*pos)++]).children[1];
    }
    return 0;
}
int flow_compiler::get_block_value(std::vector<int> &values, int blck, std::string const &name, bool required, std::set<int> const &accepted_types) {
    int error_count = 0;
    if(blck == 0) return 0;
    if(at(blck).type == FTK_NODE || at(blck).type == FTK_ENTRY || at(blck).type == FTK_CONTAINER || at(blck).type == FTK_MOUNT || at(blck).type == FTK_ENVIRONMENT)
        blck = get_ne_block_node(blck);
    if(blck == 0) return 0;

    MASSERT(at(blck).type == FTK_blck) << "node " << blck << " is " << node_name(blck) << ", expected to be " << node_name(FTK_blck) << "\n";

    for(int p = 0, v = find_in_blck(blck, name, &p); v != 0; v = find_in_blck(blck, name, &p)) {
        if(accepted_types.size() > 0 && !cot::contains(accepted_types, at(v).type)) {
            ++error_count;
            std::set<std::string> accepted;
            std::transform(accepted_types.begin(), accepted_types.end(), std::inserter(accepted, accepted.end()), node_name); 
            pcerr.AddError(main_file, at(v), stru1::sfmt() << "\"" << name << "\" must be of type " << stru1::join(accepted, ", ", " or "));
            continue;
        }
        values.push_back(v);
    }
    if(required && values.size() == 0) {
        ++error_count;
        pcerr.AddError(main_file, at(blck), stru1::sfmt() << "at least one \"" << name << "\" value must be set"); 
    }
    return error_count;
}
int flow_compiler::get_block_value(int &value, int blck, std::string const &name, bool required, std::set<int> const &accepted_types) {
    std::vector<int> values;
    value = 0;
    int error_count = get_block_value(values, blck, name, required, accepted_types);
    if(error_count == 0 && values.size() > 0) {
        if(values.size() != 1) for(unsigned i = 0; i + 1 < values.size(); ++i)
            pcerr.AddWarning(main_file, at(values[i]), stru1::sfmt() << "ignoring previously set \"" << name << "\" value");
        value = values.back();
    }
    return error_count;
}
int flow_compiler::get_nv_block(std::map<std::string, int> &nvs, int parent_block, std::string const &block_label, std::set<int> const &accepted_types) {
    int error_count = 0;
    if(at(parent_block).type == FTK_NODE || at(parent_block).type == FTK_ENTRY || at(parent_block).type == FTK_CONTAINER)
        parent_block = get_ne_block_node(parent_block);

    for(int p = 0, v = find_in_blck(parent_block, block_label, &p); v != 0; v = find_in_blck(parent_block, block_label, &p)) {
        if(at(v).type != FTK_blck && at(v).type != FTK_HEADERS && at(v).type != FTK_MOUNT && at(v).type != FTK_ENVIRONMENT) {
            error_count += 1;
            pcerr.AddError(main_file, at(v), stru1::sfmt() << "\"" << block_label << "\" must be a name/value pair block");
            continue;
        }
        // Grab all name value pairs 
        for(int nv: at(v).children) {
            int n = at(nv).children[0], v = at(nv).children[1];
            if(!cot::contains(accepted_types, at(v).type)) {
                error_count += 1;
                std::set<std::string> accepted;
                std::transform(accepted_types.begin(), accepted_types.end(), std::inserter(accepted, accepted.end()), node_name);
                pcerr.AddError(main_file, at(v), stru1::sfmt() << "value for \"" << get_id(n) << "\" must be of type " << stru1::join(accepted, ", ", " or "));
                continue;
            }
            nvs[get_id(n)] = v; 
        }
    }
    return error_count;
}
int flow_compiler::get_block_s(std::string &value, int blck, std::string const &name, std::set<int> const &accepted_types) {
    int v = 0;
    int r = get_block_value(v, blck, name, true, accepted_types);
    if(v != 0) value = at(v).type == FTK_STRING? get_string(v): get_text(v);
    return r;
}
int flow_compiler::get_block_s(std::string &value, int blck, std::string const &name, std::set<int> const &accepted_types, std::string const &def) {
    int v = 0;
    int r = get_block_value(v, blck, name, false, accepted_types);
    if(v != 0) 
        value = at(v).type == FTK_STRING? get_string(v): get_text(v);
    else 
        value = def;
    return r;
}
int flow_compiler::get_block_i(int &value, int blck, std::string const &name) {
    int v = 0;
    int r = get_block_value(v, blck, name, true, {FTK_INTEGER, FTK_FLOAT});
    if(v != 0) 
        value = at(v).type == FTK_INTEGER? get_integer(v): int(get_float(v));
    return r;
}
int flow_compiler::get_block_i(int &value, int blck, std::string const &name, int def) {
    int v = 0;
    int r = get_block_value(v, blck, name, false, {FTK_INTEGER, FTK_FLOAT});
    if(v != 0) 
        value = at(v).type == FTK_INTEGER? get_integer(v): int(get_float(v));
    else
        value = def;
    return r;
}
int flow_compiler::get_blck_timeout(int blck, int default_timeout) {
    int timeout_value = 0;
    get_block_value(timeout_value, blck, "timeout", false, {});
    if(timeout_value == 0) return default_timeout;
    int timeout = 0;
    switch(at(timeout_value).type) {
        case FTK_FLOAT:
            timeout = int(get_float(timeout_value)*1000);
            break;
        case FTK_INTEGER:
            timeout = get_integer(timeout_value)*1000;
            break;
        case FTK_STRING:
            timeout = stru1::get_time_value(get_string(timeout_value));
            break;
        default:
            break;
    }
    if(timeout <= 0) {
        pcerr.AddWarning(main_file, at(timeout_value), stru1::sfmt() << "ignoring invalid value for \"timeout\", using the default of \""<<default_timeout<<"ms\"");
        return default_timeout;
    }
    return timeout;
}
