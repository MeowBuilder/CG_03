#version 330 core

in vec3 passColorAttribute;
out vec4 Fragcolor;

uniform vec3 lightcolor;
uniform vec3 lightPos;

in vec3 FragPos;
in vec3 Normal;

void main()
{
    float ambientLight = 0.5;
    vec3 ambient = ambientLight * lightcolor;

    vec3 normalVector = normalize(Normal);

    vec3 lightDir = normalize(lightPos - FragPos);  
    float diffuseLight = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightcolor;

    vec3 result = (ambient + diffuse) * passColorAttribute;

    Fragcolor = vec4(result, 1.0);
};
