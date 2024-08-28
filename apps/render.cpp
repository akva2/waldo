#include "bvh.hpp"
#include "ReadSTL.hpp"

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <string>
#include <vector>

static constexpr int WINDOW_WIDTH = 1600;
static constexpr int WINDOW_HEIGHT = 900;
constexpr float ROTATION_SPEED = .1;
constexpr float MOVEMENT_SPEED = 10;

using namespace std::string_literals;

const std::string vShader =
R"(#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
   gl_Position = projection * view * vec4(aPos, 1.0);
}
)"s;

const std::string fShader =
R"(#version 330 core
out vec4 FragColor;
void main()
{
   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
)"s;

namespace {

std::vector<BVH::Triangle>
bvh_tris_from_stl_file(std::string_view filepath, float scale)
{
    std::vector<BVH::Triangle> tris = STLReader::read(filepath, true);
    for (auto& tri : tris) {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                tri.vertices[i][j] *= scale;
            }
        }
    }

    return tris;
}
}

unsigned int compileShader(const std::string& source, int type)
{
    const char* ss = source.c_str();
    unsigned int sId = glCreateShader(type);
    glShaderSource(sId, 1, &ss, nullptr);
    glCompileShader(sId);
    int success;
    glGetShaderiv(sId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512]{};
        glGetShaderInfoLog(sId, 512, nullptr, log);
        std::cerr << "Error compiling shader: " << log << std::endl;
        return -1;
    }

    return sId;
}

void addAABB_lines(std::vector<float>& vertices, const BVH::Node* node,
                   int depth, int part)
{
    if (depth > 0) {
        if ((part == 0 || part == 1) && node->left)
            addAABB_lines(vertices, node->left, depth-1, part);
        if ((part == 0 || part == 2) && node->right)
            addAABB_lines(vertices, node->right, depth-1, part);
        return;
    }

    const auto& x1 = node->aabb.lower.x;
    const auto& y1 = node->aabb.lower.y;
    const auto& z1 = node->aabb.lower.z;
    const auto& x2 = node->aabb.upper.x;
    const auto& y2 = node->aabb.upper.y;
    const auto& z2 = node->aabb.upper.z;

    auto addLine = [&vertices](float x1, float y1, float z1,
                               float x2, float y2, float z2)
    {
      vertices.push_back(x1);
      vertices.push_back(y1);
      vertices.push_back(z1);
      vertices.push_back(x2);
      vertices.push_back(y2);
      vertices.push_back(z2);
    };

    addLine(x1,y1,z1,x2,y1,z1);
    addLine(x2,y1,z1,x2,y1,z2);
    addLine(x2,y1,z2,x1,y1,z2);
    addLine(x1,y1,z2,x1,y1,z1);
    addLine(x1,y1,z1,x1,y2,z1);
    addLine(x2,y1,z1,x2,y2,z1);
    addLine(x2,y1,z2,x2,y2,z2);
    addLine(x1,y1,z2,x1,y2,z2);
    addLine(x1,y2,z1,x2,y2,z1);
    addLine(x2,y2,z1,x2,y2,z2);
    addLine(x2,y2,z2,x1,y2,z2);
    addLine(x1,y2,z2,x1,y2,z1);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Need one parameter, .stl file to load" << std::endl;
    }
    std::vector<BVH::Triangle> tris = bvh_tris_from_stl_file(argv[1], 1.0);
    std::cout << "Loaded " << tris.size() << " triangles from " << argv[1] << std::endl;
    BVH::AABBTree bvh(tris, 0.001f);
    bvh.print_stats();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window =
        SDL_CreateWindow("Render", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         WINDOW_WIDTH, WINDOW_HEIGHT,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);


    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    glEnable(GL_DEPTH_TEST);

    unsigned int vId = compileShader(vShader, GL_VERTEX_SHADER);
    unsigned int fId = compileShader(fShader, GL_FRAGMENT_SHADER);

    if (vId == -1 || fId == 1)
        return 1;

    unsigned int spId = glCreateProgram();
    glAttachShader(spId, vId);
    glAttachShader(spId, fId);
    glLinkProgram(spId);
    int success;
    glGetProgramiv(spId, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512]{};
        glGetProgramInfoLog(spId, 512, nullptr, log);
        std::cerr << "Error linking program: " << log << std::endl;
        return 2;
    }
    glDeleteShader(vId);
    glDeleteShader(fId);

/*    std::vector<float> vertices(tris.size()*9);
    for (size_t i = 0, idx = 0; i < tris.size(); ++i)
        for (size_t j = 0; j < 3; ++j) {
            vertices[idx++] = tris[i].vertices[j][0];
            vertices[idx++] = tris[i].vertices[j][1];
            vertices[idx++] = tris[i].vertices[j][2];
        }*/
    std::vector<float> vertices;
    addAABB_lines(vertices, bvh.root, 0, false);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // camera
    glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 projection = glm::perspective(glm::radians(45.f),
                                            float(WINDOW_WIDTH) / float(WINDOW_HEIGHT),
                                            0.1f, 100000.f);

    glUseProgram(spId);
    glUniformMatrix4fv(glGetUniformLocation(spId, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    SDL_Event event;
    bool is_running = true;
    double yaw = 0.f, pitch = 0.f;
    bool right = false;
    int level = 0;
    bool update_view = true;
    auto ticks = SDL_GetTicks64();
    decltype(ticks) delta = 0;
    while (is_running) {
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_MOUSEMOTION:
                yaw += event.motion.xrel * ROTATION_SPEED;
                pitch += event.motion.yrel * ROTATION_SPEED;
                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;
                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cameraFront = glm::normalize(front);
                update_view = true;
                break;
            case SDL_QUIT:
                is_running = false;
                break;
            case SDL_KEYDOWN:
            {
                bool update_data = false;
                auto ticks2 = SDL_GetTicks64();
                float cameraSpeed = 25 * delta / 1000.f;
                // quit application on ESC
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    is_running = false;
                else if (event.key.keysym.sym == SDLK_w)
                {
                    cameraPos += cameraSpeed * cameraFront;
                    update_view = true;
                }
                else if (event.key.keysym.sym == SDLK_s)
                {
                    cameraPos -= cameraSpeed * cameraFront;
                    update_view = true;
                }
                else if (event.key.keysym.sym == SDLK_a)
                {
                    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                    update_view = true;
                }
                else if (event.key.keysym.sym == SDLK_d)
                {
                    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                    update_view = true;
                }
                else if (event.key.keysym.sym == SDLK_l)
                {
                    right = false;
                    update_data = true;
                }
                else if (event.key.keysym.sym == SDLK_r)
                {
                    right = true;
                    update_data = true;
                }
                else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
                {
                    level = event.key.keysym.sym - SDLK_0;
                    update_data = true;
                }
                if (update_data)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, VBO);
                    vertices.clear();
                    addAABB_lines(vertices, bvh.root, level, right);
                    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
                }
                break;
            }
            default:
                break;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        if (update_view) {
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            glUniformMatrix4fv(glGetUniformLocation(spId, "view"), 1, GL_FALSE, &view[0][0]);
            update_view = false;
        }
        glDrawArrays(GL_LINES, 0, vertices.size() / 3);
        SDL_GL_SwapWindow(window);
        auto ticks2 = SDL_GetTicks64();
        delta = ticks2 - ticks;
        ticks = ticks2;
    }

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);

    return 0;
}
