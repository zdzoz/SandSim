@vs vs

layout(location = 0) in vec2 a_position; // vertex position
layout(location = 1) in vec2 a_instance_position;
layout(location = 2) in vec4 a_color;

layout(binding = 0) uniform mvp {
    mat4 model;
    mat4 view;
    mat4 projection;
};

out vec4 v_color;

void main() {
    vec4 position = vec4(a_position + a_instance_position, 0.0, 1.0);

    v_color = a_color;
    gl_Position = projection * view * model * position;
}

@end

@fs fs

out vec4 frag_Color;

in vec4 v_color;

void main() {
    frag_Color = v_color;
}

@end

@program grid vs fs
