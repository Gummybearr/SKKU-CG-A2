#version 330

// input from vertex shader
in vec3 norm;

// the only output variable
out vec4 fragColor;

void main()
{
	fragColor = vec4(normalize(norm), 1.0);
}
