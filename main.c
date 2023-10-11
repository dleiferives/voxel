#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _CRT_SECURE_NO_WARNINGS

char* readFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
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

int main()
{
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

    float cameraPos[3] = { 0,0,0 };
    float cameraDir[3] = { 0,0,0 };
    float cameraUp[3] = { 0,0,0 };
    float cameraRight[3] = { 0,0,0 };
    float cameraFov = 75;

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

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Set background color
        glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
        // Dispatch the compute shader
        glUseProgram(computeProgram);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraPos"), 1, cameraPos);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraDir"), 1, cameraDir);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraUp"), 1, cameraUp);
        glUniform3fv(glGetUniformLocation(computeProgram, "cameraRight"), 1, cameraRight);
        glUniform1f(glGetUniformLocation(computeProgram, "cameraFov"), cameraFov);
        glDispatchCompute(640 / 16, 480 / 16, 1);
        checkOpenGLError();
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

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
