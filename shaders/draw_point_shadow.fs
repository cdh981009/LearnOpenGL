#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float farPlane;

// blinn-phong with shadow

float calculateShadow(vec3 fragPos, float theta) {
	vec3 lightToFrag = fragPos - lightPos;
	float closestDepth = texture(depthMap, lightToFrag).r;
	closestDepth *= farPlane;
	float currentDepth = length(lightToFrag);

	float bias = max(0.01 * tan(theta), 0.005);
	float shadow = currentDepth > closestDepth + bias ? 1.0 : 0.0;
	shadow = currentDepth > farPlane ? 0.0 : shadow;

	return shadow;
}

void main() {
	vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;

	// ambient
	vec3 ambient = 0.15f * color;

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
	float shadow = calculateShadow(fs_in.FragPos, acos(dot(lightDir, norm)));
	vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

	FragColor = vec4(pow(lighting, vec3(1/1.2)), 1.0);
}