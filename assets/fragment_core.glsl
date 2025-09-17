#version 330 core

struct Material{
	vec3 ambient;
	sampler2D diffuse;
	vec3 specular;
	float shininess;
};

struct DirLight{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform DirLight dirLight;

struct SpotLight{
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;

	// attenuation
	float k0;
	float k1;
	float k2;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform SpotLight spotLight;

struct PointLight{
	vec3 position;

	// attenuation
	float k0;
	float k1;
	float k2;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform PointLight pointLight;

in vec3 FragPos;
in vec3 Normal;


out  vec4 FragColor;

//in vec3 ourColor;
in vec2 TexCoord;

//uniform sampler2D texture1;
//uniform sampler2D texture2;

uniform Material material;
uniform vec3 viewPos;


vec3 calcPointLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap);

vec3 calcDirLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap);

vec3 calcSpotLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap);

void main(){
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 diffMap = vec3(texture(material.diffuse, TexCoord));
	vec3 specMap = vec3(texture(material.diffuse, TexCoord));

	FragColor = vec4(calcSpotLight(norm, viewDir, diffMap, specMap), 1.0);
}

vec3 calcPointLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap){

	//FragColor = vec4(1.0f, 0.2f, 0.6f, 1.0f);
	//FragColor = vec4(ourColor, 1.0);

	//FragColor = texture(texture1, TexCoord);

	// diffuse 
	vec3 ambient = pointLight.ambient * diffMap;
	vec3 lightDir = normalize(pointLight.position - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = pointLight.diffuse * (diff * diffMap);

	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
	vec3 specular = pointLight.specular * (spec * specMap);

	float dist = length(pointLight.position - FragPos);
	float attenuation = 1.0 / (pointLight.k0 + pointLight.k1 * dist + pointLight.k2 * (dist * dist));


	return vec3(ambient + diffuse + specular)*attenuation;
}

vec3 calcDirLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap){
	// ambient 
	vec3 ambient = dirLight.ambient * diffMap;

	// diffuse
	vec3 lightDir = normalize(-dirLight.direction);
	
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = dirLight.diffuse * (diff * diffMap);

	// specular
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
	vec3 specular = dirLight.specular * (spec * specMap);
	return vec3(ambient + diffuse + specular);
}

vec3 calcSpotLight(vec3 norm, vec3 viewDir, vec3 diffMap, vec3 specMap){
	// ambient 
	vec3 lightDir = normalize(spotLight.position - FragPos);
	vec3 ambient = spotLight.ambient * diffMap;

	float theta = dot(lightDir, normalize(-spotLight.direction));

	if(theta > spotLight.cutOff){

		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = spotLight.diffuse * (diff * diffMap);

		// specular
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
		vec3 specular = spotLight.specular * (spec * specMap);

		// spotlight intensity
		
		float epsilon = (spotLight.cutOff - spotLight.outerCutOff);
		float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

		diffuse *= intensity;
		specular *= intensity;


		float dist = length(spotLight.position - FragPos);
		float attenuation = 1.0 / (spotLight.k0 + spotLight.k1 * dist + spotLight.k2 * (dist * dist));

		return vec3(ambient + diffuse + specular) * attenuation;
		
	}else{
		return ambient;
	}



}