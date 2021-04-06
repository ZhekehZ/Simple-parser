
#include <string_view>
#include <cctype>
#include <iostream>

#include "lexer.h"
#include "parser.h"



int main() {
    std::vector<lexer::token> tokens;
    uint32_t pos = 0u;

    using namespace lexer;
    using statement = sequence<identifier, whitespace, constant, whitespace, k_if>;
    statement::parse(tokens, pos, "asd 09230 if");
    for (auto a : tokens) {
        std::cout << std::boolalpha << a.begin  << ' ' << a.begin + a.len << std::endl;
    }

    return 0;
}