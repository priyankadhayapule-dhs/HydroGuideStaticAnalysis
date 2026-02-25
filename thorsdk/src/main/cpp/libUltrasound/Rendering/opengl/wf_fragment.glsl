R"(#version 300 es
precision mediump float;
in vec2 TexCoords;
out vec4 color;

uniform sampler2D u_MTexture;
uniform vec3 u_TextColor;

void main()
{
    vec4 outPix = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 texOut = texture(u_MTexture, TexCoords);

    if (texOut.x > 0.1)
        outPix = vec4(u_TextColor * texOut.x, 1.0);

    color = outPix;
}
)"
