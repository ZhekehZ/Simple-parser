#pragma once

#include <map>
#include <set>
#include "parser.h"
#include "lexer.h"

namespace detail {

    void get_free_variables_in_expression_helper(
        std::set<std::string> & tokens,
        parser::ast::tree const & tree,
        uint32_t expression,
        std::string_view sv
    ) {
        sw: switch (tree.get_kind(expression)) {
            case parser::ast::kind::VAR: {
                tokens.insert(std::string(tree.get_string(expression, sv)));
                break;
            }
            case parser::ast::kind::BINOP: {
                get_free_variables_in_expression_helper(tokens, tree, tree.get_left(expression), sv);
                expression = tree.get_right(expression);
                goto sw;
            }
            default:
                break;
        }
    }

    std::set<std::string> get_free_variables_in_expression(
            parser::ast::tree const & tree,
            uint32_t expression,
            std::string_view sv
    ) {
        std::set<std::string> tokens;
        get_free_variables_in_expression_helper(tokens, tree, expression, sv);
        return tokens;
    }

    void calculate_free_variables_for_scopes_helper(
        parser::ast::tree const & tree,
        uint32_t node,
        uint32_t current_scope,
        std::string_view sv,
        std::map<uint32_t, std::set<std::string>> & frees,
        std::set<std::string> & binded
    ) {
        auto process_expression = [&](uint32_t idx) {
            auto free_expr = get_free_variables_in_expression(tree, idx, sv);
            for (auto const & b : binded) {
                free_expr.erase(b);
            }
            frees[current_scope].insert(free_expr.begin(), free_expr.end());
        };

        repeat: switch (tree.get_kind(node)) {
            case parser::ast::kind::IF: {
                process_expression(tree.get_left(node));
                node = tree.get_right(node);
                goto repeat;
            }
            case parser::ast::kind::WHILE: {
                frees[node] = get_free_variables_in_expression(tree, tree.get_left(node), sv);
                std::set<std::string> new_binded;
                calculate_free_variables_for_scopes_helper(
                        tree, tree.get_right(node), node, sv, frees, new_binded);
                for (auto const &f : frees[node]) {
                    if (binded.find(f) == binded.end()) {
                        frees[current_scope].insert(f);
                    }
                }
                break;
            }
            case parser::ast::kind::ASSIGNMENT: {
                process_expression(tree.get_right(node));
                binded.insert(std::string(tree.get_string(tree.get_left(node), sv)));
                break;
            }
            case parser::ast::kind::STATEMENTS: {
                calculate_free_variables_for_scopes_helper(
                        tree, tree.get_left(node), current_scope, sv, frees, binded);
                node = tree.get_right(node);
                goto repeat;
            }
            default:
                break;
        }
    }

    std::map<uint32_t, std::set<std::string>> calculate_free_variables_for_scopes(
        parser::ast::tree const & tree,
        std::string_view sv
    ) {
        std::map<uint32_t, std::set<std::string>> frees;
        std::set<std::string> binded;
        calculate_free_variables_for_scopes_helper(tree, tree.get_root(), tree.get_root(), sv, frees, binded);
        return frees;
    }


    void find_unused_assignments_helper(
        parser::ast::tree const & tree,
        uint32_t node,
        std::map<uint32_t, std::set<std::string>> const & frees,
        std::vector<uint32_t> & unused,
        std::map<std::string, uint32_t> & pending,
        std::string_view sv
    ) {
        repeat: switch (tree.get_kind(node)) {
            case parser::ast::kind::IF: {
                for (const auto & s : get_free_variables_in_expression(tree, tree.get_left(node), sv)) {
                    pending.erase(std::string(s));
                }
                node = tree.get_right(node);
                goto repeat;
            }
            case parser::ast::kind::WHILE: {
                for (const auto & s : get_free_variables_in_expression(tree, tree.get_left(node), sv)) {
                    pending.erase(std::string(s));
                }
                find_unused_assignments_helper(tree, tree.get_right(node), frees, unused, pending, sv);
                for (const auto & s : frees.at(node)) {
                    pending.erase(s);
                }
                break;
            }
            case parser::ast::kind::ASSIGNMENT: {
                std::string var(tree.get_string(tree.get_left(node), sv));
                auto it = pending.find(var);
                if (it != pending.end()) {
                    unused.push_back(it->second);
                }
                for (const auto &s : get_free_variables_in_expression(tree, tree.get_right(node), sv)) {
                    pending.erase(std::string(s));
                }
                pending[var] = node;
                break;
            }
            case parser::ast::kind::STATEMENTS: {
                find_unused_assignments_helper(tree, tree.get_left(node), frees, unused, pending, sv);
                node = tree.get_right(node);
                goto repeat;
            }
            default:
                break;
        }
    }

}

std::vector<uint32_t> find_unused_assignments(
    parser::ast::tree const & tree,
    std::string_view sv
) {
    std::vector<uint32_t> unused;
    std::map<std::string, uint32_t> pending;
    auto frees = detail::calculate_free_variables_for_scopes(tree, sv);
    detail::find_unused_assignments_helper(tree, tree.get_root(), frees, unused, pending, sv);
    for (auto const & [_, idx] : pending) {
        unused.push_back(idx);
    }
    return unused;
}
