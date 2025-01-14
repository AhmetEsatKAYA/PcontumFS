#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/model.h>
#include <learnopengl/camera.h>
#include <learnopengl/shader_m.h>
#include <chrono>
#include <thread>

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera and object variables
glm::vec3 airplanePosition(0.0f, 0.0f, 5.0f); // Start higher in the air
glm::vec3 cameraOffset(0.0f, 8.0f, 0.0f); // Camera is slightly above and behind the airplane
Camera camera(airplanePosition + cameraOffset);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// airplane control variables
float pitch = 0.0f; // x-axis rotation
float yaw = 0.0f;   // y-axis rotation
float roll = 180.0f;  // z-axis rotation
float speed = 10.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void updateCamera(); // Prototip eklendi

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Airplane Simulator", primaryMonitor, NULL);

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");
    Model airplaneModel(FileSystem::getPath("resources/objects/fonalti/fonalti.dae"));
    Model groundModel(FileSystem::getPath("resources/objects/ettayyariyyetul_gemiyye/ettayyariyyetul_gemiyye.dae"));

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        processInput(window);


    updateCamera();

        glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // Sky blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Update camera position to follow the airplane
        camera.Position = airplanePosition + cameraOffset; // Uçağın arkasına yerleştir
        camera.updateCameraVectors(); // Kamera yön vektörlerini güncelle


        // Model matrisi (uçak için)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, airplanePosition); // Pozisyonu uygula
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        model = glm::rotate(model, glm::radians(pitch + 180), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        model = glm::rotate(model, glm::radians(roll + 180), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll


        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);

        airplaneModel.Draw(ourShader);

        // Ground model
        glm::mat4 groundModelMatrix = glm::mat4(1.0f);
        groundModelMatrix = glm::scale(groundModelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
        
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(360.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

        ourShader.setMat4("model", groundModelMatrix);
        groundModel.Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float movementSpeed = speed * deltaTime;
    float rotationSpeed = 50.0f * deltaTime;
    float cameraMoveSpeed = 5.0f * deltaTime; // Kamera hareket hızı

    // Hızlandırma ve yavaşlatma
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) // '+' tuşu
        speed += 10.0f * deltaTime; // Hızı artır
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) // '-' tuşu
        speed = glm::max(speed - 10.0f * deltaTime, 0.0f); // Hızı azalt, minimum 0

    // Uçağı durdurma
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        speed = 0.0f; // Hızı sıfırla

    // Uçağın burun hareketlerini kontrol eden tuşlar
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pitch -= rotationSpeed; // Burun yukarı
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pitch += rotationSpeed; // Burun aşağı

    // Uçağın yön değiştirme hareketlerini kontrol eden tuşlar
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        roll -= rotationSpeed; // Roll sola
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        roll += rotationSpeed; // Roll sağa

    // Uçağın sağa-sola yatma hareketlerini kontrol eden tuşlar
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        yaw -= rotationSpeed; // Sola dönüş (yaw etkisi)
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        yaw += rotationSpeed; // Sağa dönüş (yaw etkisi)

    // Eğer hız sıfır değilse pozisyon güncelle
    if (speed > 0.0f) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));         // Pitch
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));          // Roll

        glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)); // İleri vektör
        airplanePosition += forward * movementSpeed; // Yeni hareket yönüyle pozisyonu güncelle
    }

    // Kamera offseti ok tuşları ile değiştir
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraOffset.z += cameraMoveSpeed; // Kamerayı yukarı kaydır
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraOffset.z -= cameraMoveSpeed; // Kamerayı aşağı kaydır
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraOffset.x -= cameraMoveSpeed; // Kamerayı sola kaydır
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraOffset.x += cameraMoveSpeed; // Kamerayı sağa kaydır
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        cameraOffset.y -= cameraMoveSpeed; // Kamerayı sola kaydır
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        cameraOffset.y += cameraMoveSpeed; // Kamerayı sağa kaydır

    // Debug için uçak ve kamera durumu
    std::cout << "Airplane Position: " << airplanePosition.x << ", " << airplanePosition.y << ", " << airplanePosition.z
              << " | Speed: " << speed << " | Pitch: " << pitch << " | Yaw: " << yaw << " | Roll: " << roll
              << " | Camera Offset: " << cameraOffset.x << ", " << cameraOffset.y << ", " << cameraOffset.z << std::endl;
}




void updateCamera()
{
    // Kameranın sabit mesafesi ve yüksekliği
    float cameraDistance = 10.0f; // Kameranın uçağa uzaklığı
    float cameraHeight = 3.0f;    // Kameranın uçağa göre yüksekliği

    // Uçağın dönüş matrisini oluştur
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));         // Pitch
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));          // Roll

    // Kameranın uçağa göre pozisyonunu hesapla (çember hareketi)
    glm::vec3 offset = glm::vec3(0.0f, cameraHeight, cameraDistance); // Sabit bir mesafe ve yükseklik
    glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(offset, 1.0f)); // Dönüş matrisini uygula

    // Kamera pozisyonunu hesapla
    camera.Position = airplanePosition - rotatedOffset;

    // Kamera yönünü uçağa bakacak şekilde ayarla
    camera.Front = glm::normalize(airplanePosition - camera.Position);
    camera.Up = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))); // Yukarı vektör
}






void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    static float lastX = SCR_WIDTH / 2.0f;
    static float lastY = SCR_HEIGHT / 2.0f;
    static bool firstMouse = true;

    if (firstMouse)
    {
        lastX = xposIn;
        lastY = yposIn;
        firstMouse = false;
    }

    // Mouse hareketini hesaplarken yalnızca kamerayı döndürme amacıyla kullanılan xoffset ve yoffset'i hesaplayabilirsiniz.
    float xoffset = xposIn - lastX;
    float yoffset = lastY - yposIn; // Y ekseni ters çevrildiği için bu şekilde hesaplanır.

    lastX = xposIn;
    lastY = yposIn;

    // Kamera işlemleri burada yapılır; yaw ve pitch değerleri doğrudan etkilenmez.
    // Eğer bu işlemleri tamamen etkisiz hale getirmek istiyorsanız aşağıdaki satırı kaldırabilirsiniz.
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
