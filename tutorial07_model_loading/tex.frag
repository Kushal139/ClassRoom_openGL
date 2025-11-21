#version 330 core

in vec2 UV;
in vec3 FragPos;
in vec3 Normal;

out vec4 color;

uniform sampler2D texture_diffuse;

void main() {
    color = texture(texture_diffuse, UV);
}
