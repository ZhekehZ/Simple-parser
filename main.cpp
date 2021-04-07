
#include <string_view>
#include <cctype>
#include <iostream>

#include "lexer.h"
#include "parser.h"

auto to_string(lexer::kind k) {
    switch (k) {
        case lexer::kind::WHITESPACE: return "WHITESPACE";
        case lexer::kind::IDENTIFIER: return "IDENTIFIER";
        case lexer::kind::CONSTANT: return "CONSTANT";
        case lexer::kind::ASSIGNMENT: return "ASSIGNMENT";
        case lexer::kind::OPEN: return "OPEN";
        case lexer::kind::CLOSE: return "CLOSE";
        case lexer::kind::IF: return "IF";
        case lexer::kind::WHILE: return "WHILE";
        case lexer::kind::END: return "END";
        case lexer::kind::OPERATOR: return "OPERATOR";
    }
    return "";
}

int main() {
    std::vector<lexer::token> tokens;
    uint32_t pos = 0u;

    using namespace lexer;
    std::string str("x = x");
    auto tok = program::parse(tokens, pos, str);
    for (auto a : tokens) {
        std::cout << std::boolalpha << a.begin  << '\t' << a.begin + a.len << '\t' << str.substr(a.begin, a.len)  << ' '
        << to_string(a.type) << std::endl;
    }

    if (!std::holds_alternative<uint32_t>(tok)) {
        auto res = std::get<error>(tok);
        std::cout << res.cause << " at pos " << res.pos << std::endl;
    }

    auto parse_res = parser::parse(tokens);
    parse_res.print();

    return 0;
}