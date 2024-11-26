#version 330 core

in vec3 passColorAttribute;
out vec4 Fragcolor;

uniform vec3 lightcolor;
uniform vec3 lightPos;

in vec3 FragPos;
in vec3 Normal;

void main()
{
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightcolor;

    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * lightcolor;

    vec3 result = (ambient + diffuse) * passColorAttribute;

    Fragcolor = vec4(result, 1.0);
};
