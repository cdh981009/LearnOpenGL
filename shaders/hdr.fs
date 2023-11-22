#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D hdrBuffer;

void main() {
    const float gamma = 2.2;
    
    // reinhard tone mapping
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}