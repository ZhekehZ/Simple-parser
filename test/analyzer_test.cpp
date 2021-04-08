#include <catch2/catch.hpp>

#include <analyze.h>
#include <iostream>
#include <sstream>

std::string find_unused_and_dump(std::string_view str) {
    auto tree = std::get<parser::ast::tree>(parser::parse(str));
    auto fvs = find_unused_assignments(tree, str);
    std::sort(fvs.begin(), fvs.end(), [&](uint32_t l, uint32_t r) {
        return tree.get_range(l).first < tree.get_range(r).first;
    });
    std::stringstream ss;
    for (auto idx : fvs) {
        ss << tree.get_string(idx, str) << std::endl;
    }
    return ss.str();
}

TEST_CASE("Test unused assignments analyzer", "[analyzer]") {
    std::string input, expected;

    std::tie(input, expected) =
        GENERATE(table<std::string, std::string>({
             {
                 R"(x=1)",
                 "x=1\n"
             },
             {
                 R"(x=y y=x)",
                 "y=x\n"
             },
             {
                 R"(x=y x=0)",
                 "x=y\n"
                 "x=0\n"
             },
             {
                 R"(
                    x = 0
                    if x > 0
                        y = x
                    end
                    x = y
                 )",
                 "x = y\n"
             },
             {
                 R"(
                    x = 0
                    while x < 100
                        x = x + 1
                    end
                 )",
                 ""
             },
             {
                 R"(
                    x = 0
                    while x < 100
                        y = x + 1
                    end
                 )",
                 "y = x + 1\n"
             },
             {
                 R"(
                    x = 0
                    while x < 100
                        x = z
                    end
                    y = z
                 )",
                 "y = z\n"
             },
             {
                 R"(
                    x = 12
                    if t > 0
                        z = x
                    end
                 )",
                 "z = x\n"
             },
             {
                 R"(
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
                 )",
                 "y = 4\n"
                 "z = x\n"
                 "y = 10\n"
             },
             {
                 R"(
                    while 1 > 0
                        x = y
                        y = z
                        z = x
                    end
                 )",
                 ""
             },
             {
                 R"(
                    if 1 > 0
                        x = y
                        y = z
                        z = x
                    end
                 )",
                 "y = z\n"
                 "z = x\n"
             }
        }));

    CAPTURE(input);
    auto result = find_unused_and_dump(input);
    REQUIRE(result == expected);
}

TEST_CASE("1", "[1]") {
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

    auto parser_result = parser::parse(str);

    if (std::holds_alternative<lexer::error>(parser_result)) {
        auto err = std::get<lexer::error>(parser_result);
        std::cerr << "Error: " << err.cause << " at pos " << err.pos << std::endl;
//        return 1;
    }
    auto tree = std::get<parser::ast::tree>(parser_result);

    auto unused = find_unused_assignments(tree, str);
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
}