#pragma once
#include <array>
#include <vector>
#include <variant>

namespace lexer {

    enum class kind {
        WHITESPACE, // \t\r\n...
        IDENTIFIER, // [a-zA-Z]+
        CONSTANT,   // [0-9]+
        ASSIGNMENT, // =
        OPEN,       // (
        CLOSE,      // )
        IF,         // if
        WHILE,      // while
        END,        // end
        OPERATOR,   // <>+-*/
    };

    struct token {
        uint32_t begin, len;
        kind type;
    };

    struct error {
        char const * cause;
        uint32_t pos;
    };

    using lexer_result = std::variant<uint32_t, error>;

    bool constexpr is_success(lexer_result const & l) {
        return std::holds_alternative<uint32_t>(l);
    }

    template<typename T>
    concept Lexer = requires(std::vector<token> &vector, uint32_t uint, std::string_view string_view) {
        { T::parse(vector, uint, string_view) } -> std::same_as<lexer_result>;
    };

    namespace errors {
    #define CREATE_ERROR(message) constexpr char message[] = #message

        CREATE_ERROR(STRING_IS_TOO_SHORT);
        CREATE_ERROR(INVALID_SYMBOL);
        CREATE_ERROR(IDENTIFIER_OR_CONSTANT_EXPECTED);
        CREATE_ERROR(UNCLOSED_PARENTHESIS);
        CREATE_ERROR(UNFINISHED_STATEMENT);

    #undef CREATE_ERROR
    }

    namespace combinators {

        template<uint32_t LEN, char const (&TOK)[LEN + 1], kind KIND>
        requires (LEN > 0)
        struct token_match {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                if (str.size() < pos + LEN) {
                    return error {
                        .cause = errors::STRING_IS_TOO_SHORT,
                        .pos = static_cast<uint32_t>(str.size()),
                    };
                }

                for (auto len = 0u; len < LEN; ++len) {
                    if (str[pos + len] != TOK[len]) {
                        return error {
                            .cause = errors::INVALID_SYMBOL,
                            .pos = pos + len,
                        };
                    }
                }

                output.emplace_back(pos, LEN, KIND);
                return pos + LEN;
            }
        };

        template<uint32_t LEN, char const (&CHARS)[LEN + 1], kind KIND>
        requires (LEN > 0)
        struct any_char {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                if (str.size() < pos + 1) {
                    return error {
                        .cause = errors::STRING_IS_TOO_SHORT,
                        .pos = static_cast<uint32_t>(str.size()),
                    };
                }

                for (auto idx = 0u; idx < LEN; ++idx) {
                    if (str[pos] == CHARS[idx]) {
                        output.emplace_back(pos, 1, KIND);
                        return pos + 1;
                    }
                }

                return error {
                    .cause = errors::INVALID_SYMBOL,
                    .pos = pos,
                };
            }
        };

        template<int (*GOOD_CHAR)(int), kind KIND>
        requires (bool(GOOD_CHAR))
        struct symbols {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                if (str.size() <= pos) {
                    return error {
                        .cause = errors::STRING_IS_TOO_SHORT,
                        .pos = static_cast<uint32_t>(str.size()),
                    };
                }

                if (!GOOD_CHAR(str[pos])) {
                    return error{
                        .cause = errors::INVALID_SYMBOL,
                        .pos = pos
                    };
                }

                auto curr_pos = pos;
                while (curr_pos < str.size() && GOOD_CHAR(str[curr_pos])) {
                    ++curr_pos;
                }

                output.emplace_back(pos, curr_pos - pos, KIND);
                return curr_pos;
            }
        };

        template<char SYM, kind KIND>
        struct symbol {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                if (str.size() <= pos) {
                    return error {
                        .cause = errors::STRING_IS_TOO_SHORT,
                        .pos = static_cast<uint32_t>(str.size()),
                    };
                }

                if (SYM != str[pos]) {
                    return error{
                        .cause = errors::INVALID_SYMBOL,
                        .pos = pos
                    };
                }

                output.emplace_back(pos, 1, KIND);
                return pos + 1;
            }
        };

        template<Lexer ... LEXERS>
        requires (sizeof ... (LEXERS) > 0)
        struct sequence {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                lexer_result status(pos);
                (void) (
                    (status = LEXERS::parse(output, std::get<uint32_t>(status), str)
                    , is_success(status)
                    ) && ...
                );
                return status;
            }
        };

        template<Lexer ... LEXERS>
        requires (sizeof ... (LEXERS) > 0)
        struct alternative {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                lexer_result status;
                auto initial_size = output.size();
                (void) (
                    (output.erase(output.begin() + initial_size, output.end()) // rollback changes
                    , status = LEXERS::parse(output, pos, str)
                    , is_success(status)
                    ) || ...
                );
                return status;
            }
        };

        template<Lexer LEXER, bool AT_LEAST_ONE = true, bool MERGE_TOKENS = false>
        struct many {
            static lexer_result parse(std::vector<token> &output, uint32_t pos, std::string_view str) {
                lexer_result status = LEXER::parse(output, pos, str);

                if (!is_success(status)) {
                    if constexpr (AT_LEAST_ONE) {
                        return status;
                    } else {
                        status = pos;
                    }
                }

                lexer_result new_status = status;
                while (is_success(new_status)) {
                    status = new_status;
                    new_status = LEXER::parse(output, std::get<uint32_t>(status), str);

                    if constexpr (MERGE_TOKENS) {
                        if (is_success(new_status)) {
                            auto last_token = output.back().len;
                            output.pop_back();
                            output.back().len += last_token;
                        }
                    }
                }
                return status;
            }
        };

    }

    namespace token_strings {
        constexpr char IF[] = "if";
        constexpr char WHILE[] = "while";
        constexpr char END[] = "end";
        constexpr char WHITESPACE[] = " \t\r\n";
        constexpr char OPERATOR[] = "><+-*/";
    }

#define MAKE_TOKEN_LEX(str, kind) combinators::token_match<sizeof(str) - 1, str, kind>
#define MAKE_ANYCHAR_LEX(str, kind) combinators::any_char<sizeof(str) - 1, str, kind>

    using identifier = combinators::symbols<std::isalpha, kind::IDENTIFIER>;
    using constant = combinators::symbols<std::isdigit, kind::CONSTANT>;
    using whitespaces = combinators::many<MAKE_ANYCHAR_LEX(token_strings::WHITESPACE, kind::WHITESPACE), false, true>;
    using k_if = MAKE_TOKEN_LEX(token_strings::IF, kind::IF);
    using k_while = MAKE_TOKEN_LEX(token_strings::WHILE, kind::WHILE);
    using k_end = MAKE_TOKEN_LEX(token_strings::END, kind::END);
    using operator_sym = MAKE_ANYCHAR_LEX(token_strings::OPERATOR, kind::OPERATOR);

#undef MAKE_ANYCHAR_LEX
#undef MAKE_TOKEN_LEX

    template <Lexer LEX>
    void maybe(std::vector<token> & output, uint32_t & pos, std::string_view str) {
        auto tok = LEX::parse(output, pos, str);
        if (is_success(tok)) {
            pos = std::get<uint32_t>(tok);
        }
    }

    struct expression {
        static lexer_result parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            using namespace combinators;

            auto opened = 0u;
            lexer_result tok;

            while (true) {
                if (is_success(tok = symbol<'(', kind::OPEN>::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);
                    ++opened;
                }

                maybe<whitespaces>(output, pos, str);

                if (is_success(tok = alternative<identifier, constant>::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);
                } else {
                    return error {
                        .cause = errors::IDENTIFIER_OR_CONSTANT_EXPECTED,
                        .pos = pos,
                    };
                }

                maybe<whitespaces>(output, pos, str);

                if (is_success(tok = symbol<')', kind::CLOSE>::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);
                    --opened;

                    maybe<whitespaces>(output, pos, str);
                }

                if (is_success(tok = operator_sym::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);
                    maybe<whitespaces>(output, pos, str);
                    continue;
                }

                return opened
                        ? error {
                            .cause = errors::UNCLOSED_PARENTHESIS,
                            .pos = pos,
                            }
                        : lexer_result(pos);
            }
        }
    };

    using assignment = combinators::sequence<
            identifier, whitespaces, combinators::symbol<'=', kind::ASSIGNMENT>, whitespaces, expression>; // id = expr

    struct statement {
        static lexer_result parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            using namespace combinators;

            auto opened = 0u;
            lexer_result tok;

            while (true) {
                if (is_success(tok = alternative<k_while, k_if>::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);
                    maybe<whitespaces>(output, pos, str);

                    if (!is_success(tok = expression::parse(output, pos, str))) {
                        return tok;
                    }
                    pos = std::get<uint32_t>(tok);

                    maybe<whitespaces>(output, pos, str);
                    ++opened;
                }

                if (is_success(tok = assignment::parse(output, pos, str))) {
                    pos = std::get<uint32_t>(tok);

                    maybe<whitespaces>(output, pos, str);

                    while (opened && is_success(tok = k_end::parse(output, pos, str))) {
                        pos = std::get<uint32_t>(tok);
                        maybe<whitespaces>(output, pos, str);
                        --opened;
                    }

                    if (opened) {
                        continue;
                    }

                    return {pos};
                }

                return error {
                    .cause = errors::UNFINISHED_STATEMENT,
                    .pos = pos,
                };
            }
        }
    };

    using program = combinators::many<statement>;
}
