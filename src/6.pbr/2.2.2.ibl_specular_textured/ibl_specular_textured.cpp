#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/quaternion.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <math.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexturef(const char *path);
void renderSphere();
void renderCube();
void renderQuad(float width);
void updateCamera(); // Prototip eklendi

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1020;

// camera

glm::vec3 airplanePosition(-102.815, 1, -59.034); // Start higher in the air
glm::vec3 cameraOffset(0.0f, 8.0f, 0.0f); // Camera is slightly above and behind the airplane
Camera camera(airplanePosition + cameraOffset);

bool isPlaneTouchGround;
bool pressingS;
// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// airplane control variables
float pitch = 0.0f; // x-axis rotation
float yaw = 0.0f;   // y-axis rotation
float roll = 0.0f;  // z-axis rotation
float speed = 1.0f;

float groundscale = 0.3f;
float airplanescale = 0.15f;

bool cobra = false;

float lastPitch = 0.0f;
float lastPitchUpdateTime = 0.0f;
float cobraEndTime = 0.0f;
float pitchRate;
float cobraStartTime = 0.0f;



int main()
{

    #pragma region Baslangis Islemleri

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Mouse imlecini gizle ve kontrolü al.

    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // set depth function to less than AND equal for skybox depth trick.
    glDepthFunc(GL_LEQUAL);
    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // build and compile shaders
    // -------------------------
    Shader pbrShader("2.2.2.pbr.vs", "2.2.2.pbr.fs");
    Shader equirectangularToCubemapShader("2.2.2.cubemap.vs", "2.2.2.equirectangular_to_cubemap.fs");
    Shader irradianceShader("2.2.2.cubemap.vs", "2.2.2.irradiance_convolution.fs");
    Shader prefilterShader("2.2.2.cubemap.vs", "2.2.2.prefilter.fs");
    Shader brdfShader("2.2.2.brdf.vs", "2.2.2.brdf.fs");
    Shader backgroundShader("2.2.2.background.vs", "2.2.2.background.fs");

    
    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    
    Model airplaneModel(FileSystem::getPath("resources/objects/kaan/kaan.dae"));
    Model groundModel(FileSystem::getPath("resources/objects/ettayyariyyetul_gemiyye/ettayyariyyetul_gemiyye.dae"));

    pbrShader.use();
    pbrShader.setInt("irradianceMap", 0);
    pbrShader.setInt("prefilterMap", 1);
    pbrShader.setInt("brdfLUT", 2);
    pbrShader.setInt("albedoMap", 3);
    pbrShader.setInt("normalMap", 4);
    pbrShader.setInt("metallicMap", 5);
    pbrShader.setInt("roughnessMap", 6);
    pbrShader.setInt("aoMap", 7);

    backgroundShader.use();
    backgroundShader.setInt("environmentMap", 0);

    // load PBR material textures
    // --------------------------

    // gold
    unsigned int airplaneAlbedoMap = loadTexturef(FileSystem::getPath("resources/objects/kaan/kaan.png").c_str());
    unsigned int airplaneNormalMap = loadTexturef(FileSystem::getPath("resources/textures/pbr/gold/normal.png").c_str());
    unsigned int airplaneMetalicMap = loadTexturef(FileSystem::getPath("resources/textures/pbr/gold/metallic.png").c_str());
    unsigned int airplaneRoughnessMap = loadTexturef(FileSystem::getPath("resources/textures/pbr/gold/roughness.png").c_str());
    unsigned int airplaneAOMap = loadTexturef(FileSystem::getPath("resources/textures/pbr/gold/ao.png").c_str());


    // lights
    // ------
    glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3( 10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3( 10.0f, -10.0f, 10.0f),
    };
    glm::vec3 lightColors[] = {
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(FileSystem::getPath("resources/textures/hdr/newport_loft.hdr").c_str(), &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad(1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // initialize static shader uniforms before rendering
    // --------------------------------------------------
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    pbrShader.use();
    pbrShader.setMat4("projection", projection);
    backgroundShader.use();
    backgroundShader.setMat4("projection", projection);

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    #pragma endregion
   
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float currentTime = static_cast<float>(glfwGetTime()); // Mevcut zaman
        float pitchChange = pitch - lastPitch; // Pitch değişimi hesapla
        float timeSinceLastUpdate = currentTime - lastPitchUpdateTime; // Son güncellemeden geçen süre


         std::this_thread::sleep_for(std::chrono::milliseconds(8)); // Removed to improve airplane movement smoothness

        // -----
        processInput(window);


        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render scene, supplying the convoluted irradiance map to the final shader.
        // ------------------------------------------------------------------------------------------

        ourShader.use();
        // Ground model

        glm::mat4 groundModelMatrix = glm::mat4(1.0f);
        groundModelMatrix = glm::scale(groundModelMatrix, glm::vec3(groundscale, groundscale, groundscale));
                
        groundModelMatrix = glm::translate(groundModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); // Pozisyonu uygula.
        
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(360.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        groundModelMatrix = glm::rotate(groundModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

        glm::mat4 viewxz = camera.GetViewMatrix();
        glm::mat4 projectionxz = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        ourShader.setMat4("model", groundModelMatrix);
        ourShader.setMat4("view", viewxz);
        ourShader.setMat4("projection", projectionxz);
        groundModel.Draw(ourShader);

        

        pbrShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        pbrShader.setMat4("view", view);
        pbrShader.setVec3("camPos", camera.Position);

       // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
         
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, airplaneAlbedoMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, airplaneNormalMap);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, airplaneMetalicMap);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, airplaneRoughnessMap);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, airplaneAOMap);
        
        pbrShader.use();

        if (airplanePosition.y <= 1.0f) {
            airplanePosition.y = 1.0f;
            isPlaneTouchGround = true;
        } else {
            isPlaneTouchGround = false;
        }

        if ((airplanePosition.y < 1.5f && airplanePosition.y > 1.0f) && pressingS == false) {
            roll = 0.0f;
            pitch = 0.0f;
        }
        // Update camera position to follow the airplane
        float baseDistance = 10.0f; float baseHeight = 3.0f; // cameraOffset.x ve cameraOffset.y'yi orbit açılar olarak kullanıyoruz.
        float angleX = glm::radians(cameraOffset.x);
        float angleY = glm::radians(cameraOffset.y);
        float x = baseDistance * sin(angleX) * cos(angleY);
        float y = baseDistance * sin(angleY) + baseHeight;
        float z = baseDistance * cos(angleX) * cos(angleY);
        camera.Position = airplanePosition + glm::vec3(x, y, z);
        camera.Front = glm::normalize(airplanePosition - camera.Position);
        camera.Up = glm::vec3(0,1,0);


        glm::mat4 modelx = glm::mat4(1.0f);

        // Uçağın pozisyonunu uygula
        modelx = glm::translate(modelx, airplanePosition);


        // Zaman farkı 0'dan büyükse, saniyelik pitch değişimini hesapla
        pitchRate = (timeSinceLastUpdate > 0) ? (pitch - lastPitch) / timeSinceLastUpdate : 0.0f;


        std::cout << "Pitch değişim hızı: " << pitchRate << " derece/saniye" << std::endl;

        // Pitch değerlerini güncelle
        lastPitch = pitch;
        lastPitchUpdateTime = currentTime;


        // Pitch değerlerini güncelle
        lastPitch = pitch;
        lastPitchUpdateTime = currentTime;

        // Quaternion dönüşümleri ayrı ayrı uygula
        glm::quat yawQuat = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));  // Yaw (yön)
        glm::quat pitchQuat = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0)); // Pitch (burun yukarı/aşağı)
        glm::quat rollQuat = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1)); // Roll (yan yatma)


        glm::quat cobraPitchQuat = glm::angleAxis(glm::radians(10.0f), glm::vec3(1, 0, 0)); // Cobra manevrası için pitch


        glm::quat finalRotation = yawQuat * pitchQuat * rollQuat;
        glm::quat cobrafinalRotation = yawQuat * cobraPitchQuat * rollQuat;
        

        
        // Quaternion'ü model matrisine uygula
        modelx *= glm::mat4_cast(finalRotation);

     
        cobrafinalRotation = glm::normalize(cobrafinalRotation); // Bozulmaları önlemek için normalize et

        finalRotation = glm::normalize(finalRotation); // Bozulmaları önlemek için normalize et

        
        glm::vec3 forward = ((cobra) ? cobrafinalRotation : finalRotation) * glm::vec3(0.0f, 0.0f, -1.0f);


        airplanePosition += forward * (speed * deltaTime);

        
        // Ölçekleme en sonda uygulanmalı
        modelx = glm::scale(modelx, glm::vec3(airplanescale, airplanescale, airplanescale));

        // Shader'a model matrisini gönder
        pbrShader.setMat4("model", modelx);
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelx))));

        // Modeli çiz
        airplaneModel.Draw(pbrShader);






        glm::mat4 modely = glm::mat4(1.0f);
        glm::mat4 viewy = camera.GetViewMatrix();
        pbrShader.setMat4("view", viewy);
        pbrShader.setVec3("camPos", camera.Position);

        modely = glm::mat4(1.0f);
        modely = glm::translate(modely, glm::vec3(-5.0, 0.0, 0.0));

        modely = glm::rotate(modely, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); //Pitch
        modely = glm::rotate(modely, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        modely = glm::rotate(modely, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

        
        modely = glm::scale(modely, glm::vec3(30.0f, 30.0f, 30.0f));

        pbrShader.setMat4("model", modely);
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modely))));
        renderQuad(100.0f);   

        // render light source (simply re-render sphere at light positions)
        // this looks a bit off as we use the same shader, but it'll make their positions obvious and 
        // keeps the codeprint small.
        for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i)
        {
            glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
            newPos = lightPositions[i];
            pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);

            model = glm::mat4(1.0f);
            model = glm::translate(model, newPos);
            model = glm::scale(model, glm::vec3(0.5f));
            pbrShader.setMat4("model", model);
            pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            //renderSphere();
        }

        // render skybox (render as last to prevent overdraw)
        backgroundShader.use();

        backgroundShader.setMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        //glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
        //glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
        renderCube();

        // render BRDF map to screen
        //brdfShader.Use();
        //renderQuad();


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float movementSpeed = speed * deltaTime;
    float rotationSpeed = 50.0f * deltaTime;
    (cobra) ? movementSpeed *= 0.05f : movementSpeed;
    float cameraMoveSpeed = 50.0f * deltaTime; // Kamera hareket hızı

    // Hızlandırma ve yavaşlatma
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) // '+' tuşu
        speed += 10.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        speed = glm::max(speed - 10.0f * deltaTime, 0.0f);

    // Uçağı durdurma
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        speed = 0.0f;
    }

    float zroll = fmod(roll, 360.0f);
    float zpitch = fmod(pitch + 180.0f, 360.0f) - 180.0f;
    float zyaw = fmod(yaw, 360.0f);


    float pitchcalc = (rotationSpeed * (1.0f - (fabs(zroll) / 90.0f)));
    float yawcalc = (rotationSpeed * ((zroll) / 90.0f));

    int wscalc;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        wscalc = 1;//w
        pressingS = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        wscalc = -1;//s
        pressingS = true;
    }
    else {
        wscalc = 0;//none
        pressingS = false;
    }

    // Uçağın yönlendirilmesi
    if (wscalc == 1) {
        if (zroll == 0.0f) {
            zpitch -= rotationSpeed;
        } else if (zroll < 90.0f || zroll > 0.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll > -90.0f || zroll < 0.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll < 180.0f || zroll > 90.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll > -180.0f || zroll < -90.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;
        }} else if (wscalc == -1) {
        if (zroll == 0.0f) {
            zpitch += rotationSpeed;
        } else if (zroll < 90.0f || zroll > 0.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;

        } else if (zroll > -90.0f || zroll < 0.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;
        }
        else if (zroll < 180.0f || zroll > 90.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;


        } else if (zroll > -180.0f || zroll < -90.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;
        }

    }
    
    zpitch = std::clamp(zpitch, -179.0f, 179.0f);

    pitch = zpitch;
    yaw = zyaw;

    roll = zroll;

    if ((((pitch > 89.0f && pitch < 129.0f) && pitchRate >=40.0f) || glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) && cobra == false) {
        cobra = true;
        cobraStartTime = static_cast<float>(glfwGetTime());
    }
    else if ((pitch < 89.0f && pitch > 10.0f && cobra == true) || (pitch > 130.0f && cobra == true)) {
        cobra = false;
    }

    if (cobra && (glfwGetTime() - cobraStartTime >= 3.0f)) {
        cobra = false;
        std::cout << glfwGetTime() - cobraStartTime << std::endl;
    }
    
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if(isPlaneTouchGround) {
            yaw -= (rotationSpeed);
        } else {
            roll -= (rotationSpeed * 1.5f);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if(isPlaneTouchGround) {
            yaw += (rotationSpeed);
        } else {
            roll += (rotationSpeed * 1.5f);
        }
    }


    // Pozisyon güncelle
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
        if (speed > 0.0f) {
            airplanePosition.y -= (9.8f * deltaTime * 0.5f); // Yerçekimi etkisi
        }
        glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)); 
        airplanePosition += forward * movementSpeed;


    // Kamera offsetini uçak konumundan bağımsız değiştir
    glm::vec3 newOffset = cameraOffset;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        newOffset.y += cameraMoveSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        newOffset.y -= cameraMoveSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        newOffset.x -= cameraMoveSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        newOffset.x += cameraMoveSpeed;
    

    // Değişiklikleri uygula
    cameraOffset = newOffset;
}



void updateCamera()
{
    float cameraDistance = 10.0f;  // Kamera uçağın ne kadar gerisinde olacak
    float cameraHeight = 3.0f;     // Kamera yüksekliği

    // **Kameranın uçağın arkasına otomatik geçmesi için matris hesapla**
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

    // Kamera pozisyonunu hesapla (uçağın arkasında belirli bir mesafede olacak)
    glm::vec3 cameraOffset = glm::vec3(0.0f, cameraHeight, cameraDistance);
    glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(cameraOffset, 1.0f));

    // Kamera, her zaman uçağın arkasında duracak
    camera.Position = airplanePosition - rotatedOffset;

    // Kamera, uçağa bakacak şekilde hizalanmalı
    camera.Front = glm::normalize(airplanePosition - camera.Position);
    camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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

    float sensitivity = 0.1f; // Mouse duyarlılığı
    float xoffset = (xposIn - lastX) * sensitivity;
    float yoffset = (yposIn - lastY) * sensitivity; // Y ekseni ters çevrildiği için çıkartıyoruz.

    float zroll = fmod(roll, 360.0f);
    float zpitch = fmod(pitch + 180.0f, 360.0f) - 180.0f;
    float zyaw = fmod(yaw, 360.0f);

    float rotationSpeed = fabs(yoffset);

    float pitchcalc = (rotationSpeed * (1.0f - (fabs(zroll) / 90.0f)));
    float yawcalc = (rotationSpeed * ((zroll) / 90.0f));

    int wscalc;
    if (yoffset > 0.0f) {
        wscalc = 1;//w
    }
    else if (yoffset < 0.0f) {
        wscalc = -1;//s
    }
    else {
        wscalc = 0;//none
    }

    // Uçağın yönlendirilmesi
    if (wscalc == 1) {
        if (zroll == 0.0f) {
            zpitch -= rotationSpeed;
        } else if (zroll < 90.0f || zroll > 0.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll > -90.0f || zroll < 0.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll < 180.0f || zroll > 90.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;


        } else if (zroll > -180.0f || zroll < -90.0f) {
            zpitch -= pitchcalc;
            zyaw -= yawcalc;
        }} else if (wscalc == -1) {
        if (zroll == 0.0f) {
            zpitch += rotationSpeed;
        } else if (zroll < 90.0f || zroll > 0.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;

        } else if (zroll > -90.0f || zroll < 0.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;
        }
        else if (zroll < 180.0f || zroll > 90.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;


        } else if (zroll > -180.0f || zroll < -90.0f) {
            zpitch += pitchcalc;
            zyaw += yawcalc;
        }

    }

    

    pitch = zpitch;
    yaw = zyaw;
    //roll = zroll;

    lastX = xposIn;
    lastY = yposIn;

    // Mouse ile uçağın yönünü değiştir
    roll += xoffset;   // Mouse sağa/sola hareket edince uçak yön değiştirir.
    //pitch += yoffset; // Mouse yukarı/aşağı hareket edince uçak burnu yukarı/aşağı gider.

    // Pitch açılarını sınırlayarak uçağın ters dönmesini engelleyelim.


}



// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
 
// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
GLsizei indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<GLsizei>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad(float width)
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -width,  width, 0.0f, 0.0f, width,
            -width, -width, 0.0f, 0.0f, 0.0f,
             width,  width, 0.0f, width, width,
             width, -width, 0.0f, width, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexturef(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


// the end