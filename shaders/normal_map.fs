#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 Normal;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;
uniform sampler2D normalMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float farPlane;
uniform bool useNormalMap;

// blinn-phong with shadow

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);  

float calculateShadow(vec3 fragPos, float theta) {
	vec3 lightToFrag = fragPos - lightPos;
	float currentDepth = length(lightToFrag);

	float shadow  = 0.0;
	int samples = 20;
	// float viewDistance = length(viewPos - fragPos); // for adaptive diskRadius
	float diskRadius = 0.02;
	float bias = max(0.05 * tan(theta), 0.04);

	for (int i = 0; i < samples; ++i) {
		vec3 samplingPoint = lightToFrag + sampleOffsetDirections[i] * diskRadius;
		float closestDepth = texture(depthMap, samplingPoint).r * farPlane;	
		if (currentDepth > closestDepth + bias)
			shadow += 1.0;
	}

	shadow /= samples;
	shadow = currentDepth > farPlane ? 0.0 : shadow;

	return shadow;
}

void main() {
	vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;

	// ambient
	vec3 ambient = 0.15f * color;

	// diffuse
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	vec3 norm;
	if (useNormalMap) {
		norm = texture(normalMap, fs_in.TexCoords).rgb * 2.0 - 1.0;
		norm = normalize(norm);
	} else {
		norm = normalize(fs_in.Normal); 
	}
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = diff * color;

	// specular
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
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
	// FragColor = vec4(vec3(shadow), 1.0); // for shadow debugging
	// FragColor = vec4(norm, 1.0); // for shadow debugging
}