#version 400

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTextureCoordinate0;
in vec4 VertexColor;

layout (std140) uniform Matrices
{
	mat4 ModelViewMatrix;//no more gl_matrices
    mat4 ProjectionMatrix;//no more matrices
    mat3 NormalMatrix;// no more matrices
};

uniform sampler2D texUnit0;

uniform vec4 LightPosition;

out vec3 ecPosition;
out vec3 normal;
out vec2 textureCoord0;

void main (void)
{
    // Eye-coordinate position of vertex, needed in various calculations
    ecPosition = vec3(ModelViewMatrix * vec4(VertexPosition,1));
    normal = normalize(NormalMatrix * VertexNormal);

    textureCoord0 = VertexTextureCoordinate0;//worry about texture matrices later
    // Do fixed functionality vertex transform
    gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(VertexPosition,1);
}