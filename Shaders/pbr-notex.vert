#version 450
layout (location=0) in vec3 inPos;
layout (location=1) in vec3 inNormal;
layout (location=2) in vec2 inTexCoords;

layout (location=0) out vec3 outWorldPos;
layout (location=1) out vec3 outNormal;
layout (location=2) out vec2 outTexCoords;

layout (set=0,binding=0) uniform UBO{
	mat4 projection;
	mat4 view;
	//mat4 model;
	vec3 camPos;
	//float metallic;//alignment
	float ao;
	//vec3 albedo;	
	//float roughness;
	//float ao;	
};

layout (set=0,binding=1) readonly buffer Lights{
	vec3 lightPositions[4];
	vec3 lightColors[4];
}lights;

layout (push_constant) uniform PushConst{
	mat4 model;
	vec3 albedo;
	float metallic;
	float roughness;
};

void main(){
		outTexCoords=inTexCoords;
		outWorldPos=vec3(model*vec4(inPos,1.0));
		outNormal = mat3(model)*inNormal;
		gl_Position = projection*view*vec4(outWorldPos,1.0);
}