#pragma once

#include <queue>
#include <memory>
#include <cassert>
#include <functional>

#include <cmath>

#include "common_types.h"

#include "tokenizer.hpp"

namespace parser
{
    struct ExprAST
    {
        enum class Type
        {
            Nothing,
            Variable,
            Number,
            Operator,
            UnaryOperator,
            Function,
        } type;

        union Data {

            Data() { }

            ~Data() { }

            double value;
            char variable;

            struct Ptrs {
                std::unique_ptr<ExprAST> left;
                std::unique_ptr<ExprAST> right;
                types::Operators op;
                types::Functions fun;
            } ptr;
        } data;
    };


    std::ostream& operator<<(std::ostream& out, ExprAST::Type type)
    {
        out << (int)type;
        return out;
    }


    std::unique_ptr<ExprAST> read_factor(std::queue<tokenizer::Token>& tokens);
    std::unique_ptr<ExprAST> read_term(std::queue<tokenizer::Token>& tokens);
    std::unique_ptr<ExprAST> read_expr(std::queue<tokenizer::Token>& tokens);

    // factor: NUM
    // |       VAR
    // |       ( expr )
    // |       function: NAME( expr )
    // ;
    std::unique_ptr<ExprAST> read_factor(std::queue<tokenizer::Token>& tokens)
    {
        if (!tokens.empty())
        {
            auto curr_token = tokens.front();

            bool is_num = curr_token.type == tokenizer::Token::Type::Number;
            bool is_var = curr_token.type == tokenizer::Token::Type::Variable;
            bool is_par = curr_token.type == tokenizer::Token::Type::LeftPar;
            bool is_fun = curr_token.type == tokenizer::Token::Type::Function;
            bool is_pipe = curr_token.type == tokenizer::Token::Type::Pipe;
            bool is_err = curr_token.type == tokenizer::Token::Type::Error;

            if (is_num)
            {
                tokens.pop();

                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::Type::Number;
                node->data.value = curr_token.value;

                return node;
            }
            else if (is_var)
            {
                tokens.pop();

                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::Type::Variable;
                node->data.variable = curr_token.symbol;

                return node;
            }
            else if (is_par)
            {
                tokens.pop();
                auto node = read_expr(tokens);
                tokens.pop();

                return node;
            }
            else if (is_fun)
            {
                tokens.pop(); // name ?

                auto node = std::make_unique<ExprAST>();

                node->type = ExprAST::Type::Function;
                node->data.ptr.fun = curr_token.funtype;
                node->data.ptr.left = std::move(read_factor(tokens));

                return node;
            }
            else if (is_pipe)
            {
                tokens.pop();

                auto node = std::make_unique<ExprAST>();

                node->type = ExprAST::Type::UnaryOperator;
                node->data.ptr.op = types::Operators::Abs;
                node->data.ptr.left = std::move(read_factor(tokens));

                tokens.pop();

                return node;
            }
        }

        auto node = std::make_unique<ExprAST>();
        node->type = ExprAST::Type::Nothing;

        return node;
    }


    // exp: factor ^ factor ;
    std::unique_ptr<ExprAST> read_exp(std::queue<tokenizer::Token>& tokens)
    {
        auto node = read_factor(tokens);

        if (!tokens.empty())
        {
            auto curr_token = tokens.front();
            bool is_op = (curr_token.type == tokenizer::Token::Type::Operator);
            bool is_exp = (is_op && curr_token.op == types::Operators::Exp);

            if (is_exp)
            {
                tokens.pop();

                auto new_node = std::make_unique<ExprAST>();

                new_node->type = ExprAST::Type::Operator;
                new_node->data.ptr.left = std::move(node);
                new_node->data.ptr.right = std::move(read_factor(tokens));
                new_node->data.ptr.op = curr_token.op;

                return new_node;
            }
        }

        return node;
    }

    // term: factor
    // |     factor * factor
    // |     factor / factor
    // |     factor % factor
    // % priority = exp
    // ;
    // 
    std::unique_ptr<ExprAST> read_term(std::queue<tokenizer::Token>& tokens)
    {
        auto node = read_exp(tokens);

        if (!tokens.empty())
        {
            auto curr_token = tokens.front();

            bool is_op = curr_token.type == tokenizer::Token::Type::Operator;
            bool is_mul = is_op && curr_token.op == types::Operators::Mul;
            bool is_div = is_op && curr_token.op == types::Operators::Div;
            bool is_mod = is_op && curr_token.op == types::Operators::Mod;

            if (is_mul || is_div || is_mod)
            {
                tokens.pop();

                auto new_node = std::make_unique<ExprAST>();
                new_node->type = ExprAST::Type::Operator;
                new_node->data.ptr.left = std::move(node);
                new_node->data.ptr.right = std::move(read_exp(tokens));
                new_node->data.ptr.op = curr_token.op;

                return new_node;
            }
        }

        return node;
    }

    // expr: term
    // |     term + term
    // |     term - term
    // ;
    std::unique_ptr<ExprAST> read_expr(std::queue<tokenizer::Token>& tokens)
    {
        auto node = read_term(tokens);

        if (!tokens.empty())
        {
            auto curr_token = tokens.front();

            bool is_op = curr_token.type == tokenizer::Token::Type::Operator;
            bool is_add = is_op && curr_token.op == types::Operators::Add;
            bool is_sub = is_op && curr_token.op == types::Operators::Sub;

            if (is_add || is_sub)
            {

                tokens.pop();

                if (node->type == ExprAST::Type::Operator ||
                    node->type == ExprAST::Type::UnaryOperator ||
                    node->type == ExprAST::Type::Nothing)
                {
                    auto new_node = std::make_unique<ExprAST>();

                    new_node->type = ExprAST::Type::UnaryOperator;
                    new_node->data.ptr.left = std::move(read_term(tokens));
                    new_node->data.ptr.op = curr_token.op;

                    return new_node;
                }

                auto new_node = std::make_unique<ExprAST>();
                new_node->type = ExprAST::Type::Operator;
                new_node->data.ptr.left = std::move(node);
                new_node->data.ptr.right = std::move(read_term(tokens));
                new_node->data.ptr.op = curr_token.op;

                return new_node;
            }

        }

        return node;
    }


    template <typename Container>
    std::unique_ptr<ExprAST> create_ast(Container& tokens)
    {
        return read_expr(tokens);
    }

    constexpr auto visit(ExprAST& node);
    std::function<double(double)> visit_op(ExprAST& node);
    std::function<double(double)> visit_uop(ExprAST& node);
    std::function<double(double)> visit_fun(ExprAST& node);

    constexpr auto visit_num(ExprAST& node)
    {
        return [&] (double x) { return node.data.value; };
    }

    std::function<double(double)> visit_var(ExprAST& node)
    {
        switch (node.data.variable)
        {
        case 'e':
            return [] (double x) { return 2.71828; };

        case 'p':
            return [] (double x) { return 3.14; };

        default:
            return [] (double x) { return x; };
        }
    }

    constexpr auto visit(ExprAST& node)
    {
        return [&] (double x) -> double {
            switch (node.type)
            {
            case ExprAST::Type::Nothing:
                return 0;

            case ExprAST::Type::Number:
                return visit_num(node)(x);

            case ExprAST::Type::Operator:
                return visit_op(node)(x);

            case ExprAST::Type::Variable:
                return visit_var(node)(x);

            case ExprAST::Type::UnaryOperator:
                return visit_uop(node)(x);

            case ExprAST::Type::Function:
                return visit_fun(node)(x);
            }
        };
    }

    std::function<double(double)> visit_fun(ExprAST& node)
    {
        auto& left = *node.data.ptr.left.get();

        switch (node.data.ptr.fun)
        {
        case types::Functions::Sin:
            return [&] (double x) -> double {
                return std::sin(visit(left)(x));
            };

        case types::Functions::Cos:
            return [&] (double x) -> double {
                return std::cos(visit(left)(x));
            };

        case types::Functions::Tan:
            return [&] (double x) -> double {
                return std::tan(visit(left)(x));
            };

        case types::Functions::Asin:
            return [&] (double x) -> double {
                return std::asin(visit(left)(x));
            };

        case types::Functions::Acos:
            return [&] (double x) -> double {
                return std::acos(visit(left)(x));
            };

        case types::Functions::Atan:
            return [&] (double x) -> double {
                return std::atan(visit(left)(x));
            };

        case types::Functions::Ln:
            return [&] (double x) -> double {
                return std::log(visit(left)(x));
            };

        case types::Functions::Log:
            return [&] (double x) -> double {
                return std::log10(visit(left)(x));
            };

        case types::Functions::Sqrt:
            return [&] (double x) -> double {
                return std::sqrt(visit(left)(x));
            };

        case types::Functions::Cbrt:
            return [&] (double x) -> double {
                return std::cbrt(visit(left)(x));
            };
        }
    }


    std::function<double(double)> visit_op(ExprAST& node)
    {
        auto& left = *node.data.ptr.left.get();
        auto& right = *node.data.ptr.right.get();

        switch (node.data.ptr.op)
        {
        case types::Operators::Add:
            return [&] (double x) -> double {
                return visit(left)(x) + visit(right)(x);
            };

        case types::Operators::Sub:
            return [&] (double x) -> double {
                return visit(left)(x) - visit(right)(x);
            };

        case types::Operators::Mul:
            return [&] (double x) -> double {
                return visit(left)(x) * visit(right)(x);
            };

        case types::Operators::Div:
            return [&] (double x) -> double {
                return visit(left)(x) / visit(right)(x);
            };

        case types::Operators::Exp:
            return [&] (double x) -> double {
                return std::pow(visit(left)(x), visit(right)(x));
            };

        case types::Operators::Mod:
            return [&] (double x) -> double {
                return std::fmod(visit(left)(x), visit(right)(x));
            };
        }
    }

    std::function<double(double)> visit_uop(ExprAST& node)
    {
        auto& left = *node.data.ptr.left.get();

        switch (node.data.ptr.op)
        {
        case types::Operators::Add:
            return [&] (double x) -> double {
                return + visit(left)(x);
            };

        case types::Operators::Sub:
            return [&] (double x) -> double {
                return - visit(left)(x);
            };

        case types::Operators::Abs:
            return [&] (double x) -> double {
                return std::abs(visit(left)(x));
            };
        }
    }
}
