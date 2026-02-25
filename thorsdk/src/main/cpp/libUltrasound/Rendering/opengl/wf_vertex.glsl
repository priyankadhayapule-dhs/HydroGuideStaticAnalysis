R"(#version 300 es

precision mediump float;
layout (location = 3) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat3 u_projection;

void main()
{
    TexCoords = vertex.zw;
    vec3 nvPos = vec3(vertex.xy, 1.0);
    gl_Position = vec4(nvPos.x, nvPos.y, 0.0, 1.0);

}
)"
