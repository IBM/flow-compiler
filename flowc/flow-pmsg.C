#include <fstream>
#include <string>
#include <algorithm>

#include "ansi-escapes.H"
#include "ast-scanner.H"
#include "filu.H"
#include "stru.H"
#include "flow-comp.H"

static
void ShowLine(std::ostream *outs, std::string filename, int line, int column) {
    std::string disk_file = filename;
    std::ifstream sf(disk_file.c_str());
    if(sf.is_open()) {
        std::string lines;
        for(int i = 0; i < line; ++i) std::getline(sf, lines);
        // FIXME: The column value doesn't take tabs into account.
        // We replace tabs with spaces so the caret will point to the correct position.
        // Should the column number consider tabs, that would need to be addresed in the scanner.
        // If not then it can be addessed here and in the Message() below.
        std::transform(lines.cbegin(), lines.cend(), lines.begin(), 
                   [](char c) { return c == '\t'? ' ': c; });
        *outs << lines << "\n";
        if(column > 2) *outs << std::string(column-1, ' ');
        *outs << "^" << "\n";
    }
}
static
void Message(std::ostream *outs, std::string type, ANSI_ESCAPE color, std::string filename, int line, int column, std::string const &message) {
    *outs << ANSI_BOLD;
    if(!filename.empty()) {
        *outs << filu::basename(filename);
        if(line > 0) *outs << "(" << line;
        if(line > 0 && column > 0) *outs << ":" << column;
        if(line > 0) *outs << ")";
        *outs << ": ";
    }
    *outs << color;
    *outs << type << ": ";
    *outs << ANSI_RESET;
    *outs << ansi::emphasize(message, ANSI_BOLD+ANSI_GREEN) << "\n";
}
namespace fc {
void compiler::message(std::string filename, int line, int column, message_type type, bool prompt, std::string const &message) const {
    switch(type) {
        case mt_error:
            Message(errs, "error", ANSI_RED, filename, line, column, message);
            break;
        case mt_warning:
            Message(errs, "warning", ANSI_MAGENTA, filename, line, column, message);
            break;
        case mt_note:
            Message(errs, "note", ANSI_BLUE, filename, line, column, message);
            break;
    }
    if(prompt && !filename.empty() && line >= 0 && column >=0)
        ShowLine(errs, filename, line, column);
}

}
