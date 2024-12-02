#version 330 core

in vec3 passColorAttribute;
out vec4 Fragcolor;

uniform vec3 lightcolor;
uniform vec3 lightPos;

in vec3 FragPos;
in vec3 Normal;

void main()
{
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightcolor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightcolor;

    float specularStrength = 0.3;
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightcolor;

    vec3 result = (ambient + diffuse + specular) * passColorAttribute;
    Fragcolor = vec4(result, 1.0);
}
