R"(#version 300 es
precision mediump float;
in vec2 TexCoords;
out vec4 color;

uniform sampler2D u_MTexture;

void main()
{
    color = texture(u_MTexture, TexCoords);
}
)"
