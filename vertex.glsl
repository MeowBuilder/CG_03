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
    // 월드 공간에서의 위치 계산
    vec4 worldPos = transform * vec4(positionAttribute, 1.0);
    // 뷰 공간에서의 위치 계산
    vec4 viewPos = viewTransform * worldPos;
    FragPos = viewPos.xyz;
    
    // 노말 변환 행렬 계산 (transpose(inverse(mat3(viewTransform * transform))))
    mat3 normalMatrix = mat3(transpose(inverse(viewTransform * transform)));
    Normal = normalize(normalMatrix * normalAttribute);
    
    gl_Position = projectionTransform * viewPos;
    passColorAttribute = colorAttribute;
}
