R"(#version 300 es

uniform mat3 u_MVPMatrix;		// A constant representing the combined model/view/projection matrix.

layout(location = 0) in vec2 a_Position;		        // Per-vertex position information we will pass in.
layout(location = 1) in vec2 a_TexCoordinate;           // Per-vertex texture coordinate information we will pass in.

out vec2 v_TexCoordinate;       // This will be passed into the fragment shader.

// The entry point for our vertex shader.
void main()
{
	// Pass through the texture coordinate.
	v_TexCoordinate = a_TexCoordinate;

	vec3 vPos = vec3(a_Position, 1.0);
    vec3 nvPos = u_MVPMatrix * vPos;

	gl_Position = vec4(nvPos.x, nvPos.y, 0.0, 1.0);
}
)"
