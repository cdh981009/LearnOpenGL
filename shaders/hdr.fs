#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D hdrBuffer;
uniform sampler2D bloom;
uniform float exposure;

void main() {
    const float gamma = 2.2;
    
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    vec3 bloom = texture(bloom, TexCoords).rgb;

    vec3 color = hdrColor + bloom;
    vec3 result = vec3(1.0) - exp(-color * exposure);

    result = pow(result, vec3(1.0 / gamma));

    FragColor = vec4(result, 1.0);
}