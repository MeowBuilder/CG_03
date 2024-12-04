#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightcolor;
uniform vec3 colorAttribute;

uniform sampler2D outTexture;

void main()
{
    // 조명 계산
    vec3 ambient = 0.1 * lightcolor;

    vec3 normalVector = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float dist = distance(lightPos, FragPos);  // 조명과 프래그먼트 사이의 거리
    float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);  // 거리에 따른 감쇠

    float diff = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diff * lightcolor;

    vec3 result = (ambient + diffuse * attenuation) * colorAttribute;
    FragColor = vec4(result, 1.0);
    FragColor = texture(outTexture, TexCoord) * FragColor;
}
