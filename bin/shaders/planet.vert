#version 330

// vertex attributes
in vec3 position;
in vec3 normal;
in vec2 texcoord;

// matrices
uniform mat4 model_matrix;
uniform mat4 view_projection_matrix;
uniform float aspect_ratio;

out vec2 tc;
out vec3 norm;
out uint color_mode_f;

void main() {
    gl_Position = view_projection_matrix * model_matrix * vec4(position, 1);
    gl_Position.xy *= aspect_ratio > 1 ? vec2(1 / aspect_ratio, 1) : vec2(1, aspect_ratio);

    tc = texcoord;
    norm = normalize(mat3(view_projection_matrix)*normal);
}
