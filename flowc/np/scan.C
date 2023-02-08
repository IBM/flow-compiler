#include <iostream>
#include "ast-scanner.H"
#include "ansi-escapes.H"

int main(int argc, char *argv[]) {
    ast::scanner sc(&std::cin);
    for(auto t = sc.scan(); t.type != ATK_EOF; t = sc.scan())
        if(t.type == ATK_UNK)
            std::cout << ANSI_RED << t << ANSI_RESET << " ";
        else
            std::cout << t << " ";
    return 0;
}
