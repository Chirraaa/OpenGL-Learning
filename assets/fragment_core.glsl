#version 330 core

struct Material{
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform sampler2D diffuse0;
uniform sampler2D specular0;

struct DirLight{
	vec3 direction;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform DirLight dirLight;


#define MAX_SPOT_LIGHTS 20
struct SpotLight{
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;

	// attenuation
	float k0;
	float k1;
	float k2;

	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform SpotLight spotLight[MAX_SPOT_LIGHTS];
uniform int noSpotLights;

#define MAX_POINT_LIGHTS 20

struct PointLight{
	vec3 position;

	// attenuation
	float k0;
	float k1;
	float k2;

	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform PointLight pointLight[MAX_POINT_LIGHTS];
uniform int noPointLights;


in vec3 FragPos;
in vec3 Normal;


out  vec4 FragColor;

//in vec3 ourColor;
in vec2 TexCoord;

//uniform sampler2D texture1;
//uniform sampler2D texture2;

uniform Material material;
uniform int noTex;
uniform vec3 viewPos;

uniform int applyGrassTint;
uniform vec3 grassTintColor;

vec4 calcPointLight(int idx, vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap);

vec4 calcDirLight(vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap);

vec4 calcSpotLight(int idx, vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap);

void main(){
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	
	vec4 diffMap;
	vec4 specMap;

	if (noTex == 1){
		diffMap = material.diffuse;
		specMap = material.specular;
		}else{
		diffMap = texture(diffuse0, TexCoord);
		specMap = texture(specular0, TexCoord);

		if (applyGrassTint == 1) {
            diffMap.rgb *= grassTintColor;
        }
	}



	// placeholder
	vec4 result;
	result = calcDirLight(norm, viewDir, diffMap, specMap);

	// point lights
	for(int i = 0; i < noPointLights; i++){
		result += calcPointLight(i, norm, viewDir, diffMap, specMap);
	}

	// spot lights
	for(int i = 0; i < noSpotLights; i++){
		result += calcSpotLight(i, norm, viewDir, diffMap, specMap);
	}

	FragColor = result;
}

vec4 calcPointLight(int idx, vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap){

	//FragColor = vec4(1.0f, 0.2f, 0.6f, 1.0f);
	//FragColor = vec4(ourColor, 1.0);

	//FragColor = texture(texture1, TexCoord);

	// diffuse 
	vec4 ambient = pointLight[idx].ambient * diffMap;
	vec3 lightDir = normalize(pointLight[idx].position - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec4 diffuse = pointLight[idx].diffuse * (diff * diffMap);

	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
	vec4 specular = pointLight[idx].specular * (spec * specMap);

	float dist = length(pointLight[idx].position - FragPos);
	float attenuation = 1.0 / (pointLight[idx].k0 + pointLight[idx].k1 * dist + pointLight[idx].k2 * (dist * dist));


	return vec4(ambient + diffuse + specular)*attenuation;
}

vec4 calcDirLight(vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap){
	// ambient 
	vec4 ambient = dirLight.ambient * diffMap;

	// diffuse
	vec3 lightDir = normalize(-dirLight.direction);
	
	float diff = max(dot(norm, lightDir), 0.0);
	vec4 diffuse = dirLight.diffuse * (diff * diffMap);

	// specular
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
	vec4 specular = dirLight.specular * (spec * specMap);
	return vec4(ambient + diffuse + specular);
}

vec4 calcSpotLight(int idx, vec3 norm, vec3 viewDir, vec4 diffMap, vec4 specMap){
	// ambient 
	vec3 lightDir = normalize(spotLight[idx].position - FragPos);
	vec4 ambient = spotLight[idx].ambient * diffMap;

	float theta = dot(lightDir, normalize(-spotLight[idx].direction));

	if(theta > spotLight[idx].outerCutOff){

		float diff = max(dot(norm, lightDir), 0.0);
		vec4 diffuse = spotLight[idx].diffuse * (diff * diffMap);

		// specular
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess * 128);
		vec4 specular = spotLight[idx].specular * (spec * specMap);

		// spotlight intensity
		
		float epsilon = (spotLight[idx].cutOff - spotLight[idx].outerCutOff);
		float intensity = clamp((theta - spotLight[idx].outerCutOff) / epsilon, 0.0, 1.0);

		diffuse *= intensity;
		specular *= intensity;


		float dist = length(spotLight[idx].position - FragPos);
		float attenuation = 1.0 / (spotLight[idx].k0 + spotLight[idx].k1 * dist + spotLight[idx].k2 * (dist * dist));

		return vec4(ambient + diffuse + specular) * attenuation;
		
	}else{
		return ambient;
	}



}