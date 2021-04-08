## Simple parser with unused assignments analyzer
[![CMake](https://github.com/ZhekehZ/repo1/actions/workflows/cmake.yml/badge.svg?branch=main)](https://github.com/ZhekehZ/repo1/actions/workflows/cmake.yml)

### Language
```hs
program ::= statement_list
statement_list ::= statement 
                 | statement_list statement
statement ::= variable '=' expression 
            | 'if' expression statement_list 'end' 
            | 'while' expression statement_list 'end'   
expression ::= variable 
             | constant 
             | '(' expression ')' 
             | expression operator expression
operator ::= '+' | '-' | '*' | '/' | '<' | '>' 
```


### Usage
```c++
int main() {
    auto str = R"(
        a = 1
        b = a
        x = 3
        y = 4

        while (b < 5)
          z = x
          b = b + 1
          x = 9
          y = 10
        end
    )";
    
    auto parser_result = parser::parse(str); // returns tree or error
    
    if (std::holds_alternative<lexer::error>(parser_result)) {
        auto err = std::get<lexer::error>(parser_result);
        std::cerr << "Error: " << err.cause << " at pos " << err.pos << std::endl;
        return 1;
    }
    
    auto tree = std::get<parser::ast::tree>(parser_result); // AST
    auto unused = find_unused_assignments(tree, str); // unused assignment
    
    std::sort(unused.begin(), unused.end(),
        [&](uint32_t l, uint32_t r) {
            return tree.get_range(l).first < tree.get_range(r).first;
        }); // sort in order of appearance
    
    for (auto idx : unused) {
        std::cout << tree.get_string(idx, str) << std::endl;
        /* OUTPUT:
         * y = 4
         * z = x
         * y = 10
         */
    }
    return 0;
}
```
