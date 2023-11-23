#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct Light { 
    vec3 position;
    vec3 color;
};
uniform Light lights[4];

uniform sampler2D diffuse;
uniform vec3 viewPos;

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    // phase 1: Directional lighting
    // vec3 result = CalcDirLight(dirLight, norm, viewDir);
    // phase 2: Point lights
    for (int i = 0; i < 4; i++) {
        result += CalcPointLight(lights[i], norm, FragPos, viewDir);
    }
    // phase 3: Spot light
    // todo

    FragColor = vec4(result, 1.0);

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.1)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 color = vec3(texture(diffuse, TexCoords));

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // attenuation
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (dist * dist);

    // combine results
    vec3 diffuse = light.color * diff * color;

    return diffuse * attenuation;
}