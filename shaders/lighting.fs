#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

uniform Material material;

struct Light { 
    vec4 lightVector;
    vec3 direction;
    float cutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // attenuation
    float constant;
    float linear;
    float quadratic;
};

uniform Light light;

uniform vec3 viewPos;

void main()
{
    // ambient
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir;
    float attenuation = 1.0f;

    if (light.lightVector.w == 0.0) {
        // directional light
        lightDir = normalize(-light.lightVector.xyz);
    } else {
        // point light
        vec3 lightPosition = light.lightVector.xyz;

        lightDir = normalize(lightPosition - FragPos);
        float dist = length(lightPosition - FragPos);
        attenuation = 1.0 /
                        (light.constant +
                         light.linear * dist +
                         light.quadratic * dist * dist);
    }

    float theta = dot(lightDir, normalize(-light.direction));

    if (theta > light.cutOff) {
        float diff = max(dot(norm, lightDir), 0.0f);
        vec3 diffuse = light.diffuse * (diff * vec3(texture(material.diffuse, TexCoords)));
    
        // specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * (spec * vec3(texture(material.specular, TexCoords)));

        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;

        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    } else {
        FragColor = vec4(ambient, 1.0);
    }
}
