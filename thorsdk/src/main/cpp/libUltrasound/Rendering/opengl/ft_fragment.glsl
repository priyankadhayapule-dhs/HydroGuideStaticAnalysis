R"(#version 300 es
precision mediump float;
in vec2 TexCoords;
out vec4 color;

uniform sampler2D u_text;
uniform vec3 u_textColor;

void main()
{
    float sampled = texture(u_text, TexCoords).r;

    //float sampled = texture(u_text, vec2(0.0, 0.0)).r;

    color = vec4(u_textColor, 1.0) * sampled;

    //color = vec4(1.0, 1.0, 0.0, 1.0);
}
)"
