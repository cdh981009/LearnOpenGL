#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

// blinn-phong with shadow

float calculateShadow(vec4 fragPosLightSpace, vec3 norm, vec3 lightDir) {
	// perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform from [-1, 1] NDC to [0, 1] for texture sampling
	projCoords = projCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float bias = max(0.005 * (1.0 - dot(norm, lightDir)), 0.001);
	float shadow = currentDepth > closestDepth + bias ? 1.0 : 0.0;
	return shadow;
}

void main() {
	vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;

	// ambient
	vec3 ambient = 0.1f * color;

	// diffuse
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	vec3 norm = normalize(fs_in.Normal);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = diff * color;

	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float shininess = 32.0;
	vec3 halfway = normalize(lightDir + viewDir);
	float spec = 0.0;
	spec = pow(max(dot(norm, halfway), 0.0f), shininess);
	// spec = pow(max(dot(reflectDir, viewDir), 0.0f), shininess); // phong
	float specularStrength = 0.5;
	vec3 specular = specularStrength * vec3(1.0) * spec;

	// shadow
	float shadow = calculateShadow(fs_in.FragPosLightSpace, norm, lightDir);
	vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

	FragColor = vec4(lighting, 1.0);
}