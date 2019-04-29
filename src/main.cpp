#include "cgmath.h"  // slee's simple math library
#include "cgut.h"    // slee's OpenGL utility
using namespace std;

// Global constants
static const char* WindowName = "Planet in Space";
static const char* VertexShaderPath = "../bin/shaders/planet.vert";
static const char* FragmentShaderPath = "../bin/shaders/planet.frag";
static const char* MeshVertexPath = "../bin/mesh/dragon.vertex.bin";
static const char* MeshIndexPath = "../bin/mesh/dragon.index.bin";

// Common structures
struct camera {
    vec3 eye = vec3(0, 30, 300);
    vec3 at = vec3(0, 0, 0);
    vec3 up = vec3(0, 1, 0);
    mat4 view_matrix = mat4::look_at(eye, at, up);

    float fovy = PI / 4.0f; // Must be in radian
    float aspect_ratio;
    float dNear = 1.0f;
    float dFar = 1000.0f;
    mat4 projection_matrix;
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

// Scene objects
mesh* pMesh = nullptr;
camera	Camera;

// Holder of vertices and indices
vector<vertex> VertexList; // Host-side vertices
vector<uint> IndexList;    // Host-side indices

void Update() {
    // Update projection matrix
    Camera.aspect_ratio = WindowSize.x / float(WindowSize.y);
    Camera.projection_matrix = mat4::perspective(Camera.fovy, Camera.aspect_ratio, Camera.dNear, Camera.dFar);

    // Update uniform variables in vertex/fragment shaders
    GLint uloc;
    if((uloc = glGetUniformLocation(ProgramID, "view_matrix")) > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, Camera.view_matrix);
    if((uloc = glGetUniformLocation(ProgramID, "projection_matrix")) > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, Camera.projection_matrix);
}

void Render() {
    // Clear screen (with background color) and clear depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Notify GL that we use our own program and buffers
    glUseProgram(ProgramID);
    if(pMesh&&pMesh->vertex_buffer) glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertex_buffer);
    if(pMesh&&pMesh->index_buffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->index_buffer);
    //if(VertexBufferID) glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
    //if(IndexBufferID) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferID);

    // Bind vertex attributes to your shader program
    const char*	vertex_attrib[] = { "position", "normal", "texcoord" };
    size_t attrib_size[] = { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
    for (size_t k = 0, kn = extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1]) {
        GLuint loc;
        if ((loc = glGetAttribLocation(ProgramID, vertex_attrib[k])) >= kn) continue;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
    }

    // Render vertices: trigger shader programs to process vertex data
    // Configure transformation parameters
    float t = float(glfwGetTime());
    float theta	= t * -0.5f*0.5f;
    float move	= -0.5f*200.0f*0.5f;

    // build the model matrix
    mat4 model_matrix = mat4::translate(move, 0.0f, -abs(move)) *
                        mat4::translate(Camera.at) *
                        mat4::rotate(vec3(0,1,0), theta) *
                        mat4::translate(-Camera.at);

    // update the uniform model matrix and render
    glUniformMatrix4fv(glGetUniformLocation(ProgramID, "model_matrix"), 1, GL_TRUE, model_matrix);
    glDrawElements(GL_TRIANGLES, pMesh->index_list.size(), GL_UNSIGNED_INT, nullptr );

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
    //printf("- Press 'd' to toggle between solid color and texture coordinates\n");
    //printf("- Press 'w' to toggle wireframe\n");
    printf("\n");
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        } else if(key == GLFW_KEY_H || key == GLFW_KEY_F1) {
            PrintHelp();
        }
    }
}

bool UserInitialize() {
    // Log hotkeys
    PrintHelp();

    // Initialize GL states
    glLineWidth(1.0f);
    glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // load the mesh
    pMesh = cg_load_mesh(MeshVertexPath, MeshIndexPath);
    if(pMesh == nullptr) {
        printf("Unable to load mesh\n");
        return false;
    }

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