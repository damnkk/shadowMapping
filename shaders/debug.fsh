#version 330 core

in vec2 texcoord;
out vec4 fColor;

uniform sampler2D shadowtex;

void main()
{
    fColor = vec4(vec3(texture2D(shadowtex,texcoord).r*0.5+0.5),1.0);
    //fColor = vec4(0.5,0.5,0.5,1.0);
}