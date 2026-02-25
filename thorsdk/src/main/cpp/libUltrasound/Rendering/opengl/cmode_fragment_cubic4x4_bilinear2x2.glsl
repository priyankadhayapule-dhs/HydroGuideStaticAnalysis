R"(#version 300 es

precision highp float;       	// Set the default precision to medium. We don't need as high of a
								// precision in the fragment shader.

uniform sampler2D u_Texture;            // The input texture.   for B-mode
                                        // Need to set texture unit to Linear for this shader!
uniform sampler2D u_CTexture;           // The input texture.  for C-mode
uniform sampler2D u_CMapTexture;        // color LUT

uniform float u_Width;          // The input texture.
uniform float u_Height;         // The input texture.
uniform float u_CWidth;         // The input texture.
uniform float u_CHeight;        // The input texture.

uniform float u_BThreshold;     // Threshold values
uniform float u_CThreshold;

uniform int   u_DopplerMode;    // CVD = 0 (default), CPD = 1

//in vec3 v_Position;		    // Interpolated position for this fragment.
//in vec3 v_Normal;         	// Interpolated normal for this fragment.
in vec2 v_TexCoordinate;        // Interpolated texture coordinate per fragment.
in vec2 v_CTexCoordinate;       // Interpolated texture coordinate per fragment.

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
vec4 cubic_catmullrom(float x) // cubic_catmullrom(float x)
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

// assume  0 <= x <= 255
float convert2s8(float x)
{
    float outVal = x;

    if (x >= 128.0)
        outVal = 256.0 - x;

    return outVal;
}

// 0 ~ 1 to -0.5 to 0.5
float convert2sf(float x)
{
    float outV = x;

    if (x >= 128.0/255.0)
        outV = -256.0/255.0 + x;

    return outV;
}

// -0.5 ~ 0.5 to 0 ~ 1
float convert2uf(float x)
{
    float outV = x;

    if (x < 0.0)
        outV = 256.0/255.0 + x;

    return outV;
}


// floating angular interpolation
// this function assumes input is in signed floating
// meaning: -0.5 ~ 0.5 range.
float angmixf(float in1, float in2, float a)
{
    float diff = abs(in1 - in2);
    if (diff > 128.0/255.0) {
        if (in2 > in1)
            in1 = in1 + 256.0/255.0;
        else
            in2 = in2 + 256.0/255.0;
    }

    float outV = in1 + (in2 - in1) * a;

    if (outV >= 128.0/255.0)
        outV = outV - 256.0/255.0;

    return outV;
}


vec4 cubic4x4(sampler2D textureS, vec2 texCoord_i, float fiWidth, float fiHeight)
{
    //float fWidth = u_Width;
    //float fHeight = u_Height;
    float fWidth = fiWidth;
    float fHeight = fiHeight;

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

    vec4 outpixel = mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);

    //if (texCoord_i.x > 0.9 )
    //if (nY < 0.0)
    //    outpixel = vec4(1.0, 1.0, 0.0, 1.0);

    return outpixel;
}


// Function to get a texel data from a texture with GL_NEAREST property.
// Bi-Linear interpolation is implemented in this function with the
// help of nearest four data. Color Power Doppler assumes uint8 data
vec4 tex2DBiLinearColorCPD( sampler2D textureSampler_i, sampler2D texture_cmap, vec2 texCoord_i, float ifWidth, float ifHeight)
{
    float fWidth = ifWidth;
    float fHeight = ifHeight;

    float texelSizeX = 1.0 / fWidth; //size of one texel
    float texelSizeY = 1.0 / fHeight; //size of one texel

    float nX = texCoord_i.x * fWidth - 0.5;
    float nY = texCoord_i.y * fHeight - 0.5;

    float Xfract = fract(nX);
    float Yfract = fract(nY);

    float Xint = nX - Xfract;
    float Yint = nY - Yfract;

    float a = Xfract; // Get Interpolation factor for X direction. // Fraction near to valid data.
    float b = Yfract; // Get Interpolation factor for Y direction.

    vec2 texCoord_New = vec2( ( Xint + 0.5 ) / fWidth,
                              ( Yint + 0.5 ) / fHeight );
    // Take nearest two data in current row.
    vec4 p0q0 = texture(textureSampler_i, texCoord_New);
    vec4 p1q0 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX, 0));

    // Take nearest two data in bottom row.
    vec4 p0q1 = texture(textureSampler_i, texCoord_New + vec2(0, texelSizeY));
    vec4 p1q1 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX , texelSizeY));

    float p0q0f = p0q0.x;
    float p1q0f = p1q0.x;
    float p0q1f = p0q1.x;
    float p1q1f = p1q1.x;

    float pInt_q0 = mix(p0q0f, p1q0f, a);
    float pInt_q1 = mix(p0q1f, p1q1f, a);

    // 0 ~ 1 float
    float pInt_pq = mix(pInt_q0, pInt_q1, b);
    vec4 lutVal;

    if (pInt_pq >= (u_CThreshold / 255.0f))
        lutVal = texture(texture_cmap, vec2(pInt_pq, 0.0));
    else
        lutVal = vec4(0.0, 0.0, 0.0, 0.0);

    return lutVal; // Interpolate in Y direction.
}


// Function to get a texel data from a texture with GL_NEAREST property.
// Bi-Linear interpolation is implemented in this function with the
// help of nearest four data.
vec4 tex2DBiLinearColor( sampler2D textureSampler_i, sampler2D texture_cmap, vec2 texCoord_i, float ifWidth, float ifHeight)
{
    float fWidth = ifWidth;
    float fHeight = ifHeight;

	float texelSizeX = 1.0 / fWidth; //size of one texel
	float texelSizeY = 1.0 / fHeight; //size of one texel

    float nX = texCoord_i.x * fWidth - 0.5;
    float nY = texCoord_i.y * fHeight - 0.5;

    float Xfract = fract(nX);
    float Yfract = fract(nY);

    float Xint = nX - Xfract;
    float Yint = nY - Yfract;

    float a = Xfract; // Get Interpolation factor for X direction. // Fraction near to valid data.
    float b = Yfract; // Get Interpolation factor for Y direction.

	vec2 texCoord_New = vec2( ( Xint + 0.5 ) / fWidth,
							  ( Yint + 0.5 ) / fHeight );
	// Take nearest two data in current row.
    vec4 p0q0 = texture(textureSampler_i, texCoord_New);
    vec4 p1q0 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX, 0));

	// Take nearest two data in bottom row.
    vec4 p0q1 = texture(textureSampler_i, texCoord_New + vec2(0, texelSizeY));
    vec4 p1q1 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX , texelSizeY));

    float p0q0f = convert2sf(p0q0.x);
    float p1q0f = convert2sf(p1q0.x);
    float p0q1f = convert2sf(p0q1.x);
    float p1q1f = convert2sf(p1q1.x);

    float pInt_q0 = mix(p0q0f, p1q0f, a);
    float pInt_q1 = mix(p0q1f, p1q1f, a);

    float pInt_pq = mix(pInt_q0, pInt_q1, b);

    // convert float to int values
    //float pInt_pqI = pInt_pq * 256.0f - 0.5f;
    //float pInt_pqIS8 = convertToS8(pInt_pqI);       // -128 to 127

    //float pInt_qpIS8N =

    float pInt_pq_uf = convert2uf(pInt_pq);

    //pInt_pq_uf = 0.8f;

    vec4 lutVal = texture(texture_cmap, vec2(pInt_pq_uf, 0.0));


	// Interpolation in X direction.
    //vec4 pInterp_q0 = mix( p0q0, p1q0, a ); // Interpolates top row in X direction.
    //vec4 pInterp_q1 = mix( p0q1, p1q1, a ); // Interpolates bottom row in X direction.

    //vec4 outPix = mix( pInterp_q0, pInterp_q1, b );

    //outPix = p0q0;


    vec4 outPix = lutVal;

    // testing
    //if (p0q0.x > 0.498)
    //    outPix = vec4(1.0, 1.0, 0.0, 1.0);

    //if (p0q0.x > 0.6) {
    //    outPix = vec4(1.0, 1.0, 0.0, 1.0);
    //}


    return outPix; // Interpolate in Y direction.
}


// Function to get a texel data from a texture with GL_NEAREST property.
// Bi-Linear interpolation is implemented in this function with the
// help of nearest four data.  Angluar interpolation
vec4 bilinearAngColor( sampler2D textureSampler_i, sampler2D texture_cmap, vec2 texCoord_i, float ifWidth, float ifHeight)
{
    float fWidth = ifWidth;
    float fHeight = ifHeight;

	float texelSizeX = 1.0 / fWidth; //size of one texel
	float texelSizeY = 1.0 / fHeight; //size of one texel

    float nX = texCoord_i.x * fWidth - 0.5;
    float nY = texCoord_i.y * fHeight - 0.5;

    float Xfract = fract(nX);
    float Yfract = fract(nY);

    float Xint = nX - Xfract;
    float Yint = nY - Yfract;

    float a = Xfract; // Get Interpolation factor for X direction. // Fraction near to valid data.
    float b = Yfract; // Get Interpolation factor for Y direction.

	vec2 texCoord_New = vec2( ( Xint + 0.5 ) / fWidth,
							  ( Yint + 0.5 ) / fHeight );
	// Take nearest two data in current row.
    vec4 p0q0 = texture(textureSampler_i, texCoord_New);
    vec4 p1q0 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX, 0));

	// Take nearest two data in bottom row.
    vec4 p0q1 = texture(textureSampler_i, texCoord_New + vec2(0, texelSizeY));
    vec4 p1q1 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX , texelSizeY));

    float p0q0f = convert2sf(p0q0.x);
    float p1q0f = convert2sf(p1q0.x);
    float p0q1f = convert2sf(p0q1.x);
    float p1q1f = convert2sf(p1q1.x);

    float pInt_q0 = angmixf(p0q0f, p1q0f, a);
    float pInt_q1 = angmixf(p0q1f, p1q1f, a);
    float pInt_pq = angmixf(pInt_q0, pInt_q1, b);

    float pInt_pqus = convert2uf(pInt_pq);

    // convert float to int values
    float pInt_pqI = round(pInt_pqus * 255.0f);           // 0 - 255
    float pInt_pqSI = abs(convert2s8(pInt_pqI));          // signed int for thresholding 0 - 128 (abs)

    float pInt_pqIS8f = (pInt_pqI + 0.5) / 256.0;         // for texture lookup

    vec4 outPix;

    if (pInt_pqSI >= u_CThreshold)
        outPix = texture(texture_cmap, vec2(pInt_pqIS8f, 0.0));
    else
        outPix = vec4(0.0, 0.0, 0.0, 0.0);

    return outPix;
}




/*
float Triangular( float f )
{
	f = f / 2.0;
	if( f < 0.0 )
	{
		return ( f + 1.0 );
	}
	else
	{
		return ( 1.0 - f );
	}
	return 0.0;
}

vec4 biCubic( sampler2D textureSampler, vec2 TexCoord )
{
    float fWidth = u_Width;
    float fHeight = u_Height;

	float texelSizeX = 1.0 / fWidth; //size of one texel
	float texelSizeY = 1.0 / fHeight; //size of one texel

    vec4 nSum = vec4( 0.0, 0.0, 0.0, 0.0 );
    vec4 nDenom = vec4( 0.0, 0.0, 0.0, 0.0 );

    float nX = TexCoord.x * fWidth - 0.5;
    float nY = TexCoord.y * fHeight - 0.5;

    float Xfract = fract(nX);
    float Yfract = fract(nY);

    float Xint = nX - Xfract;
    float Yint = nY - Yfract;

    float a = Xfract; // Get Interpolation factor for X direction. // Fraction near to valid data.
    float b = Yfract; // Get Interpolation factor for Y direction.

    vec2 texCoord_New = vec2( ( Xint + 0.5 ) / fWidth,
                              ( Yint + 0.5 ) / fHeight );

    for( int m = -1; m <=2; m++ )
    {
        for( int n =-1; n<= 2; n++)
        {
			vec4 vecData = texture(textureSampler, texCoord_New + vec2(texelSizeX * float( m ), texelSizeY * float( n )));
			float f  = Triangular( float( m ) - a );
			vec4 vecCooef1 = vec4( f,f,f,f );
			float f1 = Triangular( -( float( n ) - b ) );
			vec4 vecCoeef2 = vec4( f1, f1, f1, f1 );
            nSum = nSum + ( vecData * vecCoeef2 * vecCooef1  );
            nDenom = nDenom + (( vecCoeef2 * vecCooef1 ));
        }
    }
    return nSum / nDenom;
}*/

vec4 colorRender(sampler2D textureB, sampler2D textureC, sampler2D textureCMap, vec2 texCoord_b, vec2 texCoord_c) {

    vec4 outPix, outPixC;
    // B - lookup
    outPix = cubic4x4(textureB, texCoord_b, u_Width, u_Height);

    bool BTh = false;

    if (round(outPix.x * 255.0) >= u_BThreshold )
        BTh = true;

    // adjust B-mode Tint
    vec4 adjCoeff = vec4(u_tintAdj, 1.0);
    outPix = outPix * adjCoeff;

    // color is in the box range
    if (texCoord_c.x >= 0.0 && texCoord_c.x <= 1.0 && texCoord_c.y >= 0.0 && texCoord_c.y <= 1.0 && !BTh)
    {
        // color processing
        if (u_DopplerMode == 1) {
            outPixC = tex2DBiLinearColorCPD(textureC, textureCMap, texCoord_c, u_CWidth, u_CHeight);
        }
        else {
            outPixC = bilinearAngColor(textureC, textureCMap, texCoord_c, u_CWidth, u_CHeight);
        }

        // if not over the threshold alpha channle value is 0
        if (outPixC.w > 0.0 )
            outPix = outPixC;
    }

    //outPix= vec4(0.0, 0.0, 0.0, 1.0);
    return outPix;
}

void main() {
    // org B
    //gl_FragColor = cubic4x4(u_Texture, v_TexCoordinate);


    glFragColor = colorRender(u_Texture, u_CTexture, u_CMapTexture, v_TexCoordinate, v_CTexCoordinate);

    // testing
    //gl_FragColor = cubic4x4(u_CTexture, v_CTexCoordinate);
    //gl_FragColor = cubic4x4(u_CMapTexture, v_CTexCoordinate);

}
)"

