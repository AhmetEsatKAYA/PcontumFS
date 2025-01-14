#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix; // Kullanım isteğe bağlı

void main()
{
    // Doku koordinatlarını hesapla
    TexCoords = aTexCoords;

    // Pozisyonları hesapla
    WorldPos = vec3(model * vec4(aPos, 1.0));
    FragPos = WorldPos; // Aynı veriyi tekrar hesaplamamak için yeniden kullanıyoruz

    // Normal hesaplaması (model matrisine göre)
    if (normalMatrix != mat3(0.0)) {
        Normal = normalMatrix * aNormal;
    } else {
        Normal = mat3(transpose(inverse(model))) * aNormal;
    }

    // Nihai pozisyonu hesapla
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
