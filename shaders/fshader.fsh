#version 330 core

in vec3 worldPos;
in vec2 texcoord;
in vec3 normal;

out vec4 fColor;


uniform sampler2D texture;
uniform sampler2D shadowtex;


uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform mat4 shadowVP;

float shadowMapping(sampler2D tex,mat4 shadowVP,vec4 worldPos)
{
    vec4 lightPos = shadowVP* worldPos;
    lightPos = vec4(lightPos.xyz/lightPos.w,1.0);
    lightPos = lightPos*0.5+0.5;

    float closeDepth = texture2D(tex,lightPos.xy).r;
    float currentDepth = lightPos.z;
    float isInshadow = (currentDepth-0.0005)<=closeDepth?(0.0):(1.0);
    return isInshadow;
}

float phone (vec3 worldPos,vec3 normal,vec3 lightPos,vec3 cameraPos)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(worldPos-cameraPos);
    vec3 L = normalize(worldPos-lightPos);
    vec3 R = reflect(L,N);

    float ambient = 0.3;
    float diffuse = max(dot(N,-L),0)*0.7;
    float specular = pow(max(dot(-V,R),0),36.0)*1.1;
    return ambient +diffuse + specular;
}

void main()
{
    //fColor = vec4(0.5,0.5,0.5,1.0);
    fColor.rgb = vec3(0.5,0.5,0.5);
    float isInshadow = shadowMapping(shadowtex,shadowVP,vec4(worldPos,1.0));
    if(isInshadow==0)
    {
        fColor.rgb *= phone(worldPos,normal,lightPos,cameraPos);
    }
    else
    {
        fColor.rgb*=0.3;
    }
}


