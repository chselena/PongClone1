/**
* Author: Selena Cheung
* Assignment: Pong Clone
* Date due: 2024-03-02, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640 * 2,
          WINDOW_HEIGHT = 400 * 2;

const float BG_RED = 0.022f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.359f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "/Users/selenacheung/Desktop/game-prog/SDLSimple/SDLSimple/shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "/Users/selenacheung/Desktop/game-prog/SDLSimple/SDLSimple/shaders/fragment_textured.glsl";

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/SDLSimple/SDLSimple/assets/student.png";
const char PLAYER2_SPRITE_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/SDLSimple/SDLSimple/assets/teacher.png";
const char HW_SPRITE_FILEPATH[] = "/Users/selenacheung/Desktop/game-prog/SDLSimple/SDLSimple/assets/hw.png";


// –––––––––––––––––––––– MOVING –––––––––––––––––––––––––– //
                                                               //
glm::vec3 g_player_orientation = glm::vec3(0.0f, 0.0f, 0.0f);  //
glm::vec3 g_player_rotation    = glm::vec3(0.0f, 0.0f, 0.0f);  //
                                                               //
float g_player_speed = 5.0f;  // move 1 unit per second
float g_hw_speed = 5.0f;
float g_previous_ticks = 0.0f;
const float MINIMUM_COLLISION_DISTANCE = 0.5f;
bool collide_counter = false;
const float MILLISECONDS_IN_SECOND = 1000.0;
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––– //

int g_frame_counter = 0;
const int MAX_FRAME = 80;

SDL_Window* g_display_window;
bool g_game_is_running = true;


ShaderProgram g_shader_program;
glm::mat4 view_matrix, g_model_matrix, g_model2_matrix, g_model3_matrix, g_projection_matrix, g_trans_matrix;


GLuint g_player_texture_id;
GLuint g_player2_texture_id;
GLuint g_hw_texture_id;

bool press_t = false;
SDL_Joystick *g_player_one_controller;
SDL_Joystick *g_player_two_controller;

//here
// overall position
glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player2_position = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 g_hw_position = glm::vec3(0.5f, 0.0f, 0.0f);

// movement tracker
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_hw_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    //find this
    // Open the first controller found. Returns null on error
    g_player_two_controller = SDL_JoystickOpen(0);
    
    g_display_window = SDL_CreateWindow("Pong Clone!",
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

    g_model_matrix = glm::mat4(1.0f);
    g_model2_matrix = glm::mat4(1.0f);
    g_model3_matrix = glm::mat4(1.0f);
    
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_player2_texture_id = load_texture(PLAYER2_SPRITE_FILEPATH);
    g_hw_texture_id = load_texture(HW_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
}

void process_input()
{
    SDL_Event event;                                                         //
        while (SDL_PollEvent(&event))                                            //
        {                                                                        //
            switch (event.type)                                                  //
            {                                                                    //
                // End game                                                      //
                case SDL_QUIT:                                                   //
                case SDL_WINDOWEVENT_CLOSE:                                      //
                    g_game_is_running = false;                                   //
                    break;                                                       //
                                                                                 //
                case SDL_KEYDOWN:                                                //
                    switch (event.key.keysym.sym)                                //
                    {
                        case SDLK_t:
                            press_t = !press_t;
                            break;
                      
                        case SDLK_q:                                             //
                            // Quit the game with a keystroke                    //
                            g_game_is_running = false;                           //
                            break;

                        default:                                                 //
                        break;
                    }                                                            //
                                                                                 //
                default:                                                         //
                    break;                                                       //
            }                                                                    //
        }                                                                        //
                                                                                 //
        // ––––––––––––––––––––––––––––––– KEY HOLD –––––––––––––––––––––––––––– //
                                                                                 //
        const Uint8 *key_state = SDL_GetKeyboardState(NULL);                     //
        if (!press_t) {
            if (key_state[SDL_SCANCODE_UP])
            {
                g_player2_movement.y = 1.0f;
            }
            else if (key_state[SDL_SCANCODE_DOWN])
            {
                g_player2_movement.y = -1.0f;
            }
        }
        if (key_state[SDL_SCANCODE_S])
        {
            g_player_movement.y = -1.0f;
        }
        else if (key_state[SDL_SCANCODE_W])
        {
            g_player_movement.y = 1.0f;
        }
                                                                                 
        // This makes sure that the player can't "cheat" their way into moving
        // faster
        if (glm::length(g_player2_movement) > 1.0f)
        {
            g_player2_movement = glm::normalize(g_player2_movement);
        }
        if (glm::length(g_player_movement) > 1.0f)
        {
            g_player_movement = glm::normalize(g_player_movement);
        }
}

bool check_collision(glm::vec3 &position_a, glm::vec3 &position_b)
{
    return sqrt(
                pow(position_b[0] - position_a[0], 2) +
                pow(position_b[1] - position_a[1], 2)
            ) < MINIMUM_COLLISION_DISTANCE;
}


void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;
    
    g_model_matrix = glm::mat4(1.0f);
    g_model2_matrix = glm::mat4(1.0f);
    
    if (!press_t){
        g_player2_position += g_player2_movement * g_player_speed * delta_time;
        
        // Limit player 2 to the right edge of the window
        if (g_player2_position.x < 4.0f) {
            g_player2_position.x = 4.0f;
        }
        if (g_player2_position.y > 3.0f) {
            g_player2_position.y = 3.0f;
        }
        else if (g_player2_position.y < -3.0f) {
            g_player2_position.y = -3.0f;
        }
        
        g_model2_matrix = glm::mat4(1.0f);
        g_model2_matrix = glm::translate(g_model2_matrix, g_player2_position);
    }
    if(press_t){
        // Automatically move player 2 up and down
        g_player2_position.y += g_player_speed * delta_time * g_player2_movement.y;
        
        // Limit player 2 to stay within the window height
        if (g_player2_position.y > 3.0f) {
            g_player2_position.y = 3.0f;
            g_player2_movement.y = -1.0f; // Reverse direction when reaching the top
        } else if (g_player2_position.y < -3.0f) {
            g_player2_position.y = -3.0f;
            g_player2_movement.y = 1.0f; // Reverse direction when reaching the bottom
        }
        g_model2_matrix = glm::translate(g_model2_matrix, g_player2_position);
    }
    
    //student move
    //if left wall, end
    if (g_hw_position.x < -3.75f) {
        g_game_is_running = false;
    }
    //if right wall, end
    else if (g_hw_position.x > 4.0f) {
        g_game_is_running = false;
    }
    else {
        g_player_position += g_player_movement * g_player_speed * delta_time;
        // Limit player 2 to the left edge of the window
        if (g_player_position.x > -3.75f) {
            g_player_position.x = -3.75f;
        }
        if (g_player_position.y > 3.0f) {
            g_player_position.y = 3.0f;
        }
        else if (g_player_position.y < -3.0f) {
            g_player_position.y = -3.0f;
        }
        g_model_matrix = glm::mat4(1.0f);
        g_model_matrix = glm::translate(g_model_matrix, g_player_position);
        
        //homework ball move
        g_hw_position += g_hw_speed * delta_time * g_hw_movement;
        
        //collision
        if (check_collision(g_player_position, g_hw_position)) {
            g_hw_position.x += MINIMUM_COLLISION_DISTANCE;
            g_hw_movement.x = 1.0f;
            collide_counter = true;
            g_hw_movement.y = 0.5f;
        } else if (check_collision(g_player2_position, g_hw_position)){
            g_hw_position.x -= MINIMUM_COLLISION_DISTANCE;
            g_hw_movement.x = -1.0f;
            g_hw_movement.y = -0.5f;
        } else if (!collide_counter) {
            g_hw_movement.x = -1.0f;
        }
        
        // Check if the ball reaches the top or bottom of the window
        if (g_hw_position.y > 3.75f || g_hw_position.y < -3.75f) {
            // Reverse the direction of the ball when it hits the top or bottom
            g_hw_movement.y *= -1.0f;
        }
        
        // Update the model matrix for the ball
        g_model3_matrix = glm::mat4(1.0f);
        g_model3_matrix = glm::translate(g_model3_matrix, g_hw_position);
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    // Vertices2
    float vertices2[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,  // triangle 1
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f   // triangle 2
    };

    //player
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices2);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_model_matrix, g_player_texture_id);

    
    //player 2
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices2);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_model2_matrix, g_player2_texture_id);

    //hw (ball)
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_model3_matrix, g_hw_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_JoystickClose(g_player_two_controller);
    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
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
