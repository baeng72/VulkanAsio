#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
//layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inUV;
void main()
{
    fColor = inColor * texture(sTexture, inUV);
}