#version 450 
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outUV;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

//out gl_PerVertex { vec4 gl_Position; };
//layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    //Out.Color = aColor;
	outColor=aColor;
    //Out.UV = aUV;
	outUV=aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}