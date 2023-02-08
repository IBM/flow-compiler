#ifndef H_GRU_H
#define H_GRU_H

#include <iosfwd>
#include <string>
#include <vector>

#include <google/protobuf/compiler/importer.h>

#include "ansi-escapes.H"

namespace fc {
class FErrorPrinter: public google::protobuf::compiler::MultiFileErrorCollector {
private:
    google::protobuf::compiler::DiskSourceTree *source_tree;
    void AddMessage(std::string const type, ANSI_ESCAPE ansi_escape, std::string const filename, int line, int column, std::string const &message);
    void ShowLine(std::string const filename, int line, int column);
public:    
    std::ostream *outs;
    bool show_line_with_error;
    bool show_line_with_warning;
    int error_count;
    int warning_count;
    FErrorPrinter(google::protobuf::compiler::DiskSourceTree *source_tree = nullptr, bool use_stdout = false);
    void AddError(std::string const &filename, int line, int column, std::string const &message);
    void AddWarning(std::string const &filename, int line, int column, std::string const &message);
    void AddNote(std::string const &filename, int line, int column, std::string const &message);
};
class ErrorPrinter: public google::protobuf::io::ErrorCollector {
public:    
    FErrorPrinter &fperr;
    std::string filename;
    ErrorPrinter(FErrorPrinter &mfec, std::string const &file): fperr(mfec), filename(file) {
    }
    virtual void AddError(int line, int column, std::string const & message) {
        fperr.AddError(filename, line, column, message);
    }
};
class proto_compiler {
protected:
    google::protobuf::compiler::DiskSourceTree source_tree;
    FErrorPrinter pcerr;
    google::protobuf::compiler::Importer importer;
    // Command lines
    std::string grpccc;
    std::string protocc;
    std::set<google::protobuf::FileDescriptor const *> fds;
    std::vector<std::string> symbol_table;

    void add_symbols(google::protobuf::FileDescriptor const *fd);
    google::protobuf::DescriptorPool const *pool() const { return importer.pool(); }

public:
    void AddNote(std::string const &filename, int line, int column, std::string const &message) {
        pcerr.AddNote(filename, line, column, message);
    }
    void AddError(std::string const &filename, int line, int column, std::string const &message) {
        pcerr.AddError(filename, line, column, message);
    }
    void AddWarning(std::string const &filename, int line, int column, std::string const &message) {
        pcerr.AddWarning(filename, line, column, message);
    }
public:
    proto_compiler();

    int add_to_proto_path(std::string directory, std::string const &mapped_to="");
    int compile_proto(std::string file, bool map_file_dir=false);
    // Lookup methods
    google::protobuf::MethodDescriptor const *find_method(std::string const &name) const;
    google::protobuf::Descriptor const *find_message(std::string const &name) const;
    google::protobuf::FieldDescriptor const *find_field(std::string const &name) const;
    google::protobuf::EnumDescriptor const *find_enumt(std::string const &name) const;
    google::protobuf::EnumValueDescriptor const *find_enumv(std::string const &name) const;
    google::protobuf::ServiceDescriptor const *find_service(std::string const &name) const;
    std::string find_package(std::string const &name) const;
    // 
    int get_dependencies(std::vector<std::string> &results, std::vector<std::string> const &methods) const; 
    int get_proto_text(std::vector<std::string> &results, std::vector<std::string> const &symbols, int file_index=1) const; 
};
}

#endif
