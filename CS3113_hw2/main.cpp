/**
* Author: Rachel Chen
* Assignment: Lunar Lander
* Date due: 2024-09-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 2
#define ACC_OF_GRAVITY -9.81f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* platforms2;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.2f,
            BG_BLUE = 0.2f,
            BG_GREEN = 0.2f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "/Users/rachelchen/Desktop/CS3113Hw3/ship.png",
            PLATFORM_FILEPATH[]     = "/Users/rachelchen/Desktop/CS3113Hw3/land.png",
            PLATFORM_FILEPATH2[]     = "/Users/rachelchen/Desktop/CS3113Hw3/lava.png",
            STARTGAME_FILEPATH[] = "/Users/rachelchen/Desktop/CS3113Hw3/welcome.png",
            ENDGAME_FAIL_FILEPATH[] = "/Users/rachelchen/Desktop/CS3113Hw3/fail.png",
            ENDGAME_SUCCESS_FILEPATH[] = "/Users/rachelchen/Desktop/CS3113Hw3/success.png";

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;  // this value MUST be zero

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool pressing_left = false;
bool pressing_right = false;
bool pressing_up = false;
bool endgame = false;
bool collide_safe = false;
bool collide_danger = false;
bool collide_upper_wall = false,
     collide_lower_wall = false,
     collide_left_wall = false,
     collide_right_wall = false;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;
float acceleration_to_right = 0.0f;
float acceleration_to_up = 0.0f;
float fuel = 30.0f;

glm::mat4 startgame_UI_model_matrix,
          endgame_UI_model_matrix;

GLuint g_start_texture_id,
       g_success_texture_id,
       g_fail_texture_id;

glm::vec3 startgame_UI_scale = glm::vec3(5.0f, 0.714f,0.0f);
glm::vec3 endgame_UI_scale = glm::vec3(5.87f, 0.5f,0.0f);

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Entities!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— PLAYER ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(0.0f, 3.5f,0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(acceleration_to_right, ACC_OF_GRAVITY*0.005 + acceleration_to_up, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

    // Walking
    g_game_state.player->m_walking[g_game_state.player->LEFT]   = new int[4] { 1, 5, 9,  13 };
    g_game_state.player->m_walking[g_game_state.player->RIGHT]  = new int[4] { 3, 7, 11, 15 };
    g_game_state.player->m_walking[g_game_state.player->UP]     = new int[4] { 2, 6, 10, 14 };
    g_game_state.player->m_walking[g_game_state.player->DOWN]   = new int[4] { 0, 4, 8,  12 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];  // start George looking right
    g_game_state.player->m_animation_frames  = 4;
    g_game_state.player->m_animation_index   = 0;
    g_game_state.player->m_animation_time    = 0.0f;
    g_game_state.player->m_animation_cols    = 4;
    g_game_state.player->m_animation_rows    = 4;
    g_game_state.player->set_height(0.9f);
    g_game_state.player->set_width(0.9f);

    // Jumping
    g_game_state.player->m_jumping_power = 3.0f;

    // ————— PLATFORM ————— //
    g_game_state.platforms = new Entity[4];
    g_game_state.platforms2 = new Entity[PLATFORM_COUNT];

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_game_state.platforms[i].m_texture_id = load_texture(PLATFORM_FILEPATH);
        g_game_state.platforms[i].set_position(glm::vec3(i - 2.0f, -3.0f, 0.0f));
        g_game_state.platforms[i].update(0.0f, NULL, 0);
    }
    
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_game_state.platforms2[i].m_texture_id = load_texture(PLATFORM_FILEPATH2);
        g_game_state.platforms2[i].set_position(glm::vec3(i + 1.0f, -3.0f, 0.0f));
        g_game_state.platforms2[i].update(0.0f, NULL, 0);
    }

    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //UI messages
    startgame_UI_model_matrix = glm::mat4(1.0f);
    startgame_UI_model_matrix = glm::translate(startgame_UI_model_matrix, glm::vec3(0.0f, -3.5f, 0.0f));
    startgame_UI_model_matrix = glm::scale(startgame_UI_model_matrix, startgame_UI_scale);
    
    endgame_UI_model_matrix = glm::mat4(1.0f);
    endgame_UI_model_matrix = glm::translate(endgame_UI_model_matrix, glm::vec3(0.0f, 3.5f, 0.0f));
    
    g_start_texture_id = load_texture(STARTGAME_FILEPATH);
    g_success_texture_id = load_texture(ENDGAME_FAIL_FILEPATH);
    g_fail_texture_id = load_texture(ENDGAME_SUCCESS_FILEPATH);
    
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom) g_game_state.player->m_is_jumping = true;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
        pressing_left = true;
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
        pressing_right = true;
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_game_state.player->move_up();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->UP];
        pressing_up = true;
    }
    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    if (!endgame){
        if (-2.05f == g_game_state.player->get_position().y) {
            collide_danger = true;
            endgame = true;}
        if (g_game_state.player->check_collision(g_game_state.platforms)) {
            collide_safe = true;
            endgame = true;}
        
        // ————— DELTA TIME ————— //
        float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
        float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
        g_previous_ticks = ticks;
        
        // ————— FIXED TIMESTEP ————— //
        // STEP 1: Keep track of how much time has passed since last step
        delta_time += g_time_accumulator;
        
        // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
        if (delta_time < FIXED_TIMESTEP)
        {
            g_time_accumulator = delta_time;
            return;
        }
        
        // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
        while (delta_time >= FIXED_TIMESTEP)
        {
            // Update acceleration based on time
            if (pressing_left) {
                acceleration_to_right -= 7.0f * delta_time; // Adjust acceleration to the left over time
                fuel -= 1.0f * delta_time;
            }
            else if (pressing_right) {
                acceleration_to_right += 7.0f * delta_time; // Adjust acceleration to the right over time
                fuel -= 1.0f * delta_time;
            }
            else if (pressing_up) {
                acceleration_to_up += 0.04f * delta_time;
                fuel -= 1.0f * delta_time;
            }
            if (!pressing_up){
                acceleration_to_up = 0.0f;}
            if ((g_game_state.player->get_acceleration().y > ACC_OF_GRAVITY*0.005) && (!pressing_up) )
            {g_game_state.player->set_acceleration(glm::vec3(acceleration_to_right,ACC_OF_GRAVITY*0.005, 0.0f));}
            else {
                g_game_state.player->set_acceleration(glm::vec3(acceleration_to_right,ACC_OF_GRAVITY*0.005+ acceleration_to_up, 0.0f));
            }
            
            // Notice that we're using FIXED_TIMESTEP as our delta time
            g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
            g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms2, PLATFORM_COUNT);
            delta_time -= FIXED_TIMESTEP;}
        
        if (g_game_state.player->get_position().x < -5.0f) {
            collide_left_wall = true;
            endgame = true;
        }
        else if (g_game_state.player->get_position().x > 5.0f) {
            collide_right_wall = true;
            endgame = true;
        }
        else if (g_game_state.player->get_position().y > 3.5f) {
            collide_upper_wall = true;
            endgame = true;
        }
        else if (g_game_state.player->get_position().y < -3.5f) {
            collide_lower_wall = true;
            endgame = true;
        }
        
        if (fuel <= 0 && g_game_state.player ->get_position().y > -2.0f) {endgame = true;}
        
        g_time_accumulator = delta_time;
        
        pressing_left = false;
        pressing_right = false;
        pressing_up = false;
        
        LOG("fuel left: ");
        LOG(fuel);
    }}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— PLAYER ————— //
    g_game_state.player->render(&g_shader_program);

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {g_game_state.platforms[i].render(&g_shader_program);}
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        g_game_state.platforms2[i].render(&g_shader_program);}
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
            -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    glm::mat4 scaled_endgame_UI_model_matrix = glm::scale(endgame_UI_model_matrix, endgame_UI_scale);
    
    if (!endgame) {
        draw_object(startgame_UI_model_matrix, g_start_texture_id);}
    else {
        if (collide_safe) {draw_object(scaled_endgame_UI_model_matrix, g_fail_texture_id);}
        else {draw_object(scaled_endgame_UI_model_matrix, g_success_texture_id);}
    }


    // ————— GENERAL ————— //
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
