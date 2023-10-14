#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <math.h>

#define _CRT_SECURE_NO_WARNINGS

#define M_PI 3.1415962654

struct ChildNode{
    int isLeaf;
    int address;
    struct ChildNode* children[8];
};

struct RootNode{
    int x;
    int y;
    int z;
    int isLeaf;
    int address;
    struct ChildNode* children[8];
};


char* readFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        DWORD bufferSize = MAX_PATH;
        char buffer[MAX_PATH];

        if (GetCurrentDirectory(bufferSize, buffer) != 0) {
            printf("Current directory: %s\n", buffer);
        }
        else {
            // Handle error
            fprintf(stderr, "Failed to get the current directory. Error code: %d\n", GetLastError());
        }
        exit(1);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}


void checkShaderCompilation(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader Compilation Failed\n%s\n", infoLog);
    }
}

void checkProgramLinking(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Program Linking Failed\n%s\n", infoLog);
    }
}

void checkOpenGLError() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: %d\n", err);
    }
}

float lastX = 320, lastY = 240;  // assuming your window starts at 640x480
float yaw = -90.0f, pitch = 0.0f;
int firstMouse = 1;
float cameraDir[3];

float cameraUp[3];
float cameraRight[3];

void normalize(float vec[3]) {
    float length = sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
}

void mouse_callback(void* window, double xpos, double ypos) {
    static double lastX = 0.0;  // Initialize these at the start
    static double lastY = 0.0;

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Assuming pitch and yaw are initialized to 0.0
    static float pitch = 0.0f;
    static float yaw = 0.0f;

    pitch += yoffset;
    yaw += xoffset;

    // Make sure that when pitch is out of bounds, the screen doesn't get flipped
    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    // Update camera direction based on updated Euler angles
    cameraDir[0] = cos(pitch * M_PI / 180.0f) * cos(yaw * M_PI / 180.0f);
    cameraDir[1] = sin(pitch * M_PI / 180.0f);
    cameraDir[2] = cos(pitch * M_PI / 180.0f) * sin(yaw * M_PI / 180.0f);

    // Assuming you have a normalize function for a 3-element float array
    normalize(cameraDir);
}

void processInput(GLFWwindow *window, float deltaTime, float cameraSpeed, float cameraPos[3], float cameraDir[3])
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        cameraPos[0] += cameraSpeed * cameraDir[0] * deltaTime;
        cameraPos[1] += cameraSpeed * cameraDir[1] * deltaTime;
        cameraPos[2] += cameraSpeed * cameraDir[2] * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        cameraPos[0] -= cameraSpeed * cameraDir[0] * deltaTime;
        cameraPos[1] -= cameraSpeed * cameraDir[1] * deltaTime;
        cameraPos[2] -= cameraSpeed * cameraDir[2] * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        // Move left; not implemented yet
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        // Move right; not implemented yet
    }
}


int main()
{
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Initialize the GLFW library
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    float cameraPos[3] = { 1,1,0 };
    cameraDir[0] = 0.3;
    cameraDir[1] = 0.3;
    cameraDir[2] = 0.3;

    float cameraUp[3] = { 0,0,0 };
    float cameraRight[3] = { 0,0,0 };
    float worldUp[3] = { 0,1,0 };
    float cameraFov = 2;

    glfwSetCursorPosCallback(window, (GLFWcursorposfun) mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



    char* vertexShaderSource = readFile("vertex_shader.glsl");
    char* fragmentShaderSource = readFile("fragment_shader.glsl");
    char* computeShaderSource = readFile("compute_shader.glsl");

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const char**)&vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char**)&fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader);

    // Create and compile the compute shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, (const char**)&computeShaderSource, NULL);
    glCompileShader(computeShader);
    checkShaderCompilation(computeShader);

    // Create the compute program and link it
    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    checkProgramLinking(computeProgram);  // Line 76

    // Create a texture for the compute shader to write to
    GLuint texOutput;
    glGenTextures(1, &texOutput);
    glBindTexture(GL_TEXTURE_2D, texOutput);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 640, 480, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, texOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);



// Create the shader program and link it
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinking(shaderProgram);

// Define the full-screen quad vertices and texture coordinates
    float quadVertices[] = {
            // Positions   // TexCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

// Set up the VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

// Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
// TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



    // create svo block
    int data[64];
    for(int i = 0; i < 64; i++) {
        data[i] = i % 11 % 3 ;
    }

    GLuint buffer;
    GLuint binding_point = 1; // This should match the binding point in your shader.
    GLsizeiptr size = sizeof(data); // Size of your data in bytes.
    void* data_ptr = (void*) data; // Pointer to your data.

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data_ptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, buffer);
    float debugData[4]; // Assuming you want to read 4 float values


    GLuint debugDataUBO;
    glGenBuffers(1, &debugDataUBO); // Generate a buffer object
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugDataUBO); // Bind it as a shader storage buffer
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW); // Allocate storage for the SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, debugDataUBO); // Bind the SSBO to binding point 3

// Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime, 2.5f, cameraPos, cameraDir);  // 2.5f is the camera speed

        // print out the camera direction
        printf("cameraDir: %f %f %f\n", cameraDir[0], cameraDir[1], cameraDir[2]);
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Set background color
        glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
        // Dispatch the compute shader
        glUseProgram(computeProgram);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraPos"), 1, cameraPos);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraDir"), 1, cameraDir);
        glUniform3fv(glGetUniformLocation(computeProgram, "worldUp"), 1, worldUp);
        glUniform1f(glGetUniformLocation(computeProgram, "cameraFov"), cameraFov);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer); // Bind the SSBO for your compute shader
        GLuint uniformBlockIndex = glGetUniformBlockIndex(shaderProgram, "DebugDataBuffer");
        glUniformBlockBinding(shaderProgram, uniformBlockIndex, 3);
        glDispatchCompute(640 / 16, 480 / 16, 1);
        checkOpenGLError();
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        printf("Compute Shader Dispatched\n");

        glBindBuffer(GL_UNIFORM_BUFFER, debugDataUBO);
        float* debugDataPtr = (float*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(float) * 4, GL_MAP_READ_BIT);

        printf("Debug Data: %f %f %f %f\n", debugDataPtr[0], debugDataPtr[1], debugDataPtr[2], debugDataPtr[3]);
        glUnmapBuffer(GL_UNIFORM_BUFFER);

        // Render the texture to the screen
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texOutput);
        GLint loc = glGetUniformLocation(shaderProgram, "screenTexture");
        glUniform1i(loc, 0); // 0 corresponds to GL_TEXTURE0
        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkOpenGLError();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }



    glDeleteBuffers(1, &buffer);
    // Cleanup
    glDeleteTextures(1, &texOutput);
    glDeleteProgram(computeProgram);
    glDeleteShader(computeShader);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Free the loaded shader sources
    free(vertexShaderSource);
    free(fragmentShaderSource);
    free(computeShaderSource);

    // Terminate GLFW
    glfwTerminate();

    return 0;
}
