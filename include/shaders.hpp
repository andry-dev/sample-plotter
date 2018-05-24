#pragma once

#include <array>
#include "asl/string_view"


constexpr const char* g_vertsrc = R"(
    #version 410 core
    #extension GL_ARB_explicit_uniform_location : enable

    layout (location = 0) in vec2 Pos;
    layout (location = 1) in vec4 Color;

    uniform mat4 MVP;
    uniform float pointSize;
    uniform float scale;

    out vec4 fragColor;

    void main()
    {
        gl_Position = MVP * vec4(Pos.x * scale, Pos.y * scale, 0.0, 1.0);
        fragColor = Color;
        gl_PointSize = pointSize;
    }
)";

constexpr std::array<asl::string_view, 2> g_vertlocations {
    "Pos",
    "Color",
};

constexpr const char* g_fragsrc = R"(
    #version 410 core

    in vec4 fragColor;
    out vec4 color;

    void main()
    {
        color = fragColor;
    }
)";

