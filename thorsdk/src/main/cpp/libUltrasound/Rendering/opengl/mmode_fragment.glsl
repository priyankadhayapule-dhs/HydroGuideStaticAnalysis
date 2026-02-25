R"(#version 300 es
precision mediump float;
in vec2 TexCoords;
out vec4 color;

uniform sampler2D u_MTexture;

uniform vec3 u_tintAdj;         // Tint adjustment.

void main()
{
    vec4 adjCoeff = vec4(u_tintAdj, 1.0);
    color = texture(u_MTexture, TexCoords) * adjCoeff;

    //color = vec4(0.0, 1.0, 0.0, 1.0);

    //f (TexCoords.y > 0.5)
    //    color = vec4(0.0, 1.0, 0.0, 1.0);
}
)"