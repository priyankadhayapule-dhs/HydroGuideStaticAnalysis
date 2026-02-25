R"(#version 300 es

precision mediump float;
layout (location = 4) in vec2 vertex; // <vec2 pos>

void main()
{
    gl_Position = vec4(vertex.x, vertex.y, 0.0, 1.0);

}
)"
