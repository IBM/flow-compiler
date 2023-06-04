


namespace fg {
int flow_compiler::compile_if_import(int stmt_node, bool ignore_imports) {
    auto const &stmt = at(stmt_node);
    // Ignore unless an import statement
    if(stmt.type != FTK_IMPORT)
        return 0;

    assert(stmt.children.size() == 1);

    std::string filename;
    if(compile_string(filename, stmt.children[0])) {
        pcerr.AddError(main_file, at(stmt.children[0]), "expected filename string");
        return 1;
    }
    if(ignore_imports) 
        return 0;
    int ec = compile_proto(filename);
    if(ec > 0) 
        pcerr.AddError(main_file, stmt.token, stru::sfmt() << "failed to import \"" << filename << "\"");
    return ec;
}

int flow_compiler::compile_proto(std::string const &file, int map_file_dir) {
    auto fdp = importer.Import(file);
    if(fdp == nullptr && map_file_dir != 0) {
        add_to_proto_path(filu::dirname(file));
        fdp = importer.Import(file);
    }
    if(fdp == nullptr) {
        pcerr.AddError(file, -1, 0, "import failed");
        return 1;
    }
    if(fdp->syntax() != FileDescriptor::Syntax::SYNTAX_PROTO3) {
        pcerr.AddError(file, -1, 0, "syntax must be proto3");
        return 1;
    }
    std::set<FileDescriptor const *> included(fdps.begin(), fdps.end());
    if(cot::contains(included, fdp)) return 0;
    fdps.push_back(fdp);
    included.insert(fdp);
    get_enums(enum_value_set, fdp);
    // Add the dependents to our set
    for(int i = 0, e = fdp->dependency_count(); i != e; ++i) {
        auto fdq = fdp->dependency(i);
        if(!cot::contains(included, fdq)) {
            fdps.push_back(fdq);
            included.insert(fdq);
            get_enums(enum_value_set, fdq);
        }
    }
    return 0;
}
}
