#version 330 core

struct Material{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light{
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;


out  vec4 FragColor;

//in vec3 ourColor;
in vec2 TexCoord;

//uniform sampler2D texture1;
//uniform sampler2D texture2;

uniform Material material;
uniform Light light;
uniform vec3 viewPos;


void main(){
	//FragColor = vec4(1.0f, 0.2f, 0.6f, 1.0f);
	//FragColor = vec4(ourColor, 1.0);

	//FragColor = texture(texture1, TexCoord);

	vec3 ambient = light.ambient * material.ambient;
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.position - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);

	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
	vec3 specular = light.specular * (spec * material.specular);

	FragColor = vec4(vec3(ambient + diffuse + specular), 1.0);
}