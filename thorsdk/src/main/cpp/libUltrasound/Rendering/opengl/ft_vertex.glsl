R"(#version 300 es

precision mediump float;
layout (location = 5) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

void main()
{
    TexCoords = vertex.zw;
    gl_Position = vec4(vertex.x, vertex.y, 0.0, 1.0);

}
)"
