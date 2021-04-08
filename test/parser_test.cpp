#include <catch2/catch.hpp>
#include <string>
#include <pretty_print.h>
#include <lexer.h>
#include <sstream>

std::string parse_and_dump(std::string const & s) {
    std::stringstream ss;
    auto parsed = parser::parse(s);
    REQUIRE(std::holds_alternative<parser::ast::tree>(parsed));
    printer::print(ss, std::get<parser::ast::tree>(parsed), s);
    return ss.str();
}

lexer::error parse_and_get_error(std::string const & s) {
    std::stringstream ss;
    auto parsed = parser::parse(s);
    REQUIRE(std::holds_alternative<lexer::error>(parsed));
    return std::get<lexer::error>(parsed);
}

TEST_CASE ("Parser simple test", "[parser]") {
    std::string input, expected_output;

    std::tie(input, expected_output) =
        GENERATE(table<std::string, std::string>({
             {"x=y",                                      "{ ASSIGNMENT [0..3] 'x=y'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { VAR [2..3] 'y'}\n"
                                                          "}\n"
                                                          ""},
             {"x = y",                                    "{ ASSIGNMENT [0..5] 'x = y'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { VAR [4..5] 'y'}\n"
                                                          "}\n"
                                                          ""},
             {"x = y y = z",                              "{ STATEMENTS [0..11] 'x = y y = z'\n"
                                                          "   { ASSIGNMENT [0..5] 'x = y'\n"
                                                          "      { VAR [0..1] 'x'}\n"
                                                          "      { VAR [4..5] 'y'}\n"
                                                          "   }\n"
                                                          "   { ASSIGNMENT [6..11] 'y = z'\n"
                                                          "      { VAR [6..7] 'y'}\n"
                                                          "      { VAR [10..11] 'z'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = y y = z z = t",                        "{ STATEMENTS [0..17] 'x = y y = z z = t'\n"
                                                          "   { ASSIGNMENT [0..5] 'x = y'\n"
                                                          "      { VAR [0..1] 'x'}\n"
                                                          "      { VAR [4..5] 'y'}\n"
                                                          "   }\n"
                                                          "   { STATEMENTS [6..17] 'y = z z = t'\n"
                                                          "      { ASSIGNMENT [6..11] 'y = z'\n"
                                                          "         { VAR [6..7] 'y'}\n"
                                                          "         { VAR [10..11] 'z'}\n"
                                                          "      }\n"
                                                          "      { ASSIGNMENT [12..17] 'z = t'\n"
                                                          "         { VAR [12..13] 'z'}\n"
                                                          "         { VAR [16..17] 't'}\n"
                                                          "      }\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = 123",                                  "{ ASSIGNMENT [0..7] 'x = 123'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { CONST [4..7] '123'}\n"
                                                          "}\n"
                                                          ""},
             {"hello = world",                            "{ ASSIGNMENT [0..13] 'hello = world'\n"
                                                          "   { VAR [0..5] 'hello'}\n"
                                                          "   { VAR [8..13] 'world'}\n"
                                                          "}\n"
                                                          ""},
             {"x = 2 + 2",                                "{ ASSIGNMENT [0..9] 'x = 2 + 2'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP PLUS [4..9] '2 + 2'\n"
                                                          "      { CONST [4..5] '2'}\n"
                                                          "      { CONST [8..9] '2'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = 2 + y",                                "{ ASSIGNMENT [0..9] 'x = 2 + y'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP PLUS [4..9] '2 + y'\n"
                                                          "      { CONST [4..5] '2'}\n"
                                                          "      { VAR [8..9] 'y'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = a+b+c",                                "{ ASSIGNMENT [0..9] 'x = a+b+c'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP PLUS [4..9] 'a+b+c'\n"
                                                          "      { BINOP PLUS [4..7] 'a+b'\n"
                                                          "         { VAR [4..5] 'a'}\n"
                                                          "         { VAR [6..7] 'b'}\n"
                                                          "      }\n"
                                                          "      { VAR [8..9] 'c'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = a+b*c",                                "{ ASSIGNMENT [0..9] 'x = a+b*c'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP MULTIPLICATION [4..9] 'a+b*c'\n"
                                                          "      { BINOP PLUS [4..7] 'a+b'\n"
                                                          "         { VAR [4..5] 'a'}\n"
                                                          "         { VAR [6..7] 'b'}\n"
                                                          "      }\n"
                                                          "      { VAR [8..9] 'c'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = a*b+c",                                "{ ASSIGNMENT [0..9] 'x = a*b+c'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP MULTIPLICATION [4..9] 'a*b+c'\n"
                                                          "      { VAR [4..5] 'a'}\n"
                                                          "      { BINOP PLUS [6..9] 'b+c'\n"
                                                          "         { VAR [6..7] 'b'}\n"
                                                          "         { VAR [8..9] 'c'}\n"
                                                          "      }\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = y > z",                                "{ ASSIGNMENT [0..9] 'x = y > z'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP GREATER [4..9] 'y > z'\n"
                                                          "      { VAR [4..5] 'y'}\n"
                                                          "      { VAR [8..9] 'z'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = y * z / 2",                            "{ ASSIGNMENT [0..13] 'x = y * z / 2'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP DIVISION [4..13] 'y * z / 2'\n"
                                                          "      { BINOP MULTIPLICATION [4..9] 'y * z'\n"
                                                          "         { VAR [4..5] 'y'}\n"
                                                          "         { VAR [8..9] 'z'}\n"
                                                          "      }\n"
                                                          "      { CONST [12..13] '2'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"x = 1-2-3",                                "{ ASSIGNMENT [0..9] 'x = 1-2-3'\n"
                                                          "   { VAR [0..1] 'x'}\n"
                                                          "   { BINOP MINUS [4..9] '1-2-3'\n"
                                                          "      { BINOP MINUS [4..7] '1-2'\n"
                                                          "         { CONST [4..5] '1'}\n"
                                                          "         { CONST [6..7] '2'}\n"
                                                          "      }\n"
                                                          "      { CONST [8..9] '3'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"if x > 0 x = 0 end",                       "{ IF [0..18] 'if x > 0 x = 0 end'\n"
                                                          "   { BINOP GREATER [3..8] 'x > 0'\n"
                                                          "      { VAR [3..4] 'x'}\n"
                                                          "      { CONST [7..8] '0'}\n"
                                                          "   }\n"
                                                          "   { ASSIGNMENT [9..14] 'x = 0'\n"
                                                          "      { VAR [9..10] 'x'}\n"
                                                          "      { CONST [13..14] '0'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"while x > 0 x = 0 end",                    "{ WHILE [0..21] 'while x > 0 x = 0 end'\n"
                                                          "   { BINOP GREATER [6..11] 'x > 0'\n"
                                                          "      { VAR [6..7] 'x'}\n"
                                                          "      { CONST [10..11] '0'}\n"
                                                          "   }\n"
                                                          "   { ASSIGNMENT [12..17] 'x = 0'\n"
                                                          "      { VAR [12..13] 'x'}\n"
                                                          "      { CONST [16..17] '0'}\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"while x > 0 if x > 1 x = 0 end end",       "{ WHILE [0..34] 'while x > 0 if x > 1 x = 0 end end'\n"
                                                          "   { BINOP GREATER [6..11] 'x > 0'\n"
                                                          "      { VAR [6..7] 'x'}\n"
                                                          "      { CONST [10..11] '0'}\n"
                                                          "   }\n"
                                                          "   { IF [12..30] 'if x > 1 x = 0 end'\n"
                                                          "      { BINOP GREATER [15..20] 'x > 1'\n"
                                                          "         { VAR [15..16] 'x'}\n"
                                                          "         { CONST [19..20] '1'}\n"
                                                          "      }\n"
                                                          "      { ASSIGNMENT [21..26] 'x = 0'\n"
                                                          "         { VAR [21..22] 'x'}\n"
                                                          "         { CONST [25..26] '0'}\n"
                                                          "      }\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
             {"while x > 0 x = 4 if x > 1 x = 0 end end", "{ WHILE [0..40] 'while x > 0 x = 4 if x > 1 x = 0 end end'\n"
                                                          "   { BINOP GREATER [6..11] 'x > 0'\n"
                                                          "      { VAR [6..7] 'x'}\n"
                                                          "      { CONST [10..11] '0'}\n"
                                                          "   }\n"
                                                          "   { STATEMENTS [12..36] 'x = 4 if x > 1 x = 0 end'\n"
                                                          "      { ASSIGNMENT [12..17] 'x = 4'\n"
                                                          "         { VAR [12..13] 'x'}\n"
                                                          "         { CONST [16..17] '4'}\n"
                                                          "      }\n"
                                                          "      { IF [18..36] 'if x > 1 x = 0 end'\n"
                                                          "         { BINOP GREATER [21..26] 'x > 1'\n"
                                                          "            { VAR [21..22] 'x'}\n"
                                                          "            { CONST [25..26] '1'}\n"
                                                          "         }\n"
                                                          "         { ASSIGNMENT [27..32] 'x = 0'\n"
                                                          "            { VAR [27..28] 'x'}\n"
                                                          "            { CONST [31..32] '0'}\n"
                                                          "         }\n"
                                                          "      }\n"
                                                          "   }\n"
                                                          "}\n"
                                                          ""},
     }));

    CAPTURE(input);
    REQUIRE(parse_and_dump(input) == expected_output);

}

TEST_CASE ("Parser extra case", "[parser]") {
    auto program = R"(
x = 12
y = 14
z = 15

while x * (y + z) < 12000
    while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end
end
)";
   auto expected = R"({ STATEMENTS [1..227] 'x = 12
y = 14
z = 15

while x * (y + z) < 12000
    while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end
end'
   { ASSIGNMENT [1..7] 'x = 12'
      { VAR [1..2] 'x'}
      { CONST [5..7] '12'}
   }
   { STATEMENTS [8..227] 'y = 14
z = 15

while x * (y + z) < 12000
    while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end
end'
      { ASSIGNMENT [8..14] 'y = 14'
         { VAR [8..9] 'y'}
         { CONST [12..14] '14'}
      }
      { STATEMENTS [15..227] 'z = 15

while x * (y + z) < 12000
    while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end
end'
         { ASSIGNMENT [15..21] 'z = 15'
            { VAR [15..16] 'z'}
            { CONST [19..21] '15'}
         }
         { WHILE [23..227] 'while x * (y + z) < 12000
    while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end
end'
            { BINOP MULTIPLICATION [29..48] 'x * (y + z) < 12000'
               { VAR [29..30] 'x'}
               { BINOP LESS [33..48] '(y + z) < 12000'
                  { BINOP PLUS [33..40] '(y + z)'
                     { VAR [34..35] 'y'}
                     { VAR [38..39] 'z'}
                  }
                  { CONST [43..48] '12000'}
               }
            }
            { STATEMENTS [53..223] 'while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end
    if x > y
        z = 15
    end'
               { WHILE [53..187] 'while x > 0
        if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125
    end'
                  { BINOP GREATER [59..64] 'x > 0'
                     { VAR [59..60] 'x'}
                     { CONST [63..64] '0'}
                  }
                  { STATEMENTS [73..179] 'if y > 3 x = x + 1 end
        if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125'
                     { IF [73..95] 'if y > 3 x = x + 1 end'
                        { BINOP GREATER [76..81] 'y > 3'
                           { VAR [76..77] 'y'}
                           { CONST [80..81] '3'}
                        }
                        { ASSIGNMENT [82..91] 'x = x + 1'
                           { VAR [82..83] 'x'}
                           { BINOP PLUS [86..91] 'x + 1'
                              { VAR [86..87] 'x'}
                              { CONST [90..91] '1'}
                           }
                        }
                     }
                     { STATEMENTS [104..179] 'if y < 3 y = y + 1 end
        z = z / 2
        unused = x * 123 + z * 125'
                        { IF [104..126] 'if y < 3 y = y + 1 end'
                           { BINOP LESS [107..112] 'y < 3'
                              { VAR [107..108] 'y'}
                              { CONST [111..112] '3'}
                           }
                           { ASSIGNMENT [113..122] 'y = y + 1'
                              { VAR [113..114] 'y'}
                              { BINOP PLUS [117..122] 'y + 1'
                                 { VAR [117..118] 'y'}
                                 { CONST [121..122] '1'}
                              }
                           }
                        }
                        { STATEMENTS [135..179] 'z = z / 2
        unused = x * 123 + z * 125'
                           { ASSIGNMENT [135..144] 'z = z / 2'
                              { VAR [135..136] 'z'}
                              { BINOP DIVISION [139..144] 'z / 2'
                                 { VAR [139..140] 'z'}
                                 { CONST [143..144] '2'}
                              }
                           }
                           { ASSIGNMENT [153..179] 'unused = x * 123 + z * 125'
                              { VAR [153..159] 'unused'}
                              { BINOP MULTIPLICATION [162..179] 'x * 123 + z * 125'
                                 { BINOP MULTIPLICATION [162..173] 'x * 123 + z'
                                    { VAR [162..163] 'x'}
                                    { BINOP PLUS [166..173] '123 + z'
                                       { CONST [166..169] '123'}
                                       { VAR [172..173] 'z'}
                                    }
                                 }
                                 { CONST [176..179] '125'}
                              }
                           }
                        }
                     }
                  }
               }
               { IF [192..223] 'if x > y
        z = 15
    end'
                  { BINOP GREATER [195..200] 'x > y'
                     { VAR [195..196] 'x'}
                     { VAR [199..200] 'y'}
                  }
                  { ASSIGNMENT [209..215] 'z = 15'
                     { VAR [209..210] 'z'}
                     { CONST [213..215] '15'}
                  }
               }
            }
         }
      }
   }
}
)";

   REQUIRE(parse_and_dump(program) == expected);
}

TEST_CASE ("Parser error test", "[parser]") {
    std::string input, expected_error;
    uint32_t expected_pos;

    std::tie(input, expected_error, expected_pos) =
            GENERATE(table<std::string, std::string, uint32_t>({
                 {"x=", "IDENTIFIER_OR_CONSTANT_EXPECTED", 2},
                 {"x=_", "IDENTIFIER_OR_CONSTANT_EXPECTED", 2},
                 {"x=(1", "UNCLOSED_PARENTHESIS", 4},
                 {"if x > 0 x = 2", "UNFINISHED_STATEMENT", 14},
                 {"  ", "STRING_IS_TOO_SHORT", 2},
                 {"white", "STRING_IS_TOO_SHORT", 5}
            }));

    CAPTURE(input);
    auto error = parse_and_get_error(input);
    REQUIRE(expected_error == error.cause);
    REQUIRE(expected_pos == error.pos);
}
