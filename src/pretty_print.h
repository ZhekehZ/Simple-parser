#pragma once

#include "lexer.h"
#include "parser.h"

namespace detail {
    auto kind_to_string(parser::ast::kind k) {
        switch (k) {
            case parser::ast::kind::IF:
                return "IF";
            case parser::ast::kind::WHILE:
                return "WHILE";
            case parser::ast::kind::ASSIGNMENT:
                return "ASSIGNMENT";
            case parser::ast::kind::VAR:
                return "VAR";
            case parser::ast::kind::CONST:
                return "CONST";
            case parser::ast::kind::BINOP:
                return "BINOP";
            case parser::ast::kind::STATEMENTS:
                return "STATEMENTS";
        }
        return "";
    }

    auto type_to_string(lexer::operator_type t) {
        switch (t) {
            case lexer::PLUS:
                return "PLUS";
            case lexer::MINUS:
                return "MINUS";
            case lexer::MULTIPLICATION:
                return "MULTIPLICATION";
            case lexer::DIVISION:
                return "DIVISION";
            case lexer::LESS:
                return "LESS";
            case lexer::GREATER:
                return "GREATER";
            case lexer::UNDEFINED:
                return "UNDEFINED";
        }
        return "";
    }

    void print(
            std::ostream &out,
            parser::ast::tree const &tree,
            std::string const &prefix,
            std::string_view sv,
            uint16_t node
    ) {
        auto op_type = tree.get_operator_type(node);
        auto[from, to] = tree.get_range(node);

        out << prefix << "{ " << kind_to_string(tree.get_kind(node))
            << (op_type == lexer::UNDEFINED ? "" : std::string(" ") + type_to_string(op_type))
            << " [" << from << ".." << to << "] '" << tree.get_string(node, sv) << "'";

        if (!tree.have_left(node) && !tree.have_right(node)) {
            out << '}' << std::endl;
            return;
        }

        out << std::endl;

        if (tree.have_left(node)) {
            print(out, tree, prefix + "   ", sv, tree.get_left(node));
        }
        if (tree.have_right(node)) {
            print(out, tree, prefix + "   ", sv, tree.get_right(node));
        }

        out << prefix << "}" << std::endl;
    }
}

namespace printer {
    void print(std::ostream &out, parser::ast::tree const &tree, std::string_view sv) {
        detail::print(out, tree, "", sv, tree.get_root());
    }
}

