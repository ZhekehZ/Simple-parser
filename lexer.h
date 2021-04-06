#pragma once
#include <array>
#include <vector>
#include <optional>

namespace lexer {

    struct token {
        uint32_t begin, len;
    };

    template <typename T>
    concept Lexer = requires (std::vector<token> & vector, uint32_t uint, std::string_view string_view) {
        { T::parse(vector, uint, string_view) } -> std::same_as<std::optional<uint32_t>>;
    };

    template<uint32_t LEN, char const (&TOK)[LEN]>
    struct token_match {
        static std::optional<uint32_t> parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            if (str.empty()) {
                return std::nullopt;
            }

            for (auto len = 0u; len < LEN; ++len) {
                if (str[pos + len] != TOK[len]) {
                    return std::nullopt;
                }
            }

            output.template emplace_back(pos, LEN);
            return {pos + LEN};
        }
    };

    template<uint32_t LEN, char const (&CHARS)[LEN]>
    struct any_char {
        static std::optional<uint32_t> parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            if (str.empty()) {
                return std::nullopt;
            }

            for (auto idx = 0u; idx < LEN; ++idx) {
                if (str[pos + idx] == CHARS[idx]) {
                    output.template emplace_back(pos, 1);
                    return {pos + 1};
                }
            }

            return std::nullopt;
        }
    };

    template<int (*GOOD_CHAR)(int)>
    struct symbols {
        static std::optional<uint32_t> parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            if (str.empty() || !GOOD_CHAR(str[pos])) {
                return std::nullopt;
            }

            auto curr_pos = pos;
            while (curr_pos < str.size() && GOOD_CHAR(str[curr_pos])) {
                ++curr_pos;
            }

            output.template emplace_back(curr_pos, curr_pos - pos);
            return {curr_pos};
        }
    };

    template <Lexer ... LEXERS>
    struct sequence {
        static std::optional<uint32_t> parse(std::vector<token> & output, uint32_t pos, std::string_view str) {
            std::optional<uint32_t> status(pos);
            (void) (
                ( status = LEXERS::parse(output, status.value(), str)
                , status.has_value()
                ) && ...
            );
            return status;
        }
    };

    namespace token_strings {
        constexpr char IF[] = "if";
        constexpr char WHILE[] = "while";
        constexpr char END[] = "end";
        constexpr char WHITE[] = " \t\r\n";
    }

#define MAKE_TOKEN_LEX(str) token_match<sizeof(str), str>
#define MAKE_ANYCHAR_LEX(str) any_char<sizeof(str), str>

    using identifier = symbols<std::isalpha>;
    using constant = symbols<std::isdigit>;
    using whitespace = MAKE_ANYCHAR_LEX(token_strings::WHITE);
    using k_if = MAKE_TOKEN_LEX(token_strings::IF);
    using k_while = MAKE_TOKEN_LEX(token_strings::WHILE);
    using k_end = MAKE_TOKEN_LEX(token_strings::END);



#undef MAKE_ANYCHAR_LEX
#undef MAKE_TOKEN_LEX

}
