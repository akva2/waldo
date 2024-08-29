#include "bvh.hpp"
#include "ReadSTL.hpp"

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
uniform float alpha;
out float a;

void main()
{
   gl_Position = projection * view * vec4(aPos, 1.0);
   a = alpha;
}
)"s;

const std::string fShader =
R"(#version 330 core
in float a;
out vec4 FragColor;
void main()
{
   FragColor = vec4(1.0f, 1.0f, 1.0f, a);
}
)"s;

const std::string vShaderM =
R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 projection;
uniform mat4 view;

out vec3 Normal;
out vec3 FragPos;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    Normal = vec3(view * vec4(aNormal, 1.0));
    FragPos = aPos;
}
)"s;

const std::string fShaderM =
R"(#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
   // ambient
   float ambientStrength = 0.1;
   vec3 ambient = ambientStrength * lightColor;

   // diffuse
   vec3 norm = normalize(Normal);
   vec3 lightDir = normalize(lightPos - FragPos);
   float diff = max(dot(norm, lightDir), 0.0);
   vec3 diffuse = diff * lightColor;

   vec3 result = (ambient + diffuse) * objectColor;
   FragColor = vec4(result, 1.0);
}
)"s;

namespace {

auto bvh_tris_from_stl_file(std::string_view filepath, float scale)
{
    auto [tris, normals] = STLReader::read(filepath, true);
    for (auto& tri : tris) {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                tri.vertices[i][j] *= scale;
            }
        }
    }

    return std::make_tuple(tris, normals);
}
}

unsigned int compileShader(std::string_view source, int type)
{
    const char* ss = source.data();
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

unsigned int makeProgram(std::string_view vShader, std::string_view fShader)
{
    unsigned int vId = compileShader(vShader, GL_VERTEX_SHADER);
    unsigned int fId = compileShader(fShader, GL_FRAGMENT_SHADER);
    if (vId == -1 || fId == -1)
        exit(1);

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
        exit(2);
    }
    glDeleteShader(vId);
    glDeleteShader(fId);

    return spId;
}

void addAABB_lines(std::vector<float>& vertices, const BVH::Node* node,
                   int depth, int part)
{
    if (depth > 0) {
        if ((part == 0 || part == 1) && node->left)
            addAABB_lines(vertices, node->left, depth-1, 0);
        if ((part == 0 || part == 2) && node->right)
            addAABB_lines(vertices, node->right, depth-1, 0);
        return;
    }

    const auto& x1 = node->aabb.lower.x;
    const auto& y1 = node->aabb.lower.z;
    const auto& z1 = node->aabb.lower.y;
    const auto& x2 = node->aabb.upper.x;
    const auto& y2 = node->aabb.upper.z;
    const auto& z2 = node->aabb.upper.y;

    auto addLine = [&vertices](const glm::vec3& v1,
                               const glm::vec3& v2)
    {
      vertices.push_back(v1.x);
      vertices.push_back(v1.y);
      vertices.push_back(v1.z);
      vertices.push_back(v2.x);
      vertices.push_back(v2.y);
      vertices.push_back(v2.z);
    };

    addLine({x1, y1, z1}, {x2, y1, z1});
    addLine({x2, y1, z1}, {x2, y1, z2});
    addLine({x2, y1, z2}, {x1, y1, z2});
    addLine({x1, y1, z2}, {x1, y1, z1});
    addLine({x1, y1, z1}, {x1, y2, z1});
    addLine({x2, y1, z1}, {x2, y2, z1});
    addLine({x2, y1, z2}, {x2, y2, z2});
    addLine({x1, y1, z2}, {x1, y2, z2});
    addLine({x1, y2, z1}, {x2, y2, z1});
    addLine({x2, y2, z1}, {x2, y2, z2});
    addLine({x2, y2, z2}, {x1, y2, z2});
    addLine({x1, y2, z2}, {x1, y2, z1});
}

void addAABB_tri(std::vector<float>& vertices, const BVH::Node* node,
                 int depth, int part)
{
    if (depth > 0) {
        if ((part == 0 || part == 1) && node->left)
            addAABB_tri(vertices, node->left, depth-1, 0);
        if ((part == 0 || part == 2) && node->right)
            addAABB_tri(vertices, node->right, depth-1, 0);
        return;
    }

    const auto& x1 = node->aabb.lower.x;
    const auto& y1 = node->aabb.lower.z;
    const auto& z1 = node->aabb.lower.y;
    const auto& x2 = node->aabb.upper.x;
    const auto& y2 = node->aabb.upper.z;
    const auto& z2 = node->aabb.upper.y;

    auto addTri = [&vertices](const glm::vec3& v1,
                              const glm::vec3& v2,
                              const glm::vec3& v3)
    {
      vertices.push_back(v1.x);
      vertices.push_back(v1.y);
      vertices.push_back(v1.z);
      vertices.push_back(v2.x);
      vertices.push_back(v2.y);
      vertices.push_back(v2.z);
      vertices.push_back(v3.x);
      vertices.push_back(v3.y);
      vertices.push_back(v3.z);
    };

    addTri({x1, y1, z1}, {x2, y1, z1}, {x2, y2, z1});
    addTri({x1, y1, z1}, {x2, y2, z1}, {x1, y2, z1});

    addTri({x1, y1, z2}, {x2, y1, z2}, {x2, y2, z2});
    addTri({x1, y1, z2}, {x2, y2, z2}, {x1, y2, z2});

    addTri({x1, y1, z2}, {x1, y2, z2}, {x1, y2, z1});
    addTri({x1, y1, z2}, {x1, y1, z1}, {x1, y2, z1});

    addTri({x2, y1, z1}, {x2, y2, z1}, {x2, y2, z2});
    addTri({x2, y1, z1}, {x2, y1, z2}, {x2, y2, z2});

    addTri({x1, y1, z1}, {x2, y1, z1}, {x2, y1, z2});
    addTri({x1, y1, z1}, {x1, y1, z2}, {x2, y1, z2});

    addTri({x1, y2, z1}, {x2, y2, z1}, {x2, y2, z2});
    addTri({x1, y2, z1}, {x1, y2, z2}, {x2, y2, z2});
}

void addModel(std::vector<float>& vertices, const std::vector<BVH::Triangle>& tris,
              const std::vector<std::array<float,3>>& normals)
{
    vertices.reserve(tris.size() * 3 * 6);
    auto it = normals.begin();
    for (const auto& tri : tris) {
        const auto& n = *it;
        for (const auto& v : tri.vertices) {
          vertices.push_back(v.x);
          vertices.push_back(v.z);
          vertices.push_back(v.y);
          vertices.push_back(n[0]);
          vertices.push_back(n[2]);
          vertices.push_back(n[1]);
        }
        ++it;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Need one parameter, .stl file to load" << std::endl;
    }
    auto [tris, normals] = bvh_tris_from_stl_file(argv[1], 1.0);
    std::cout << "Loaded " << tris.size() << " triangles from " << argv[1] << std::endl;
    BVH::AABBTree bvh(tris, 0.001f);
    bvh.print_stats();

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_LoadLibrary(nullptr);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int spId = makeProgram(vShader, fShader);
    unsigned int spIdM = makeProgram(vShaderM, fShaderM);

    std::vector<float> verticesL;
    std::vector<float> verticesV;
    std::vector<float> verticesM;
    addAABB_lines(verticesL, bvh.root, 0, false);
    addAABB_tri(verticesV, bvh.root, 0, false);
    addModel(verticesM, tris, normals);

    unsigned int VBO[3], VAO[3];
    glGenVertexArrays(3, VAO);
    glGenBuffers(3, VBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesL.size()*sizeof(float),
                 verticesL.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, verticesV.size()*sizeof(float),
                 verticesV.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, verticesM.size()*sizeof(float),
                 verticesM.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

    glEnableVertexArrayAttrib(VAO[0], 0);
    glEnableVertexArrayAttrib(VAO[1], 0);
    glEnableVertexArrayAttrib(VAO[2], 0);
    glEnableVertexArrayAttrib(VAO[2], 1);

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
    glUseProgram(spIdM);
    glUniformMatrix4fv(glGetUniformLocation(spIdM, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(spIdM, "lightPos"), 0.f, 15.f, -1.f);
    glUniform3f(glGetUniformLocation(spIdM, "lightColor"), 0.675f, 0.512, 0.09f);
    glUniform3f(glGetUniformLocation(spIdM, "objectColor"), 1.f, 1.f, 1.f);

    SDL_Event event;
    bool is_running = true;
    double yaw = 0.f, pitch = 0.f;
    int part = 0;
    int level = 0;
    bool update_view = true;
    auto ticks = SDL_GetTicks64();
    decltype(ticks) delta = 0;
    bool tri = false;
    bool relative = true;
    bool model = true;
    while (is_running) {
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_MOUSEMOTION:
                yaw += event.motion.xrel * ROTATION_SPEED;
                pitch -= event.motion.yrel * ROTATION_SPEED;
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
                float cameraSpeed = 100 * delta / 1000.f;
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
                    part = 1;
                    update_data = true;
                }
                else if (event.key.keysym.sym == SDLK_r)
                {
                    part = 2;
                    update_data = true;
                }
                else if (event.key.keysym.sym == SDLK_b)
                {
                    part = 0;
                    update_data = true;
                }
                else if (event.key.keysym.sym == SDLK_t)
                {
                    tri = !tri;
                    update_data = true;
                }
                else if (event.key.keysym.sym == SDLK_m)
                {
                    model = !model;
                }
                // additional control inputs
                else if (event.key.keysym.sym == SDLK_g) // release mouse cursor on G
                {
                    relative = !relative;
                    SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE);
                }
                else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
                {
                    level = event.key.keysym.sym - SDLK_0;
                    update_data = true;
                }
                if (update_data)
                {
                    verticesL.clear();
                    verticesV.clear();
                    addAABB_lines(verticesL, bvh.root, level, part);
                    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
                    glBufferData(GL_ARRAY_BUFFER, verticesL.size()*sizeof(float),
                                 verticesL.data(), GL_STATIC_DRAW);
                    addAABB_tri(verticesV, bvh.root, level, part);
                    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
                    glBufferData(GL_ARRAY_BUFFER, verticesV.size()*sizeof(float),
                                 verticesV.data(), GL_STATIC_DRAW);
                }
                break;
            }
            default:
                break;
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (update_view) {
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            glUseProgram(spId);
            glUniformMatrix4fv(glGetUniformLocation(spId, "view"), 1, GL_FALSE, &view[0][0]);
            glUseProgram(spIdM);
            glUniformMatrix4fv(glGetUniformLocation(spIdM, "view"), 1, GL_FALSE, &view[0][0]);
            update_view = false;
        }

        if (model) {
            glUseProgram(spIdM);
            glBindVertexArray(VAO[2]);
            glDrawArrays(GL_TRIANGLES, 0, verticesM.size() / 6);
        }

        glUseProgram(spId);
        glBindVertexArray(VAO[0]);
        glUniform1f(glGetUniformLocation(spId, "alpha"), 1.f);
        glDrawArrays(GL_LINES, 0, verticesL.size() / 3);
        if (tri) {
            glBindVertexArray(VAO[1]);
            glUniform1f(glGetUniformLocation(spId, "alpha"), 0.5f);
            glDrawArrays(GL_TRIANGLES, 0, verticesV.size() / 3);
        }
        SDL_GL_SwapWindow(window);
        auto ticks2 = SDL_GetTicks64();
        delta = ticks2 - ticks;
        ticks = ticks2;
    }

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);

    return 0;
}
