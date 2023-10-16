#version 400

in vec3 ecPosition;
in vec3 normal;
in vec2 textureCoord0;

uniform sampler2D texUnit0;
uniform vec4 LightPosition;
uniform float shininess;
/*
layout (std140) uniform Light
{
	
};
*/
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;
/*
void pointLight(in int i, in vec3 normal, in vec3 eye, in vec3 ecPosition3)
{
   float nDotVP;       // normal . light direction
   float nDotHV;       // normal . light half vector
   float pf;           // power factor
   float attenuation;  // computed attenuation factor
   float d;            // distance from surface to light source
   vec3  VP;           // direction from surface to light position
   vec3  halfVector;   // direction of maximum highlights

   // Compute vector from surface to light position
   VP = gl_LightSource[i].position.xyz - ecPosition3;

   // Compute distance between surface and light position
   d = length(VP);

   // Normalize the vector from surface to light position
   VP = normalize(VP);

   // Compute attenuation
   //attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +
     //  gl_LightSource[i].linearAttenuation * d +
       //gl_LightSource[i].quadraticAttenuation * d * d);

   nDotVP = max(0.0, dot(normal, VP));

   vec3 R = normalize(-reflect(VP, normal));
   pf = pow(max(dot(R,eye),0.0),.3*shininess);

   attenuation = 1;
   Ambient  += gl_LightSource[i].ambient * attenuation;
   Diffuse  += gl_LightSource[i].diffuse * nDotVP * attenuation;
   Specular += gl_FrontLightProduct[i].specular * pf * attenuation;
}
*/
out vec4 colorOut;

void main(void)
{
   vec4 color;
   vec3 eye;

   eye = vec3 (0,0,1);

   Ambient = vec4(0.0);
   Diffuse = vec4(0.0);
   Specular = vec4(0.0);

   //pointLight(0, normal, eye, ecPosition);
/*
   color = gl_FrontLightModelProduct.sceneColor +
     Ambient * gl_FrontMaterial.ambient +
     Diffuse * gl_FrontMaterial.diffuse;
   color += Specular;
   color = clamp( color, 0.0, 1.0);
*/
   vec4 texel = texture(texUnit0,textureCoord0.st);
   color = texel;
   colorOut = color;
}