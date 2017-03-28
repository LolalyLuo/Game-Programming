
//Shuyuan Luo
/*
 Game rules: the left side player will use Q and A to control the paddle, and the right side player uses UP and Down keys
 Try to catch the penguin with your paddle
 If you didn't catch the penguin and lose the game, the penguin and your paddle will disappear
 Enjoy the game Pong!
 */

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <math.h>
#define PI 3.14159265

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

class Entity {
public:
    void Draw(){
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 6);

    }
    float x;
    float y;
    float rotation;
    int textureID;
    float width;
    float height;
    float speed;
    float direction_x;
    float direction_y;
};

SDL_Window* displayWindow;
Entity paddle1, paddle2, ball, sep, wallTop, wallBot;
float elapsed;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float angle;
bool leftWin, rightWin;

GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
}

void ProcessEvents(const SDL_Event& event, bool& done){
    if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
        done = true;
    }
}


bool if_collision (Entity& lhs, Entity& rhs){
    if (lhs.y - lhs.height/2 - (rhs.y + rhs.height/2)>=0) { //bottom higher than top
        return false;
    }
    if (lhs.y + lhs.height/2 - (rhs.y - rhs.height/2)<=0){ //top lower than bottom
        return false;
    }
    if (lhs.x - lhs.width/2 - (rhs.x + rhs.width/2) >=0){ //left is on the right of rigt
        return false;
    }
    if (lhs.x + lhs.width/2 - (rhs.x - rhs.width/2) <=0){ // right is on the right of left
        return false;
    }
    return true;
}

void Update(ShaderProgram& program){
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    //updating the left paddle
    if(keys[SDL_SCANCODE_Q]) {
        paddle1.y += elapsed * paddle1.speed;
    } else if (keys[SDL_SCANCODE_A]) {
        paddle1.y -= elapsed * paddle1.speed;
    }
    if (rightWin == false ){
        modelMatrix.identity();
        modelMatrix.Translate(paddle1.x, paddle1.y, 0);
        modelMatrix.Scale(0.12, 0.12, 1);
        program.setModelMatrix(modelMatrix);
        paddle1.Draw();
    }
    
    //updating the right paddle
    if(keys[SDL_SCANCODE_UP]) {
        paddle2.y += elapsed * paddle2.speed;
    } else if (keys[SDL_SCANCODE_DOWN]) {
        paddle2.y -= elapsed * paddle2.speed;
    }
    if (leftWin == false){
        modelMatrix.identity();
        modelMatrix.Translate(paddle2.x, paddle2.y, 0);
        modelMatrix.Scale(0.12, 0.12, 1);
        program.setModelMatrix(modelMatrix);
        paddle2.Draw();
    }
    
    //updating the ball
    if (if_collision(ball, wallTop) || if_collision(ball, wallBot)){
        ball.direction_y *= -1;
    }
    if (if_collision(ball, paddle1) || if_collision(ball, paddle2)){
        ball.direction_x *= -1;
    }
    ball.x += ball.direction_x * elapsed * ball.speed;
    ball.y += ball.direction_y * elapsed * ball.speed;
    modelMatrix.identity();
    modelMatrix.Translate(ball.x, ball.y, 0);
    modelMatrix.Scale(0.1, 0.1, 1);
    program.setModelMatrix(modelMatrix);
    ball.Draw();
    
    if (ball.x < -3.6){
        rightWin = true;
    }
    if (ball.x > 3.6){
        leftWin = true;
    }
}

void Background(ShaderProgram& program){
    //set up seperator
    modelMatrix.identity();
    modelMatrix.Scale(0.3, 1, 1);
    program.setModelMatrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, sep.textureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    //set up the top and bottom wall
    wallBot.width = wallTop.width = 13;
    wallTop.height = wallBot.height = 0.07;
    wallTop.x = wallBot.x = 0;
    
    wallBot.y = -1.9;
    wallTop.y = 1.9;
    modelMatrix.identity();
    modelMatrix.Translate(wallTop.x, wallTop.y, 0);
    modelMatrix.Scale(3.5, 0.02, 1);
    
    
    program.setModelMatrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, wallTop.textureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    
    modelMatrix.identity();
    modelMatrix.Translate(wallBot.x, wallBot.y, 0);
    modelMatrix.Scale(3.5, 0.02, 1);
    program.setModelMatrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, wallBot.textureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main(int argc, char *argv[])
{
    //set up the window and matrix
    Setup();
    glViewport(0, 0, 640, 360);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    glUseProgram(program.programID);

    
    //set up needed entity
    GLuint stormID  = LoadTexture(RESOURCE_FOLDER"pinkstar.png");
    GLuint paddle1ID  = LoadTexture(RESOURCE_FOLDER"paddle.png");
    GLuint paddle2ID  = LoadTexture(RESOURCE_FOLDER"paddle.png");
    GLuint ballID  = LoadTexture(RESOURCE_FOLDER"ball.png");
    GLuint sepID  = LoadTexture(RESOURCE_FOLDER"seperator.png");
    wallTop.textureID = wallBot.textureID = LoadTexture(RESOURCE_FOLDER"wall.png");
    sep.textureID = sepID;
    paddle1.textureID = paddle1ID;
    paddle2.textureID = paddle2ID;
    ball.textureID = ballID;

    //initialize other variables
    float lastFrameTicks = 0.0f;
    
    SDL_Event event;
    bool done = false;
    
    leftWin = rightWin = false;
    //set up basic info about each object
    paddle1.x = -3.36;
    paddle2.x = 3.36;
    paddle1.width = paddle2.width = 0.1;
    paddle1.height = paddle2.height = 0.5;
    paddle1.speed = paddle2.speed = 1;
    
    ball.x = 0;
    ball.y = 0;
    ball.width = ball.height = 0.15;
    ball.speed = 1.5;
    angle = 45.0f*(PI/180.0f);
    ball.direction_x = cos(angle);
    ball.direction_y = sin(angle);
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            ProcessEvents(event, done);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.84902f, 0.847059f, 0.947059f, 1.0f);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        // code to set up the background
        modelMatrix.identity();
        program.setModelMatrix(modelMatrix);
        
        //set up background
        glBindTexture(GL_TEXTURE_2D, stormID);
        float vertices[] = {-3.5, -3.5, 3.5, -3.5, 3.5, 3.5, -3.5, -3.5, 3.5, 3.5, -3.5, 3.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        
        Background(program);
        
        // calculate elapsed
        float ticks = (float)SDL_GetTicks()/1000.0f;
        elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        Update(program);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_Quit();
    return 0;
}
