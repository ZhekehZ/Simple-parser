#pragma once

#include <vector>
#include "lexer.h"

template <lexer::Lexer T>
struct many1 {
    auto parse(uint32_t start_pos, std::string_view str) {
        std::vector<lexer::token> results;

//        auto pos = start_pos;
//        while (true) {
//            lexer::token result = T::parse(pos, str);
////            if (!result.) {
////                return result;
////            }
//        }
    }
};