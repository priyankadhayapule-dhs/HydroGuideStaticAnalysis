R"(#version 300 es
precision mediump float;
out vec4 color;

uniform vec3 u_drawColor;

void main()
{
    color = vec4(u_drawColor, 1.0);
}
)"
