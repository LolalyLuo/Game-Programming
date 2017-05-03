
//Shuyuan Luo
/*
 Game rules: 

 
 TO DO:
 3. Better tilemap
 4. Make levels 
 5. Able to quit the game
 
 */

#ifdef _WINDOWS
#include <GL/glew.h>
#include <SDL_mixer.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include "helper.h"
#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include <math.h>
#include <vector>
#include <time.h>
#include <string>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include "particles.h"
using namespace std;
#define PI 3.14159265
#define FIXED_TIMESTEP 0.00366666f
#define MAX_TIMESTEPS 1
#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 128
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define TILE_SIZE 0.3

#define GRAVITY -45
#define MAX_SPEED 50
#define JUMP 12
#define ACC 80
#define FRICTION 15


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif




void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Advanture!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
}






void ProcessEvents(const SDL_Event& event, bool& done){
    if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
        done = true;
    } else if(event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_START){
            start = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_GAME && player.collidedBottom == true){
            Mix_PlayChannel( -1, jumpS, 0);
            player.velocity_y = JUMP;
            jumpPE.Trigger();
            
        } else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
            endGame = true;
        }else if (event.key.keysym.scancode == SDL_SCANCODE_C && state == STATE_GAME){
            Mix_PlayChannel( -1, shootS, 0);
            shootBullet();
            
        }  else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && endGame == true){
            done = true;
        }
    }
}


void collisionEntity(){
    
    for (int i = 0; i < enemies.size(); i++){
        if (if_collision(player, enemies[i]) && enemyBuffer >0.5){
            player.life--;
            enemyBuffer = 0;
        }
    }
    for (int i = 0; i < keys.size(); i++){
        if (if_collision(player, keys[i])){
            keys[i].dead = true;
            Mix_PlayChannel( -1, coinS, 0);
            score += 1000;
        }
    }
    
}

void Update(float elapsed){
    if (state == STATE_GAME){
        player.Update(elapsed);
        collisionMap(player);
        enemyBuffer += elapsed;
        collisionEntity();
        
        for (int i = 0; i < enemies.size(); i++){
            if (enemies[i].dead == false){
                enemies[i].Update(elapsed);
                collisionMap(enemies[i]);
            }
        }
        
        for (int i =0; i < bullets.size(); i++){
            if (bullets[i].dead ==false){
                bullets[i].Update(elapsed);
                collisionMap(bullets[i]);
                for (int j = 0; j < enemies.size(); j++){
                    if (if_collision(bullets[i], enemies[j])){
                        enemies[j].dead = true;
                        bullets[i].dead = true;
                    }
                }
            }
        }
        jumpPE.Update(elapsed);
    }
    
}

void Render(ShaderProgram& program, ShaderProgram& program2){
    if (start == false ){
        state = STATE_START;
    } else if (player.life == 0 || endGame == true){
        state = STATE_GAME_OVER;
    }else{
        state = STATE_GAME;
    }
    
    switch(state){
        case STATE_START: {
            glUseProgram(program.programID);
            
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
            
            modelMatrix.identity();
            //modelMatrix.Translate(player.x, player.y, 0);
            modelMatrix.Scale(3, 3, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, startID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Press Space to Start!", 0.5f, -0.2f);
            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);
            break;}
            
            
            
        case STATE_GAME: {
            glUseProgram(program.programID);
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
            viewMatrix.identity();
            viewMatrix.Scale(1.5, 1.5, 1);
            viewMatrix.Translate(-player.x, -player.y, 0);
            if (screenShakeValue <2){
                screenShakeValue += elapsed;
                viewMatrix.Translate(0.0f, sin(screenShakeValue * screenShakeSpeed)* screenShakeIntensity,
                                 0.0f);
            }
            program.setViewMatrix(viewMatrix);
            viewMatrix2 = viewMatrix;
            //program2.setViewMatrix(viewMatrix2);
            
            modelMatrix.identity();
            program.setModelMatrix(modelMatrix);
            RenderMap (&program);

            
            for (int i = 0; i < enemies.size(); i++){
                if (enemies[i].dead == false){
                    enemies[i].Render(program);
                }
            }
            for (int i = 0; i < keys.size(); i++){
                if (keys[i].dead == false){
                    keys[i].Render(program);
                }
            }
            for (int i = 0; i < bullets.size(); i++){
                if (bullets[i].dead == false ){
                    bullets[i].Render(program);
                }
            }
            player.Render(program);
            
            modelMatrix.identity();
            modelMatrix.Translate(player.x -4, player.y+2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Life:", 0.3, -0.2);
            modelMatrix.Translate(6, 0, 0);
            program.setModelMatrix(modelMatrix);
            string finalscore = "Score: " + to_string(score);
            DrawText(&program, asc, finalscore, 0.3, -0.15);
            
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            for (int xx = 1; xx <= player.life; xx++){
                modelMatrix.identity();
                modelMatrix.Translate(player.x-3.8+xx*0.6, player.y+2, 0);
                modelMatrix.Scale(0.2, 0.2, 1);
                program.setModelMatrix(modelMatrix);
                glBindTexture(GL_TEXTURE_2D, lifeID);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);
            
            //jumpPE.Render(program2);
            glUseProgram(program.programID);
            
            
            
            break;}
            
        case STATE_GAME_OVER: {
            glUseProgram(program.programID);
            
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
            
            modelMatrix.identity();
            //modelMatrix.Translate(player.x, player.y, 0);
            modelMatrix.Scale(3, 3, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, startID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Press Space to Start!", 0.5f, -0.2f);
            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);

            break;}
            


        }
}


void Initiate(ShaderProgram program){
    
    asc  = LoadTexture(RESOURCE_FOLDER"asc.png");
    ssID  = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    pID = LoadTexture(RESOURCE_FOLDER"cute.png");
    startID = LoadTexture(RESOURCE_FOLDER"start.png");
    lifeID = LoadTexture(RESOURCE_FOLDER"life.png");
    overID = LoadTexture(RESOURCE_FOLDER"sad.png");
    
    jumpP = LoadTexture(RESOURCE_FOLDER"star.png");
    bulletID = LoadTexture(RESOURCE_FOLDER"moon.png");
    
    start = false;
    isSolid = {1, 2, 3, 4, 5, 6, 16, 17, 18, 19, 20, 32, 33, 34, 35, 36};
    counter = 0;
    screenShakeValue = 0;
    screenShakeSpeed = 30;
    screenShakeIntensity = mapValue(screenShakeValue, 0, 2, 5, 0.1);
    player.life = 3;
    score = 0;
    endGame = false;
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    jumpS = Mix_LoadWAV("jumpS.wav");
    coinS = Mix_LoadWAV("coinS.wav");
    shootS = Mix_LoadWAV("shootS.wav");
    music = Mix_LoadMUS("M.mp3");

    
    ParticleEmitter temp(2, PARTICLE_JUMP, player);
    jumpPE = temp;
    
    
    ifstream infile(RESOURCE_FOLDER"mymap.txt");
    string line;
    while (getline(infile, line)) {
        if(line == "[header]") {
            if(!readHeader(infile)) {
                return;
            }
        } else if(line == "[layer]") {
            readLayerData(infile);
        } else if(line == "[Object Layer]") {
            readEntityData(infile);
        }
    }
    
}


int main(int argc, char *argv[])
{
    //set up the window and matrix
    Setup();
    glViewport(0, 0, 1600, 900);
    

    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    ShaderProgram program2(RESOURCE_FOLDER"vertex_textured2.glsl", RESOURCE_FOLDER"fragment_textured2.glsl");
    
    
    projectionMatrix.setOrthoProjection(-6.4f, 6.4f, -3.6f, 3.6f, -1.0f, 1.0f);
    colorAttribute = glGetAttribLocation(program2.programID, "color");
    glUseProgram(program.programID);

    
    //initialize other variables
    float lastFrameTicks = 0.0f;
    SDL_Event event;
    bool done = false;
    
    
    
    Initiate(program);
    
    Mix_PlayMusic(music, -1);
    while (!done) {
   //     bl_timer_up += elapsed;
        while (SDL_PollEvent(&event)) {
            ProcessEvents(event, done);
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.44902f, 0.647059f, 0.847059f, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        program2.setModelMatrix(modelMatrix);
        program2.setProjectionMatrix(projectionMatrix);
        program2.setViewMatrix(viewMatrix);
        
        player.collidedRight = player.collidedLeft = player.collidedBottom = player.collidedTop = false;
        for (int i = 0; i < enemies.size(); i++){
            if (enemies[i].dead == false){
                enemies[i].collidedRight = enemies[i].collidedLeft = enemies[i].collidedBottom = enemies[i].collidedTop = false;
            }
        }
        for (int i = 0; i < bullets.size(); i++){
            if (bullets[i].dead == false ){
                bullets[i].collidedRight = bullets[i].collidedLeft = bullets[i].collidedBottom = bullets[i].collidedTop = false;
            }
        }
        //set up background
        float fixedElapsed = elapsed;
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
            Update(FIXED_TIMESTEP);
        }
        Update(fixedElapsed);
        Render(program, program2);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    
    Mix_FreeChunk(jumpS);
    Mix_FreeChunk(coinS);
    Mix_FreeChunk(shootS);
    Mix_FreeMusic(music);
    SDL_Quit();
    return 0;
}
