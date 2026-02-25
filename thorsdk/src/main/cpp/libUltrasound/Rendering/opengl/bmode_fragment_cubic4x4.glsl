R"(#version 300 es

precision highp float;       	// Set the default precision to medium. We don't need as high of a
								// precision in the fragment shader.

uniform sampler2D u_Texture;    // The input texture.
                                // Need to set texture unit to Linear for this shader!

uniform float u_Width;          // The input texture.
uniform float u_Height;         // The input texture.

in vec2 v_TexCoordinate;        // Interpolated texture coordinate per fragment.

uniform vec3 u_tintAdj;         // Tint adjustment.

layout(location = 0) out vec4 glFragColor;


vec4 cubic(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =   -x3   + 3.0*x2 - 3.0*x + 1.0;
    w.y =  3.0*x3 - 6.0*x2         + 4.0;
    w.z = -3.0*x3 + 3.0*x2 + 3.0*x + 1.0;
    w.w =  x3;
    return w / 6.0;
}

// Catmull-Rom spline actually passes through control points
vec4 cubic_catmullrom(float x)
{
    const float s = 0.5; // potentially adjustable parameter
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =      -s*x3 +       2.0*s*x2 - s*x + 0.0;
    w.y = (2.0-s)*x3 +     (s-3.0)*x2       + 1.0;
    w.z = (s-2.0)*x3 + (3.0-2.0*s)*x2 + s*x + 0.0;
    w.w =       s*x3 -           s*x2       + 0.0;
    return w;
}


vec4 cubic4x4(sampler2D textureS, vec2 texCoord_i)
{
    float fWidth = u_Width;
    float fHeight = u_Height;

    float texelSizeX = 1.0 / fWidth; //size of one texel
    float texelSizeY = 1.0 / fHeight; //size of one texel

    float nX = texCoord_i.x * fWidth - 0.5;
    float nY = texCoord_i.y * fHeight - 0.5;

    float Xfract = fract(nX);
    float Yfract = fract(nY);

    float Xint = nX - Xfract;
    float Yint = nY - Yfract;

    //vec2 texCoord_New = vec2( ( Xint + 0.5 ), ( Yint + 0.5 ) );
    vec2 texCoord_New = vec2(Xint, Yint);

    vec4 xcubic = cubic(Xfract);
    vec4 ycubic = cubic(Yfract);

    vec4 c = vec4(texCoord_New.x - 0.5, texCoord_New.x + 1.5, texCoord_New.y - 0.5, texCoord_New.y + 1.5);  //can go with .5 1.5
    vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    vec4 texelSize = vec4 (texelSizeX, texelSizeX, texelSizeY, texelSizeY);
    vec4 offsetXY = offset * texelSize;

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    vec4 sample0 = texture(textureS, vec2(offsetXY.x, offsetXY.z));
    vec4 sample1 = texture(textureS, vec2(offsetXY.y, offsetXY.z));
    vec4 sample2 = texture(textureS, vec2(offsetXY.x, offsetXY.w));
    vec4 sample3 = texture(textureS, vec2(offsetXY.y, offsetXY.w));

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

void main() {

    vec4 preOut = cubic4x4(u_Texture, v_TexCoordinate);
    vec4 adjCoeff = vec4(u_tintAdj, 1.0);
    glFragColor = adjCoeff * preOut;

}
)"
