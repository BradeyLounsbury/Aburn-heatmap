#version 420
//Written by Scott Nykl. Default Shader for a Core 3.2+.
//Performs per fragment ambient/diffuse/specular shading with material
//properties, textures, and lights.

layout ( binding = 0 ) uniform sampler2D TexUnit0;
layout ( binding = 7 ) uniform sampler2DShadow ShadowMap;

in vec4 Color;
in vec3 VertexES;
in vec3 NormalES;
in vec2 TexCoord;
in vec4 ShadowCoord;
in flat int ShadowMapShadingState;
in vec3 VertexPos;

uniform float max_height;

struct MaterialInfo
{
   vec4 Ka; //Ambient
   vec4 Kd; //Diffuse
   vec4 Ks; //Specular
   float SpecularCoeff; // Specular Coefficient
};
uniform MaterialInfo Material;


layout ( binding = 1, std140 ) uniform LightInfo
{
   vec4 PosEye[8]; // Light's Eye space position (same as view space)
   vec4 Irgba[8]; // Light's Intensity for red, green, blue reflectivity components
   vec4 GlobalAmbient;
   int NumLights; // Number of lights in the LightInfo array   
} Lights;

layout ( location = 0 ) out vec4 FragColor;

//if Lights.NumLights is zero, the alpha channel of ambient, diffuse, and specular will be 0 because
//doADS is never called since num lights is zero. As a result, when adsColor is blended with the
//its texture color, the alpha will remain zero. This will make all WOs draw invisible (this is
//probably not the best behavior for a World with no lights, we probably just want to use the global
//ambient).
//To correct this we will set the adsColor.a to be 1, in this case.
vec4 useGlobalAmbient_and_Diffuse_when_zero_lights( vec4 texColor, vec4 ambient, vec4 diffuse )
{
   if( Lights.NumLights == 0 )
   {
      //float alpha = min( texColor.a, ambient.a );
      ambient += Material.Ka + vec4( Lights.GlobalAmbient.rgb, 1 );
      diffuse += Material.Kd * vec4( Lights.GlobalAmbient.rgb, 1 );

      vec4 adsColor = vec4( ambient + diffuse + vec4( Lights.GlobalAmbient.rgb, 0 ) );
      adsColor.rgba = clamp( adsColor, vec4(0,0,0,0), vec4( 1.0, 1.0, 1.0, 1.0) ) ;
      vec4 final = texColor * adsColor;
      return final;
   }
}

void doADS( int lightIdx, inout vec4 ambient, inout vec4 diffuse, inout vec4 specular )
{
   vec3 n = normalize( NormalES );
   vec3 l = normalize( -Lights.PosEye[lightIdx].xyz ); //Assume directional light, if it is spot, it will get updated next
   if( Lights.PosEye[lightIdx].w > 0.5 ) //if this light is a SPOT Light (it has a position), then perform spot light computations
      l = normalize( Lights.PosEye[lightIdx].xyz - VertexES ); //vector to light source
   vec3 v = normalize( -VertexES ); //used by spec
   vec3 r = reflect( -l, n );       //used by spec
   
   //Compute Ambient Contribution
   ambient += Material.Ka * Lights.Irgba[lightIdx].rgba;
   
   //Compute Diffuse Contribution
   float diff = clamp( dot( l, n ), 0.0, 1.0 );
   diffuse += Material.Kd * diff * Lights.Irgba[lightIdx].rgba;
   
   //Compute Specular Contribution
   float spec = pow( max( dot( r, v ), 0.0 ), Material.SpecularCoeff );
   specular += Material.Ks  * spec * Lights.Irgba[lightIdx].rgba;
}

vec4 doADS_Textures_Shadows()
{
   vec4 ambient  = vec4(0,0,0,0);
   vec4 diffuse  = vec4(0,0,0,0);
   vec4 specular = vec4(0,0,0,0);
   vec4 texColor = texture( TexUnit0, TexCoord );
   
   //This block of code performs the same functionality as calling textureProjOffset,
   //here for reference and testing
   //float shadow = textureProj( ShadowMap, ShadowCoord );
   //vec4 shadowCoordNDC = ShadowCoord.xyzw / ShadowCoord.w; //perspective divide, bias matrix already transformed values to [0,1]   
   //float shadow = 0;
   //shadow += textureOffset( ShadowMap, shadowCoordNDC.xyz, ivec2( -1, -1 ), 0 );
   //shadow += textureOffset( ShadowMap, shadowCoordNDC.xyz, ivec2( -1,  1 ), 0 );
   //shadow += textureOffset( ShadowMap, shadowCoordNDC.xyz, ivec2(  1, -1 ), 0 );
   //shadow += textureOffset( ShadowMap, shadowCoordNDC.xyz, ivec2(  1,  1 ), 0 );
   //shadow += texture( ShadowMap, shadowCoordNDC.xyz, 0 );
   //shadow *= 0.2;
   
   //This code performs shadow mapping using PCF (Percent Closest Filtering)
   float shadow = 0; // Sum of the compairsons with nearby texels   
   //shadow += textureProjOffset( ShadowMap, ShadowCoord, ivec2( -1, -1 ), 0.0 );
   //shadow += textureProjOffset( ShadowMap, ShadowCoord, ivec2( 0,  -1 ), 0.0 );
   //shadow += textureProjOffset( ShadowMap, ShadowCoord, ivec2(  1,  1 ), 0.0 );
   //shadow += textureProjOffset( ShadowMap, ShadowCoord, ivec2(  -1, 0 ), 0.0 );
   shadow += textureProj( ShadowMap, ShadowCoord );
   //shadow *= 0.3333;
   
   
   for( int i = 0; i < Lights.NumLights; ++i )
      doADS( i, ambient, diffuse, specular );
   
   float alpha = min( texColor.a, ambient.a );
   vec4 adsColor = vec4( ambient + ( (diffuse + specular) * shadow ) ) + vec4( Lights.GlobalAmbient.rgb, 0 );
   adsColor = min( adsColor, vec4( 1.0, 1.0, 1.0, 1.0 ) ); //ensure textures don't get washed out (above 1.0 per channel)
   vec4 final = texColor * adsColor;
   final.a = alpha;
   if( Lights.NumLights == 0 )
      final = useGlobalAmbient_and_Diffuse_when_zero_lights( texColor, ambient, diffuse ); //use ambient/diffuse w/ no lights
   return final;
}

vec4 doADS_Textures()
{
   vec4 ambient  = vec4(0,0,0,0);
   vec4 diffuse  = vec4(0,0,0,0);
   vec4 specular = vec4(0,0,0,0);
   vec4 texColor = texture( TexUnit0, TexCoord );

   //float max_height = 3;
   float percent_of_max = VertexPos.z / max_height;
   float height_in_degrees = percent_of_max * 300;

   if (height_in_degrees < 60) {
      float x = 1 - abs(mod((height_in_degrees / 60), 2) - 1);
      texColor = vec4(0, x, 1, 1);
   } else if (height_in_degrees <= 120) {
      float x = ((height_in_degrees - 60) / 60);
      texColor = vec4(0, 1, 1-x, 1);
   } else if (height_in_degrees <= 180) {
      float x = ((height_in_degrees - 120) / 60);
      texColor = vec4(x, 1, 0, 1);
   } else if (height_in_degrees <= 240) {
      float x = ((height_in_degrees - 180) / 60);
      texColor = vec4(1, 1-x, 0, 1);
   } else if (height_in_degrees <= 300) {
      float x = ((height_in_degrees - 240) / 60);
      texColor = vec4(1, 0, x, 1);
   }

   
   
   for( int i = 0; i < Lights.NumLights; ++i )
      doADS( i, ambient, diffuse, specular );
   
   //Let the transparency be determined by the smallest (most transparent alpha value) passed into the
   //fragment's Ambient Material property alpha OR the fragment's Texture alpha channel
   float alpha = min( texColor.a, ambient.a );
   vec4 adsColor = vec4( ambient + ( diffuse + specular ) ) + vec4( Lights.GlobalAmbient.rgb, 0 );
   adsColor = min( adsColor, vec4( 1.0, 1.0, 1.0, 1.0 ) );

   vec4 final = texColor * adsColor;
   final.a = alpha;


   if( Lights.NumLights == 0 )
      final = useGlobalAmbient_and_Diffuse_when_zero_lights( texColor, ambient, diffuse ); //use ambient/diffuse w/ no light

   
   return final;
}

//vec4 doADS_NoTextures()
//{
//   vec4 ambient  = vec4(0,0,0,0);
//   vec4 diffuse  = vec4(0,0,0,0);
//   vec4 specular = vec4(0,0,0,0);
//   
//   for( int i = 0; i < Lights.NumLights; ++i )
//      doADS( i, ambient, diffuse, specular );
//   
//   vec4 final = vec4( ambient + ( diffuse + specular ) ) + vec4( Lights.GlobalAmbient.rgb, 0 );
//   return final;
//}

vec4 doDepthBufferGenerationOnly()
{
   return vec4( 1, 1, 0, 1 ); //just need to ensure the depth buffer is populated, we don't care about the color buffer
}

void main()
{
   vec4 color = vec4(0,0,0,1);
   
   if( ShadowMapShadingState == 2 )
      color = doADS_Textures_Shadows();
   else if( ShadowMapShadingState == 1 )
      color = doDepthBufferGenerationOnly();
   else
      color = doADS_Textures();
      
   //FragColor = vec4(1,0,1,1);
   FragColor = color;
}