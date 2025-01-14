#version 330 core
struct Material {
    sampler2D texture_diffuse1;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    vec3 ambient, diffuse, specular;

    // Texture RGB değerini al
    vec3 textureColor = texture(material.texture_diffuse1, TexCoords).rgb;

    // Eğer doku varsa (dokunun r değeri sıfır değilse), sadece dokuyu kullan.
    if (texture(material.texture_diffuse1, TexCoords).r != 0.0) {
        // Doku varsa, aydınlatma uygulanmadan doğrudan dokuyu göster
        FragColor = vec4(textureColor, 1.0);
    } else {
        // Eğer doku yoksa, aydınlatmayı uygula
        // Ambient
        ambient = light.ambient * material.diffuse;

        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(light.position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        diffuse = light.diffuse * diff * material.diffuse;

        // Specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        specular = light.specular * spec * material.specular;

        FragColor = vec4(ambient + diffuse + specular, 1.0);
    }
}
