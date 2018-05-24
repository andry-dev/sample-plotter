#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <string_view>
#include <memory>
#include <deque>
#include <queue>

#include "tokenizer.hpp"
#include "parser.hpp"

#include "tewi/Video/Window.hpp"
#include "plot_renderer.hpp"
#include "shaders.hpp"

#include "tewi/Video/Shader.hpp"
#include "tewi/IO/InputManager.h"
#include "tewi/Video/Uniform.h"
#include "tewi/Utils/TickTimer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "gsl/assert"

template <typename F>
constexpr double function_area(const double divisions,
                      double lower_limit,
                      double upper_limit,
                      F&& fun)
{
    if (upper_limit < lower_limit)
    {
        std::swap(lower_limit, upper_limit);
    }

    const double deltax = (upper_limit - lower_limit) / divisions;

    double res = 0.0;
    double curr = lower_limit;

    for (int i = 0; i < divisions; ++i)
    {
        curr += deltax;
        res += fun(curr);
    }

    return res * deltax;
}

template <typename Fun>
void start_plot(Fun&& fun)
{
    using def_tag = tewi::API::OpenGLTag;

    tewi::InputManager inputManager;

    tewi::Window<def_tag> win("Plot", tewi::Width{1280}, tewi::Height{720}, &inputManager);

    tewi::TickTimer timer;

    tewi::setWindowKeyboardCallback(win,
                                    [] (GLFWwindow* win, int key,
                                        int scancode, int action, int mods)
    {
        auto& inputMan = *(static_cast<tewi::InputManager*>(glfwGetWindowUserPointer(win)));

        if (action == GLFW_PRESS)
        {
            inputMan.pressKey(key);
        }
        else if (action == GLFW_RELEASE)
        {
            inputMan.releaseKey(key);
        }
    });

    glfwSetInputMode(win.windowPtr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // :(
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    constexpr auto max_graph_points = 9000;
    constexpr auto max_grid_lines = 1500;
    constexpr auto max_grid_size = max_grid_lines * 2;
    constexpr auto max_base_graph_size = 4 + max_grid_size;
    constexpr asl::f64 max_point_interval = 0.1;

    std::array<GraphPoint, max_graph_points> graph;
    std::array<GraphPoint, max_base_graph_size> axis;

    {
        asl::mut_f64 init = (0 - (max_graph_points * max_point_interval)) / 2;

        for (asl::mut_num i = 0; i < max_graph_points; ++i)
        {
            graph[i].pos.x = init;
            graph[i].pos.y = fun(init);

            graph[i].color.r = 0;
            graph[i].color.g = 0;
            graph[i].color.b = 255;
            graph[i].color.a = 255;

            init += max_point_interval;
        }
    }

    {
        axis[0].pos.x = -10000.0f;
        axis[0].pos.y = 0;
        axis[1].pos.x = 10000.0f;
        axis[1].pos.y = 0;

        axis[2].pos.x = 0;
        axis[2].pos.y = -10000.0f;
        axis[3].pos.x = 0;
        axis[3].pos.y = 10000.0f;
    }

    for (asl::mut_num i = 0; i < 4; ++i)
    {
        axis[i].color.r = 0;
        axis[i].color.g = 0;
        axis[i].color.b = 0;
        axis[i].color.a = 255;

        axis[i].color.r = 0;
        axis[i].color.g = 0;
        axis[i].color.b = 0;
        axis[i].color.a = 255;
    }

    {
        asl::mut_f64 start = (0 - (max_grid_lines / 4));
        for (asl::mut_num i = 4; i < max_grid_lines; i += 2)
        {
            axis[i].pos.x = -10000.0f;
            axis[i].pos.y = start;
            axis[i + 1].pos.x = 10000.0f;
            axis[i + 1].pos.y = start;
            start += 1;
        }
    }

    {
        asl::mut_f64 start = (0 - (max_grid_lines / 4));
        for (asl::mut_num i = max_grid_lines; i < max_base_graph_size; i += 2)
        {
            axis[i].pos.x = start;
            axis[i].pos.y = -10000.0f;
            axis[i + 1].pos.x = start;
            axis[i + 1].pos.y = 10000.0f;

            start++;
        }
    }

    for (asl::mut_num i = 4; i < max_base_graph_size; ++i)
    {
        axis[i].color.r = 0;
        axis[i].color.g = 0;
        axis[i].color.b = 0;
        axis[i].color.a = 20;
    }

    PlotRenderer2D<def_tag, max_graph_points> rend{graph};
    PlotRenderer2D<def_tag, max_base_graph_size> axis_rend{axis};


    using VertexShader = tewi::Shader<def_tag, tewi::VertexShader, tewi::ShaderFromMemoryPolicy>;
    using FragmentShader = tewi::Shader<def_tag, tewi::FragmentShader, tewi::ShaderFromMemoryPolicy>;

    VertexShader vert(tewi::API::Device<def_tag>{}, g_vertsrc);
    FragmentShader frag(tewi::API::Device<def_tag>{}, g_fragsrc);

    tewi::ShaderProgram<def_tag> shader(g_vertlocations, vert, frag);

    auto proj = glm::ortho(-1280.0f / 2, 1280.0f / 2, -720.0f / 2, 720.0f / 2);

    glm::mat4 view(1);
    glm::mat4 MVP = proj;

    asl::mut_num rend_type = GL_LINE_STRIP;
    asl::mut_f32 point_size = 1.0f;
    asl::mut_f32 line_thickness = 1.0f;
    asl::mut_f32 graph_scale = 10.0f;

    while (!tewi::isWindowClosed(win))
    {
        tewi::pollWindowEvents(win);

        {
            const auto deltatime = timer.getDeltaTime();
            const auto camera_speed = 200 * deltatime;
            const auto scale_speed = 10.0f * deltatime;

            constexpr auto max_margin = 1000.0f;

            if (inputManager.isKeyDown(GLFW_KEY_ESCAPE))
            {
                tewi::forceCloseWindow(win);
            }

            if (inputManager.isKeyDown(GLFW_KEY_W))
            {
                if (glm::vec3 pos{view[3]}; pos.y >= -max_margin)
                {
                    view = glm::translate(view, glm::vec3(0.0f, -camera_speed, 0.0f));
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_S))
            {
                if (glm::vec3 pos{view[3]}; pos.y <= max_margin)
                {
                    view = glm::translate(view, glm::vec3(0.0f, camera_speed, 0.0f));
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_A))
            {
                if (glm::vec3 pos{view[3]}; pos.x <= max_margin)
                {
                    view = glm::translate(view, glm::vec3(camera_speed, 0.0f, 0.0f));
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_D))
            {
                if (glm::vec3 pos{view[3]}; pos.x >= -max_margin)
                {
                    view = glm::translate(view, glm::vec3(-camera_speed, 0.0f, 0.0f));
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_Q))
            {
                if (graph_scale >= 5.0f)
                {
                    graph_scale -= scale_speed;
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_E))
            {
                if (graph_scale <= 100.0f)
                {
                    graph_scale += scale_speed;
                }
            }

            if (inputManager.isKeyDown(GLFW_KEY_F1))
            {
                rend_type = GL_LINE_STRIP;
            }
            else if (inputManager.isKeyDown(GLFW_KEY_F2))
            {
                rend_type = GL_POINTS;
            }

            if (inputManager.isKeyDown(GLFW_KEY_0))
            {
                point_size += 1.0f * deltatime;

                if (point_size > 10.0f)
                {
                    point_size = 10.0f;
                }
            }
            else if (inputManager.isKeyDown(GLFW_KEY_9))
            {
                point_size -= 1.0f * deltatime;

                if (point_size < 0.0f)
                {
                    point_size = 0.0f;
                }
            }
        }

        MVP = proj * view;

        win.context.preDraw();

        shader.enable();

        tewi::setUniform(shader.getUniformLocation("MVP"), MVP);
        tewi::setUniform(shader.getUniformLocation("pointSize"), point_size);
        tewi::setUniform(shader.getUniformLocation("scale"), graph_scale);

        axis_rend.begin();
        axis_rend.end();
        axis_rend.draw(GL_LINES);

        rend.begin();
        rend.end();
        rend.draw(rend_type);

        shader.disable();

        win.context.postDraw();

        tewi::swapWindowBuffers(win);
    }
}


int main()
{
    std::cout << "y = ";
    std::string str;
    std::getline(std::cin, str);

    int start = 0;
    std::queue<tokenizer::Token> tokens;
    while (str[start] != '\0')
    {
        tokenizer::Token t = tokenizer::parse_token(str, start);
        if ((t.type != tokenizer::Token::Type::Error) ||
            (t.type != tokenizer::Token::Type::EOL))
        {
            tokens.push(t);
        }
    }

    auto root = parser::create_ast(tokens);

    parser::ExprAST* currnode = root.get();

    auto fun = parser::visit(*currnode);

    double divisions = 0.0;
    double a = 0.0;
    double b = 0.0;

    std::cout << "For f(x) = " << str << ":\n";
    std::cout << "Lower limit: ";
    std::cin >> a;
    std::cout << "Upper limit: ";
    std::cin >> b;
    std::cout << "Number of divisions: ";
    std::cin >> divisions;

    std::cout << '\n';
    std::cout << "Area calcolata col metodo dei rettangoli: "
              << function_area(divisions, a, b, fun)  << '\n';

    std::cout << "Do you want to see the function plot? [Y/N]: ";
    char res = '\0';

    std::cin >> res;
    if (res == 'y' || res == 'Y')
    {
        start_plot(fun);
    }
}
