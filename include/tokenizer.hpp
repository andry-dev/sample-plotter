#pragma once

#include <string_view>
#include <string>
#include <cmath>
#include <cstring>
#include <iosfwd>

#include "common_types.h"

namespace tokenizer
{
    struct Token
    {
        enum class Type
        {
            Number = 0,
            Variable,

            Operator,
            UnaryOperator,

            Function,

            LeftPar,
            RightPar,
            Pipe,

            EOL,
            Error
        } type {Type::Error};

        types::Operators op{types::Operators::None};
        types::Functions funtype{types::Functions::None};

        char symbol = '\0';
        double value = 0;
    };

    std::ostream& operator<<(std::ostream& out, Token::Type& type)
    {
        out << (int)type;
        return out;
    }


    namespace parsers
    {
        Token parse_number(const std::string& str, int& index)
        {
            constexpr auto is_digit = [] (char c) -> bool {
                switch (c)
                {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '.':
                    return true;

                default:
                    return false;
                }
            };

            int first_index = index;
            while (is_digit(str[index]))
            {
                ++index;
            }

            if (index != first_index)
            {
                char buff[64] = {0};
                std::memcpy(buff, &str[first_index], index - first_index);
                Token t;
                t.type = Token::Type::Number;
                t.value = std::atof(buff);
                return t;
            }
            else
            {
                Token t;
                return t;
            }
        }

        Token parse_function(const std::string& str, int& index)
        {
            using namespace std::literals;
            Token tok;
            tok.type = Token::Type::Error;

            constexpr auto comp_fun = [] (std::string_view str,
                                          std::string_view cmp) -> bool {
                return str.compare(0, cmp.size(), cmp) == 0;
            };

            using FunctionArray = std::array<
                std::pair<types::Functions, std::string_view>,
                10>;

            constexpr FunctionArray functions {{
                { types::Functions::Sin, "sin" },
                { types::Functions::Cos, "cos" },
                { types::Functions::Tan, "tan" },
                { types::Functions::Asin, "asin" },
                { types::Functions::Acos, "acos" },
                { types::Functions::Atan, "atan" },
                { types::Functions::Log, "log" },
                { types::Functions::Ln,  "ln"  },
                { types::Functions::Sqrt, "sqrt" },
                { types::Functions::Cbrt, "cbrt" },
            }};

            for (const auto& f : functions)
            {
                if (comp_fun(&str[index], f.second))
                {
                    tok.type = Token::Type::Function;
                    tok.funtype = f.first;

                    index += f.second.size();

                    return tok;
                }
            }

            return tok;
        }
    }

    void skip_whitespace(const std::string& str, int& index)
    {
        while (str[index] == ' ')
        {
            ++index;
        }
    }

    Token parse_token(const std::string& str, int& index)
    {
        skip_whitespace(str, index);
        auto tok = parsers::parse_number(str, index);

        if (tok.type != Token::Type::Error)
        {
            return tok;
        }

        tok = parsers::parse_function(str, index);
        if (tok.type != Token::Type::Error)
        {
            return tok;
        }

        switch (str[index])
        {
        case 'x':
        case 'y':
        case 'z':
        case 'e':
        case 'p':
            tok.symbol = str[index];
            tok.type = Token::Type::Variable;
            break;

        case '+':
            tok.symbol = '+';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Add;
            break;

        case '-':
            tok.symbol = '-';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Sub;
            break;

        case '*':
            tok.symbol = '*';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Mul;
            break;

        case '/':
            tok.symbol = '/';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Div;
            break;

        case '%':
            tok.symbol = '%';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Mod;
            break;

        case '^':
            tok.symbol = '^';
            tok.type = Token::Type::Operator;
            tok.op = types::Operators::Exp;
            break;

        case '(':
            tok.symbol = '(';
            tok.type = Token::Type::LeftPar;
            break;

        case ')':
            tok.symbol = ')';
            tok.type = Token::Type::RightPar;
            break;

        case '|':
            tok.symbol = '|';
            tok.type = Token::Type::Pipe;
            break;

        case 0:
            tok.symbol = '\0';
            tok.type = Token::Type::EOL;
            break;

        default:
            tok.type = Token::Type::Error;
        }

        ++index;

        return tok;
    }

}
