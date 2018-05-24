#pragma once

namespace types
{
    enum class Operators
    {
        None,
        Add, Sub,
        Mul, Div,
        Exp,
        Mod,
        Abs
    };

    enum class Functions
    {
        None,
        Sin, Cos, Tan,
        Asin, Acos, Atan,
        Log, Ln,
        Sqrt, Cbrt
    };

    std::ostream& operator<<(std::ostream& out, types::Operators op)
    {
        out << (int)op;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, types::Functions fun)
    {
        out << (int)fun;
        return out;
    }

}
