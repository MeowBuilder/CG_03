#version 330 core

uniform mat4 transform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;

layout (location = 0) in vec3 positionAttribute;
layout (location = 1) in vec3 normalAttribute;

out vec3 FragPos;
out vec3 Normal;

uniform vec3 colorAttribute;
out vec3 passColorAttribute;

void main()
{
    // 뷰 공간에서의 위치
    vec4 viewPos = viewTransform * transform * vec4(positionAttribute, 1.0);
    FragPos = viewPos.xyz;
    
    // 뷰 공간에서의 노말
    Normal = normalize(mat3(viewTransform * transform) * normalAttribute);
    
    gl_Position = projectionTransform * viewPos;
    
    passColorAttribute = colorAttribute;
}
