#include <chrono>
#include <cmath>
#include <cstdio>
#include <fmt/format.h>
#include <iostream>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#include "bvh.hpp"
#include "camera.hpp"
#include "raytrace.hpp"
#include "vec4.hpp"

#define CLAMP(x,min,max) ((x<min)?min:((x>max)?max:x))

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 640;
constexpr float ROTATION_SPEED = .001;
constexpr float MOVEMENT_SPEED = .1;


Camera cam({0, -2, 0}, {0, 0, 0});
Vector4 light(100, 100, 100);
Vector4 material(255, 245, 213, 127); // default material color
double total_time_ns = 0.0;
size_t num_frames = 0;

struct Color
{
    unsigned char a, b, g, r;
};

static void render(Color *pixels, const BVH::AABBTree &bvh, int width, int height, int render_method)
{
    float fov = cam.get_fov();
    float tan_half_fov = std::tan(fov / 2);
    Vector4 cam_pos = cam.get_pos();
    Vector4 up;
    Vector4 right;
    Vector4 forward;
    cam.calc_vectors(&up, &right, &forward);

    float aspect_ratio;
    if (width > height)
    {
        aspect_ratio = width / (float)height;
    }
    else
    {
        aspect_ratio = height / (float)width;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
#pragma omp parallel for default(none) firstprivate(aspect_ratio, width, height, cam_pos, forward, right, tan_half_fov, up) shared(bvh, pixels)
    for (int i = 0; i < width * height; i++)
    {
        int pixel_x = i % width;
        int pixel_y = i / width;
        float pixel_x_normalized = pixel_x / (float)width;
        float pixel_y_normalized = pixel_y / (float)height;

        pixel_x_normalized = 2 * pixel_x_normalized - 1;
        pixel_x_normalized *= aspect_ratio;
        pixel_y_normalized = 1 - 2 * pixel_y_normalized;

        Vector4 pixel_pos = cam_pos + forward + right * tan_half_fov * pixel_x_normalized + up * tan_half_fov * pixel_y_normalized;

        Vector4 ray_origin = cam_pos;
        Vector4 ray_direction = (pixel_pos - cam_pos).normalized3();

        float t = 0.0f;
        Vector4 pt, normal;
        if (bvh.does_intersect_ray(ray_origin, ray_direction, &t, &pt, &normal))
        {
            // Map t from [0, inf[ to [0, 1[
            // https://math.stackexchange.com/a/3200751/691043
            float t_normalized = std::atan(t) / (M_PI_2);
            
            // Method 1: Depth map render - show how far to geometry the camera is. White = close, gray = far away
            if (render_method == 1) { 
                unsigned char pixel_color = (t_normalized * t_normalized) * 255;
                pixels[pixel_x + pixel_y * width] = {255, pixel_color, pixel_color, pixel_color};

            }
            // Method 2: Normal map render - interpret normal vector as an rgb-vector. All blue = [0,0,1] is normal pointing straight up
            else if (render_method == 2) {
                pixels[pixel_x + pixel_y * width] = { 255,
                                                      (unsigned char)((normal.x + 1) * 128),
                                                      (unsigned char)((normal.y + 1) * 128),
                                                      (unsigned char)((normal.z + 1) * 128) };

            }
            // Default: Phong shading https://en.wikipedia.org/wiki/Phong_reflection_model 
            else {
                Vector4 light_vector = (light - pt).normalized3();
                Vector4 reflection_vector = (2 * (light_vector.dot3(normal)) * normal - light_vector).normalized3();
                float specular = reflection_vector.dot3(ray_direction.normalized3());
                
                Vector4 pixel_color =       0.1 * material;                                   // Ambient
                pixel_color = pixel_color + 0.8 * material * light_vector.dot3(normal);       // Diffuse
                pixel_color = pixel_color + 0.4 * pow(specular, 2) * Vector4(255,255,255,255);// Specular
                pixels[pixel_x + pixel_y * width] = { 255,
                                                      (unsigned char)(CLAMP(pixel_color[1],0,255)),
                                                      (unsigned char)(CLAMP(pixel_color[2],0,255)),
                                                      (unsigned char)(CLAMP(pixel_color[3],0,255))};
            }

        }
        else // background image: All black
        {
            pixels[pixel_x + pixel_y * width] = {255, 0, 0, 0};
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto frame_time_ns = (t2 - t1).count();
    total_time_ns += frame_time_ns;
    num_frames++;
    std::cout << "Rendering took: " << frame_time_ns / 1'000'000 << " milli seconds" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Expected arguments: mesh.[stl|tri]");
        return 1;
    }
    
    const char *filepath = argv[1];
    float scale = (argc > 2) ? std::stof(argv[2]) : 0.01;

    std::vector<BVH::Triangle> tris = load_bvh_tris_from_mesh_file(filepath, scale);
    std::cout << "Loaded " << tris.size() << " triangles from " << filepath << std::endl;
    BVH::AABBTree bvh(tris, 0.001f);
    bvh.print_stats();

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    bool is_running = true;
    int render_method = 0;
    int32_t dx, dy;
    auto *pixels = static_cast<Color *>(malloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4));

    SDL_Texture *buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            WINDOW_WIDTH, WINDOW_HEIGHT);

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/open-sans/OpenSans-Regular.ttf", 8);
    SDL_Rect msg_rect;
    msg_rect.x = 1750;
    msg_rect.y = 10;
    msg_rect.w = 150;
    msg_rect.h = 30;
    SDL_Texture* msg = nullptr;

    bool relative = true;

    while (true)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            Vector4 up;
            Vector4 right;
            Vector4 forward;
            cam.calc_vectors(&up, &right, &forward);
            bool updateCam = true;

            switch (event.type)
            {
            case SDL_QUIT:
                is_running = updateCam = false;
                break;
            case SDL_MOUSEMOTION:
                dx = event.motion.xrel;
                dy = event.motion.yrel;
                cam.rotate(dx * ROTATION_SPEED, dy * ROTATION_SPEED);
                break;
            case SDL_KEYDOWN:
                // quit application on ESC
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    is_running = updateCam = false;
                }
                // move camera with traditional WASD
                else if (event.key.keysym.sym == SDLK_w)
                {
                    cam.move(forward * MOVEMENT_SPEED);
                }
                else if (event.key.keysym.sym == SDLK_s)
                {
                    cam.move(forward * MOVEMENT_SPEED * -1);
                }
                else if (event.key.keysym.sym == SDLK_a)
                {
                    cam.move(right * MOVEMENT_SPEED * -1);
                }
                else if (event.key.keysym.sym == SDLK_d)
                {
                    cam.move(right * MOVEMENT_SPEED);
                }
                // additional control inputs
                else if (event.key.keysym.sym == SDLK_g) // release mouse cursor on G
                {
                    relative = !relative;
                    SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE);
                }
                // switch rendering methods with numbers 0-9
                else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
                {
                    render_method = event.key.keysym.sym - SDLK_0;
                }
            default:
                break;
            }

            if (updateCam) {
                if (msg)
                    SDL_DestroyTexture(msg);
                SDL_Color White{255, 255, 255};
                const auto message = fmt::format("x={:1.2f}, y={:1.2f}, z={:1.2f}, "
                                                 "p={:1.2f}, y={:1.2f}",
                                                 cam.get_pos()[0],
                                                 cam.get_pos()[1], cam.get_pos()[2],
                                                 cam.get_pitch(), cam.get_yaw());
                SDL_Surface* surfMessage =
                    TTF_RenderText_Blended_Wrapped(font, message.c_str(), White, 140);
                msg = SDL_CreateTextureFromSurface(renderer, surfMessage);
                SDL_FreeSurface(surfMessage);
            }
        }

        if (!is_running)
            break;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render(pixels, bvh, WINDOW_WIDTH, WINDOW_HEIGHT, render_method);
        SDL_UpdateTexture(buffer, nullptr, pixels, WINDOW_WIDTH * 4);
        SDL_RenderCopy(renderer, buffer, nullptr, nullptr);
        if (msg)
            SDL_RenderCopy(renderer, msg, nullptr, &msg_rect);
        SDL_RenderPresent(renderer);
    }
    if (msg)
        SDL_DestroyTexture(msg);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(pixels);

    std::cout << "Avreage milliseconds per frame = " << (total_time_ns / num_frames) / 1'000'000 << std::endl;

    return 0;
}
