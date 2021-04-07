#include <iostream>

#include "src/pretty_print.h"

int main() {
    std::string s = R"(
       if x > 0 x = 2 end

    )";

    auto result = parser::parse(s);

    if (std::holds_alternative<lexer::error>(result)) {
        auto err = std::get<lexer::error>(result);
        std::cerr << "ERROR " << err.cause << " at pos " << err.pos << std::endl;
    } else {
        auto tree = std::get<parser::ast::tree>(result);
        printer::print(std::cout, tree, s);
    }

    return 0;
}