#version 330

// input from vertex shader
in vec2 tc;
in vec3 norm;

// uniform variable
uniform uint color_mode;

// the only output variable
out vec4 fragColor;

void main() {
    if(color_mode == 1u) {
        fragColor = vec4(tc.xy, 0, 1);
    } else if(color_mode == 2u) {
        fragColor = vec4(tc.xxx, 1);
    } else if(color_mode == 3u) {
        fragColor = vec4(tc.yyy, 1);
    }
}
