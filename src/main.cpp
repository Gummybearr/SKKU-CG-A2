#include "cgmath.h"  // slee's simple math library
#include "cgut.h"    // slee's OpenGL utility
using namespace std;

// Global constants
static const char* WindowName = "Planet in Space";
static const char* VertexShaderPath = "../bin/shaders/planet.vert";
static const char* FragmentShaderPath = "../bin/shaders/planet.frag";
static const float Radius = 1.0f;
static const uint LongitudeVertexNumber = 64;
static const uint LatitudeVertexNumber = 32;

// Common structures
struct camera {
    mat4 view_projection_matrix = {0,1,0,0,0,0,1,0,-1,0,0,1,0,0,0,1};
};

// Window objects
GLFWwindow*	Window = nullptr;
ivec2 WindowSize = ivec2(1024, 576); // Initial window size

// OpenGL objects
GLuint ProgramID = 0;      // ID holder for GPU program
GLuint VertexBufferID = 0; // ID holder for vertex buffer
GLuint IndexBufferID = 0;  // ID holder for index buffer

// Global variables
int FrameIndex = 0;
bool useWireframe = false;
uint ColorMode = 1;
bool Rotate = false;

// Scene objects
camera	Camera;

// Holder of vertices and indices
vector<vertex> VertexList; // Host-side vertices
vector<uint> IndexList;    // Host-side indices

void Update() {
    // Update uniform variables in vertex/fragment shaders
    GLint uloc;
    if((uloc = glGetUniformLocation(ProgramID, "view_projection_matrix")) > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, Camera.view_projection_matrix);
    if((uloc = glGetUniformLocation(ProgramID, "color_mode")) > -1) glUniform1ui(uloc, ColorMode);
    if((uloc = glGetUniformLocation(ProgramID, "aspect_ratio")) > -1) glUniform1f(uloc, WindowSize.x / float(WindowSize.y));
}

void Render() {
    // Clear screen (with background color) and clear depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Notify GL that we use our own program and buffers
    glUseProgram(ProgramID);
    if(VertexBufferID) glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
    if(IndexBufferID) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferID);

    // Bind vertex attributes to your shader program
    const char*	vertex_attrib[] = {"position", "normal", "texcoord"};
    size_t attrib_size[] = {sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex)};
    for (size_t k = 0, kn = extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1]) {
        GLuint loc;
        if ((loc = glGetAttribLocation(ProgramID, vertex_attrib[k])) >= kn) continue;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
    }

    // Render vertices: trigger shader programs to process vertex data
    // Configure transformation parameters
    static float last_time = 0.0f;
    static float theta = 0.0f;

    theta += Rotate ? float(glfwGetTime()) - last_time : 0.0f;
    last_time = float(glfwGetTime());

    // build the model matrix
    mat4 model_matrix = mat4::rotate(vec3(0, 0, 1), theta);

    // update the uniform model matrix and render
    glUniformMatrix4fv(glGetUniformLocation(ProgramID, "model_matrix"), 1, GL_TRUE, model_matrix);
    glDrawElements(GL_TRIANGLES, IndexList.size(), GL_UNSIGNED_INT, nullptr );

    // Swap front and back buffers, and display to screen
    glfwSwapBuffers(Window);
}

void WindowSizeCallback(GLFWwindow* window, int width, int height) {
    // Set current viewport in pixels (win_x, win_y, win_width, win_height)
    // Viewport: the window area that are affected by rendering
    WindowSize = ivec2(width, height);
    glViewport(0, 0, width, height);
}

void PrintHelp() {
    printf("\n[Help]\n");
    printf("- Press ESC or 'q' to terminate the program\n");
    printf("- Press F1 or 'h' to see help\n");
    printf("- Press 'w' to toggle wireframe\n");
    printf("- Press 'd' to toggle color (tc.xy, 0) > (tc.xxx) > (tc.yyy)\n");
    printf("- press 'r' to toggle rotation of the sphere\n");
    printf("\n");
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        } else if(key == GLFW_KEY_H || key == GLFW_KEY_F1) {
            PrintHelp();
        } else if(key == GLFW_KEY_W) {
            useWireframe = !useWireframe;
            glPolygonMode(GL_FRONT_AND_BACK, useWireframe ? GL_LINE : GL_FILL);
            printf("> Using %s mode              \r", useWireframe ? "wireframe" : "solid");
        } else if(key == GLFW_KEY_D) {
            ColorMode = (ColorMode++) % 3;
            printf("> Using %s as color          \r", ColorMode == 1 ? "(tc.xy, 0)" : ColorMode == 2 ? "(tc.xxx)" : "(tc.yyy)");
        } else if(key == GLFW_KEY_R) {
            Rotate = !Rotate;
            printf("> %s                         \r", Rotate ? "Start rotation" : "Stop rotation");
        }
    }
}

void InitializeVertexAndIndex() {
    // Clear and create new buffers
    if(VertexBufferID)
        glDeleteBuffers(1, &VertexBufferID);
    VertexBufferID = 0;
    if(IndexBufferID)
        glDeleteBuffers(1, &IndexBufferID);
    IndexBufferID = 0;

    // Create vertices
    VertexList.clear();
    for(uint theta_k = 0; theta_k <= LatitudeVertexNumber; theta_k++) {
        for(uint pi_k = 0; pi_k <= LongitudeVertexNumber; pi_k++) {
            float theta = (PI / float(LatitudeVertexNumber)) * float(theta_k);
            float pi = (2.0f * PI / float(LongitudeVertexNumber)) * float(pi_k);

            float x_norm = sin(theta) * cos(pi);
            float y_norm = sin(theta) * sin(pi);
            float z_norm = cos(theta);

            VertexList.push_back({vec3(Radius * x_norm, Radius * y_norm, Radius * z_norm),
                                  vec3(x_norm, y_norm, z_norm),
                                  vec2(pi / (2.0f * PI), 1 - theta / PI)});
        }
    }

    // Create buffers
    IndexList.clear();
    for(uint theta_k = 0; theta_k <= LatitudeVertexNumber; theta_k++) {
        for(uint pi_k = 0; pi_k < LongitudeVertexNumber; pi_k++) {
            if(theta_k > 0) {
                IndexList.push_back((theta_k - 1) * (LongitudeVertexNumber + 1) + pi_k + 1);
                IndexList.push_back((theta_k - 1) * (LongitudeVertexNumber + 1) + pi_k);
                IndexList.push_back(theta_k * (LongitudeVertexNumber + 1) + pi_k + 1);
            }
            if(theta_k < LatitudeVertexNumber) {
                IndexList.push_back(theta_k * (LongitudeVertexNumber + 1) + pi_k);
                IndexList.push_back(theta_k * (LongitudeVertexNumber + 1) + pi_k + 1);
                IndexList.push_back((theta_k - 1) * (LongitudeVertexNumber + 1) + pi_k);
            }
        }
    }

    // Generate vertex buffer
    glGenBuffers(1, &VertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * VertexList.size(), &VertexList[0], GL_STATIC_DRAW);

    // Generate index buffer
    glGenBuffers(1, &IndexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * IndexList.size(), &IndexList[0], GL_STATIC_DRAW);
}

bool UserInitialize() {
    // Log hotkeys
    PrintHelp();

    // Initialize GL states
    glLineWidth(1.0f);
    glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Make planet
    InitializeVertexAndIndex();

    return true;
}

void UserFinalize() {
}

int main(int argc, char* argv[]){
    // Initialization
    if (!glfwInit()) {
        printf("[ERROR] Failed in glfwInit()\n");
        return 1;
    }

    // Create window and initialize OpenGL extensions
    if (!(Window = cg_create_window(WindowName, WindowSize.x, WindowSize.y))) {
        glfwTerminate();
        return 1;
    }
    if (!cg_init_extensions(Window)) {
        glfwTerminate();
        return 1;
    }

    // Initializations and validations of GLSL program
    if (!(ProgramID = cg_create_program(VertexShaderPath, FragmentShaderPath))) {
        glfwTerminate();
        return 1;
    }
    if (!UserInitialize()) {
        printf("Failed to user_init()\n");
        glfwTerminate();
        return 1;
    }

    // Register event callbacks
    glfwSetWindowSizeCallback(Window, WindowSizeCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    // Enters rendering/event loop
    for (FrameIndex = 0; !glfwWindowShouldClose(Window); FrameIndex++) {
        glfwPollEvents();   // Polling and processing of events
        Update();           // Per-frame update
        Render();           // Per-frame render
    }

    // Normal termination
    UserFinalize();
    cg_destroy_window(Window);

    return 0;
}