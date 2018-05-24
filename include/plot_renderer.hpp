#pragma once

#include "tewi/Video/API/API.h"
#include "tewi/Video/Renderer2D.hpp"
#include "tewi/Video/BatchRenderer2D.hpp"
#include "tewi/Video/Shader.hpp"

#include "asl/types"
#include "glm/glm.hpp"

struct GraphPoint {
    glm::vec2 pos;

    struct Color
    {
        asl::mut_u8 r;
        asl::mut_u8 g;
        asl::mut_u8 b;
        asl::mut_u8 a;
    } color;
};

template <typename APITag, asl::i32 NumElem = 2000>
class PlotRenderer2D { };

template <asl::i32 NumElem>
class PlotRenderer2D<tewi::API::OpenGLTag, NumElem>
{
public:
    PlotRenderer2D(const std::array<GraphPoint, NumElem>& data)
    {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GraphPoint), (const void*)0);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GraphPoint), (const void*)offsetof(GraphPoint, color));

        glBufferData(GL_ARRAY_BUFFER, sizeof(GraphPoint) * NumElem, &data[0], GL_STATIC_DRAW);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void begin()
    {
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        m_lastPoint = reinterpret_cast<GraphPoint*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    }

    void add(GraphPoint* begin, asl::i32 size)
    {
        for (asl::mut_i32 i = 0; i < size; ++i)
        {
            m_lastPoint->pos = begin[i].pos;
            m_lastPoint->color = begin[i].color;
            tewi::Log::info(std::to_string(m_lastPoint->pos.y));
            ++m_lastPoint;
        }
    }

    void end()
    {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw(asl::mut_num rend_type, asl::mut_num start = 0, asl::mut_num end = NumElem)
    {
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);


        glDrawArrays(rend_type, start, end);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

private:
    GraphPoint* m_lastPoint;
    GLuint m_VAO;
    GLuint m_VBO;
};
