#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform bool blinn;

void main() {
	vec3 color = texture(texture1, TexCoords).rgb;

	// ambient
	vec3 ambient = 0.05f * color;

	// diffuse
	vec3 lightDir = normalize(lightPos - FragPos);
	vec3 norm = normalize(Normal);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = diff * color;

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, Normal);
	float shininess = 32.0;
	vec3 halfway = normalize(lightDir + viewDir);
	float spec = 0.0;
	if (blinn) {
		spec = pow(max(dot(Normal, halfway), 0.0f), shininess);
	} else {
		spec = pow(max(dot(reflectDir, viewDir), 0.0f), shininess);
	}
	float specularStrength = 0.3;
	vec3 specular = specularStrength * vec3(1.0) * spec;

	FragColor = vec4(ambient + diffuse + specular, 1.0);
}