@vs vs

in vec3 position;
in vec4 color0;
out vec4 v_color0;

void main() {
    v_color0 = color0;
    gl_Position = vec4(position, 1.0);
}

@end

@fs fs

out vec4 frag_Color;
in vec4 v_color0;

void main() {
    frag_Color = v_color0;
}

@end

@program basic vs fs
