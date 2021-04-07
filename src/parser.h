#pragma once

#include <vector>
#include "lexer.h"

namespace parser {

    namespace ast {
        class tree;
    }

    namespace detail {
        uint16_t parse_expression_from_token_list(
                ast::tree &tree, lexer::token_storage & storage, std::string_view sv);
        ast::tree parse_from_token_list(lexer::token_storage & tokens, std::string_view sv);
    }

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

        class tree {
            struct node {
                static constexpr uint16_t npos = -1;

                kind type{};
                uint16_t op1 = npos;
                uint16_t op2 = npos;
                uint16_t par = npos;

                uint32_t start_pos = -1;
                uint32_t end_pos = 0;

                lexer::operator_type op_type = lexer::operator_type::UNDEFINED;
            };

            uint16_t new_node(kind k, uint32_t start, uint32_t end) {
                nodes_.push_back(node{
                    .type = k,
                    .start_pos = start,
                    .end_pos = end
                });
                return nodes_.size() - 1;
            }

            uint16_t new_node_binop(lexer::operator_type type, uint32_t start, uint8_t end) {
                nodes_.push_back(node{
                        .type = kind::BINOP,
                        .start_pos = start,
                        .end_pos = end,
                        .op_type = type
                });
                return nodes_.size() - 1;
            }

            void set_left(uint16_t par_idx, uint16_t ch_idx) {
                auto parent = get_node(par_idx);
                auto child = get_node(ch_idx);
                parent->op1 = ch_idx;
                child->par = par_idx;
            }

            void set_right(uint16_t par_idx, uint16_t ch_idx) {
                auto parent = get_node(par_idx);
                auto child = get_node(ch_idx);
                parent->op2 = ch_idx;
                child->par = par_idx;
            }

            node * get_node(uint16_t idx) {
                return &nodes_[idx];
            }

            [[nodiscard]]
            node const * get_node(uint16_t idx) const {
                return &nodes_[idx];
            }

            bool is_scope_unfinished(uint16_t idx) {
                auto nod = get_node(idx);
                return (nod->type == kind::IF || nod->type == kind::WHILE) && nod->op2 == node::npos;
            }

        public:

            [[nodiscard]]
            uint16_t get_root() const {
                return root_;
            }

            [[nodiscard]]
            uint16_t get_left(uint16_t idx) const {
                return get_node(idx)->op1;
            }

            [[nodiscard]]
            uint16_t get_right(uint16_t idx) const {
                return get_node(idx)->op2;
            }

            [[nodiscard]]
            uint16_t get_parent(uint16_t idx) const {
                return get_node(idx)->par;
            }

            [[nodiscard]]
            uint16_t have_left(uint16_t idx) const {
                return get_left(idx) != node::npos;
            }

            [[nodiscard]]
            uint16_t have_right(uint16_t idx) const {
                return get_right(idx) != node::npos;
            }

            [[nodiscard]]
            uint16_t have_parent(uint16_t idx) const {
                return get_parent(idx) != node::npos;
            }

            [[nodiscard]]
            lexer::operator_type get_operator_type(uint16_t idx) const {
                return get_node(idx)->op_type;
            }

            [[nodiscard]]
            kind get_kind(uint16_t idx) const {
                return get_node(idx)->type;
            }

            [[nodiscard]]
            std::pair<uint16_t, uint16_t> get_range(uint16_t idx) const {
                auto node = get_node(idx);
                return {node->start_pos, node->end_pos};
            }

            [[nodiscard]]
            auto get_string(uint16_t idx, std::string_view sv) const {
                auto [from, to] = get_range(idx);
                return sv.substr(from, to - from);
            }

        private:
            std::vector<node> nodes_;
            uint16_t root_ = 0;

            friend uint16_t detail::parse_expression_from_token_list(
                    ast::tree &tree, lexer::token_storage & storage, std::string_view sv);
            friend ast::tree detail::parse_from_token_list(lexer::token_storage & tokens, std::string_view sv);
        };

    }

    uint8_t get_operator_priority(lexer::operator_type t) {
        switch (t) {
            case lexer::PLUS:
            case lexer::MINUS:
                return 5;
            case lexer::MULTIPLICATION:
            case lexer::DIVISION:
                return 6;
            default:
                return 0;
        }
    }

    namespace detail {
        uint16_t parse_expression_from_token_list(ast::tree &tree, lexer::token_storage & storage, std::string_view sv) {
            std::vector<uint16_t> nodes;
            std::vector<std::pair<lexer::token, uint8_t>> pending;

            auto apply_operator_from_stack = [&]() {
                auto op = pending.back();
                pending.pop_back();

                auto op1 = nodes.back();
                nodes.pop_back();

                auto op2 = nodes.back();
                nodes.pop_back();

                auto binop = tree.new_node_binop(
                    lexer::get_operator_type(op.first, sv),
                    tree.get_node(op2)->start_pos,
                    tree.get_node(op1)->end_pos
                );
                nodes.push_back(binop);
                tree.set_left(binop, op2);
                tree.set_right(binop, op1);
            };

            bool ok = true;
            while (ok && storage) {
                lexer::token tok = storage.peek();

                switch (tok.type) {
                    case lexer::kind::IDENTIFIER:
                        nodes.push_back(tree.new_node(ast::kind::VAR, tok.begin, tok.begin + tok.len));
                        break;

                    case lexer::kind::CONSTANT:
                        nodes.push_back(tree.new_node(ast::kind::CONST, tok.begin, tok.begin + tok.len));
                        break;

                    case lexer::kind::OPEN:
                        pending.emplace_back(tok, 10);
                        break;

                    case lexer::kind::CLOSE:
                        while (pending.back().first.type != lexer::kind::OPEN) {
                            apply_operator_from_stack();
                        }
                        tree.get_node(nodes.back())->start_pos--;
                        tree.get_node(nodes.back())->end_pos++;
                        pending.pop_back();
                        break;

                    case lexer::kind::OPERATOR: {
                        auto priority = get_operator_priority(lexer::get_operator_type(tok, sv));
                        while (!pending.empty()) {
                            auto[p_tok, p_pri] = pending.back();
                            if (p_pri > priority) {
                                break;
                            }
                            apply_operator_from_stack();
                        }

                        pending.emplace_back(tok, priority);
                        break;
                    }

                    case lexer::kind::EXPRESSION_FINISH_META:
                        storage.next();
                        ok = false;

                    default:
                        ok = false;
                        break;
                }

                if (ok) storage.next();
            }

            while (!pending.empty()) {
                apply_operator_from_stack();
            }

            return nodes.back();
        }

        ast::tree parse_from_token_list(lexer::token_storage & tokens, std::string_view sv) {
            ast::tree tree;

            std::vector<uint16_t> pending;

            while (tokens) {
                auto tok = tokens.next();
                switch (tok.type) {
                    case lexer::kind::IF: {
                        auto idx = tree.new_node(ast::kind::IF, tok.begin, tok.begin + tok.len);
                        pending.push_back(idx);
                        tree.set_left(idx, parse_expression_from_token_list(tree, tokens, sv));
                        break;
                    }

                    case lexer::kind::WHILE: {
                        auto idx = tree.new_node(ast::kind::WHILE, tok.begin, tok.begin + tok.len);
                        pending.push_back(idx);
                        tree.set_left(idx, parse_expression_from_token_list(tree, tokens, sv));
                        break;
                    }

                    case lexer::kind::END: {
                        auto statements = pending.back();
                        pending.pop_back();

                        while (!tree.is_scope_unfinished(pending.back())) {
                            auto seq = tree.new_node(
                                    ast::kind::STATEMENTS,
                                    tree.get_node(pending.back())->start_pos,
                                    tree.get_node(statements)->end_pos
                            );
                            tree.set_left(seq, pending.back());
                            tree.set_right(seq, statements);
                            statements = seq;
                            pending.pop_back();
                        }

                        tree.set_right(pending.back(), statements);
                        tree.get_node(pending.back())->end_pos = tok.begin + tok.len;
                        break;
                    }

                    default: {
                        tokens.next(); // token '='
                        auto expr = parse_expression_from_token_list(tree, tokens, sv);

                        auto node = tree.new_node(ast::kind::ASSIGNMENT, tok.begin, tree.get_node(expr)->end_pos);

                        tree.set_left(node, tree.new_node(ast::kind::VAR, tok.begin, tok.begin + tok.len));
                        tree.set_right(node, expr);
                        pending.push_back(node);
                        break;
                    }
                }
            }

            while (pending.size() != 1) {
                auto snd = pending.back();
                pending.pop_back();

                auto fst = pending.back();
                pending.pop_back();

                pending.push_back(
                        tree.new_node(
                                ast::kind::STATEMENTS,
                                tree.get_node(fst)->start_pos,
                                tree.get_node(snd)->end_pos
                        )
                );

                tree.set_left(pending.back(), fst);
                tree.set_right(pending.back(), snd);
            }

            tree.root_ = pending.back();

            return tree;
        }
    }

    std::variant<ast::tree, lexer::error> parse(std::string_view sv) {
        std::vector<lexer::token> tokens;
        auto result = lexer::program::parse(tokens, 0, sv);

        if (std::holds_alternative<lexer::error>(result)) {
            return std::get<lexer::error>(result);
        }

        lexer::token_storage storage(tokens);
        return detail::parse_from_token_list(storage, sv);
    }

}