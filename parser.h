#pragma once

#include <vector>
#include <cassert>
#include "lexer.h"


namespace parser {

    namespace ast {

        enum class kind {
            IF,
            WHILE,
            ASSIGNMENT,
            VAR,
            CONST,
            BINOP,
            STATEMENTS,
        };

        auto to_string(kind l) {
            switch (l) {
                case kind::IF: return "IF";
                case kind::WHILE: return "WHILE";
                case kind::ASSIGNMENT: return "ASSIGNMENT";
                case kind::VAR: return "VAR";
                case kind::CONST: return "CONST";
                case kind::BINOP: return "BINOP";
                case kind::STATEMENTS: return "STATEMENTS";
            }
            return "";
        }

        bool constexpr is_statement(kind k) {
            return k == kind::ASSIGNMENT || k == kind::IF || k == kind::WHILE;
        }

        bool constexpr is_scope(kind k) {
            return k == kind::IF || k == kind::WHILE;
        }


        bool constexpr is_expression(kind k) {
            return k == kind::BINOP || k == kind::VAR || k == kind::CONST;
        }


        struct tree {
            uint32_t root;

            struct node {
                static constexpr uint32_t npos = -1;

                kind type;
                uint32_t op1 = npos, op2 = npos, par = npos;

                void print(tree * t, std::string offset) {
                    std::cout << offset << "Kind(" << to_string(type) << ")" << std::endl;
                    if (op2 != npos) {
                        t->get_node(op2)->print(t, offset + "    ");
                    }
                    if (op1 != npos) {
                        t->get_node(op1)->print(t, offset + "    ");
                    }
                }
            };

            uint32_t new_node(kind k) {
                nodes.push_back(node{.type = k});
                return nodes.size() - 1;
            }

            void set_left(uint32_t par_idx, uint32_t ch_idx) {
                auto parent = get_node(par_idx);
                auto child = get_node(ch_idx);
                parent->op1 = ch_idx;
                child->par = par_idx;
            }

            void set_right(uint32_t par_idx, uint32_t ch_idx) {
                auto parent = get_node(par_idx);
                auto child = get_node(ch_idx);
                parent->op2 = ch_idx;
                child->par = par_idx;
            }

            node * left_child(node const * node) {
                return &nodes[node->op1];
            }

            node * right_child(node const * node) {
                return &nodes[node->op2];
            }

            node * parent(node const * node) {
                return &nodes[node->par];
            }

            node * get_node(uint32_t idx) {
                return &nodes[idx];
            }

            bool have_right(uint32_t idx) {
                auto nd = get_node(idx);
                return nd->op2 != node::npos;
            }

            bool is_scope_unfinished(uint32_t idx) {
                return is_scope(get_node(idx)->type) && !have_right(idx);
            }

            std::vector<node> nodes;

            void print() {
                get_node(root)->print(this, "");
            }
        };

    }

    uint32_t parse_expression(ast::tree & tree, std::vector<lexer::token>::const_iterator & it) {
        auto & tok = *(it++);
        switch (tok.type) {
            case lexer::kind::IDENTIFIER:
                return tree.new_node(ast::kind::VAR);
            case lexer::kind::CONSTANT:
                return tree.new_node(ast::kind::CONST);
            default:
                break;
        }
        return ast::tree::node::npos;
    }

    ast::tree parse(std::vector<lexer::token> const & tokens) {
        ast::tree result;

        std::vector<uint32_t> pending;

        for (auto it = tokens.begin(); it != tokens.end(); ) {
            switch (it->type) {
                case lexer::kind::WHITESPACE: {
                    ++it;
                    continue;
                }

                case lexer::kind::IF: {
                    auto idx = result.new_node(ast::kind::IF);
                    pending.push_back(idx);
                    result.set_left(idx, parse_expression(result, it)); // todo
                    break;
                }

                case lexer::kind::WHILE: {
                    auto idx = result.new_node(ast::kind::WHILE);
                    pending.push_back(idx);
                    result.set_left(idx, parse_expression(result, it)); // todo
                    break;
                }

                case lexer::kind::END: {
                    auto statements = pending.back();
                    pending.pop_back();

                    while (!result.is_scope_unfinished(pending.back())) {
                        auto seq = result.new_node(ast::kind::STATEMENTS);
                        result.set_left(seq, pending.back());
                        result.set_right(seq, statements);
                        statements = seq;
                        pending.pop_back();
                    }

                    result.set_right(pending.back(), statements);
                    ++it;
                    break;
                }

                default: {
                    ++it;

                    auto node = result.new_node(ast::kind::ASSIGNMENT);
                    if (it->type == lexer::kind::WHITESPACE) ++it;
                    assert(it->type == lexer::kind::ASSIGNMENT);
                    ++it;
                    if (it->type == lexer::kind::WHITESPACE) ++it;

                    auto expr = parse_expression(result, it);
                    result.set_left(node, result.new_node(ast::kind::VAR));
                    result.set_right(node, expr);

                    pending.push_back(node);
                    break;
                }
            }
        }

        result.root = pending.back();

        return result;
    }

}