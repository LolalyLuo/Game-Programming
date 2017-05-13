
//Shuyuan Luo
/*
Game rules:
 
Requirements:
 Must have a title screen and proper states for game over, etc. Check!
 Must have a way to quit the game. Check!
 Must have music and sound effects. Check!
 Must have at least 3 different levels or be procedurally generated. Check!
 Must be either local multiplayer or have AI (or both!). Check!
 Must have at least some animation or particle effects. Check!
 
 Extra Credit: 
 Having shader effects. Check!
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


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

void LoadLevel (){
    if (levelData != nullptr){
        for(int i = 0; i < mapHeight; ++i) {
            delete levelData[i];
        }
        delete levelData;
    }
    
    enemies.clear();
    shooters.clear();
    keys.clear();
    score = 0;
    
    ifstream infile;
    if (gameLevel == GAMELEVEL1){
        infile.open(RESOURCE_FOLDER"level1map.txt");
    } else if (gameLevel == GAMELEVEL2){
        infile.open(RESOURCE_FOLDER"level2map.txt");
    } else{
        infile.open(RESOURCE_FOLDER"level3map.txt");
        
    }
    string line;
    while (getline(infile, line)) {
        if(line == "[header]") {
            if(!readHeader(infile)) {
                exit(1);
            }
        } else if(line == "[layer]") {
            readLayerData(infile);
        } else if(line == "[Object Layer]") {
            readEntityData(infile);
        }
    }
    
}


void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Sailormoon Advanture!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
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
        if (event.key.keysym.scancode == SDL_SCANCODE_1 && state == STATE_START){
            start = true;
            gameLevel = GAMELEVEL1;
            LoadLevel();
            waterPE.dead = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_2 && state == STATE_START){
            start = true;
            gameLevel = GAMELEVEL2;
            LoadLevel();
            Entity waterfall ("water", 5, 0);
            ParticleEmitter temp6(150, PARTICLE_WATER, waterfall);
            waterPE = temp6;
            waterPE.dead = false;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_3 && state == STATE_START){
            start = true;
            gameLevel = GAMELEVEL3;
            LoadLevel();
            waterPE.dead = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_GAME && player.collidedBottom == true){
            Mix_PlayChannel( -1, jumpS, 0);
            if (jump_high ==  true){
                player.velocity_y = JUMP+8;
            } else {
                player.velocity_y = JUMP;
            }
            jumpPE.Trigger(player);
            
        } else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && endGame != true){
            endGame = true;
        }else if (event.key.keysym.scancode == SDL_SCANCODE_C && state == STATE_GAME){
            Mix_PlayChannel( -1, shootS, 0);
            shootBullet();
            
        }  else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && endGame == true){
            done = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_GAME_OVER){
            endGame = false;
            start = false;
            state = STATE_START;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_WIN){
            score = 0;
            endGame = false;
            start = false;
            state = STATE_START;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_H && state != STATE_HELP ){
            previous = state;
            state = STATE_HELP;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_HELP){
            state = previous;
        }
    }
}


void collisionEntity(){
    
    for (int i = 0; i < enemies.size(); i++){
        if (if_collision(player, enemies[i]) && enemyBuffer >0.5){
            Mix_PlayChannel( -1, painS, 0);
            shootPlayerPE.Trigger(player);
            player.life--;
            enemyBuffer = 0;
        }
    }
    for (int i = 0; i < keys.size(); i++){
        if (if_collision(player, keys[i])){
            Mix_PlayChannel( -1, coinS, 2);
            getKeyPE.Trigger(keys[i]);
            keys[i].dead = true;
            
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
        
        if (score >= 7000){
            score = 0;
            winGame = true;
            Mix_PlayChannel( -1, winS, 0);
            winBuffer += elapsed;
        }
        
        
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
                        Mix_PlayChannel( -1, explodeS, 0);
                        shootEnemyPE.Trigger(enemies[j]);
                        enemies[j].dead = true;
                        bullets[i].dead = true;
                    }
                }
                for (int j = 0; j < shooters.size(); j++){
                    if (if_collision(bullets[i], shooters[j])){
                        Mix_PlayChannel( -1, explodeS, 0);
                        shootShooterPE.Trigger(shooters[j]);
                        shooters[j].dead = true;
                        bullets[i].dead = true;
                    }
                }
                
            }
        }
        for (int i = 0; i < shooters.size(); i++){
            if (shooters[i].dead == false){
                collisionMap(shooters[i]);
                if (abs(shooters[i].x -player.x)<1.5 && abs(shooters[i].y -player.y)<1.5){
                    shooters[i].state = ES_ATTACK;
                } else {
                    shooters[i].state = ES_REST;
                }
                if ((shooters[i].x - player.x) / abs(shooters[i].x - player.x)< 0){
                    shooters[i].direction = 1;
                } else {
                    shooters[i].direction = -1;
                }
                shooters[i].Update(elapsed);
            }
        }
        
        for (int i = 0; i < bad_bullets.size(); i++){
            if (bad_bullets[i].dead ==false){
                bad_bullets[i].Update(elapsed);
                collisionMap(bad_bullets[i]);
                if (if_collision(bad_bullets[i], player)){
                    Mix_PlayChannel( -1, painS, 0);
                    shootPlayerPE.Trigger(player);
                    bad_bullets[i].dead = true;
                    player.life--;
                }
            }
        }
        spikeBuffer += elapsed;
        if (spike == true && spikeBuffer >1
            ){
            Mix_PlayChannel( -1, painS, 0);
            shootPlayerPE.Trigger(player);
            player.life--;
            spikeBuffer = 0;
        }
        
        jumpPE.Update(elapsed);
        shootPlayerPE.Update(elapsed);
        shootShooterPE.Update(elapsed);
        shootEnemyPE.Update(elapsed);
        getKeyPE.Update(elapsed);
        waterPE.Update(elapsed);
    }


    
}

void Render(ShaderProgram& program, ShaderProgram& program2){
    if (state != STATE_HELP){
        if (start == false  ){
            state = STATE_START;
        } else if (player.life == 0 || endGame == true || player.y < -15){
            state = STATE_GAME_OVER;
        } else if (winGame == true && winBuffer > 2){
            state = STATE_WIN;
        }else{
            state = STATE_GAME;
        }
    }
    
    switch(state){
        case STATE_START: {
            glUseProgram(program.programID);
            
            viewMatrix.identity();
            program.setViewMatrix(viewMatrix);
            
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4, -0.5, 0);
            modelMatrix.Scale(1.8, 3, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, frame);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            modelMatrix.Scale(1, 0.96, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, l3);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(0, -0.5, 0);
            modelMatrix.Scale(1.8, 3, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, frame);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            modelMatrix.Scale(1, 0.96, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, l1);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(4, -0.5, 0);
            modelMatrix.Scale(1.8, 3, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, frame);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            modelMatrix.Scale(1, 0.96, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, l2);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4.9, -2.5, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "LEVEL 1", 0.5f, -0.2f);
            modelMatrix.identity();
            modelMatrix.Translate(-0.8, -2.5, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "LEVEL 2", 0.5f, -0.2f);
            modelMatrix.identity();
            modelMatrix.Translate(3.3, -2.5, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "LEVEL 3", 0.5f, -0.2f);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3, 3, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Pick a Level to Start!", 0.5f, -0.2f);
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
            
            jumpPE.Render(program2);
            shootEnemyPE.Render(program2);
            shootShooterPE.Render(program2);
            shootPlayerPE.Render(program2);
            getKeyPE.Render(program2);
            waterPE.Render(program2);

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
            for (int i = 0; i < shooters.size(); i++){
                if (shooters[i].dead == false){
                    shooters[i].Render(program);
                }
            }
            for (int i = 0; i < bad_bullets.size(); i++){
                if (bad_bullets[i].dead == false){
                    bad_bullets[i].Render(program);
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
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
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
            modelMatrix.Scale(2.5, 2.5, 1);
            program.setModelMatrix(modelMatrix);
            viewMatrix.identity();
            program.setViewMatrix(viewMatrix);
            glBindTexture(GL_TEXTURE_2D, overID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3.8, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "You Can Do Better Next Time!", 0.5f, -0.2f);
            modelMatrix.identity();
            modelMatrix.Translate(-3.5, -2.8, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Press Space to Play Again!", 0.5f, -0.2f);
            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);

            break;}
            
        case STATE_WIN: {
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
            viewMatrix.identity();
            program.setViewMatrix(viewMatrix);
            glBindTexture(GL_TEXTURE_2D, startID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3.8, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "You Won! Sailor Moon Loves You!", 0.5f, -0.2f);
            modelMatrix.identity();
            modelMatrix.Translate(-3.5, -2.8, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Press Space to Play Again!", 0.5f, -0.2f);

            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);
            
            break;}
            
        case STATE_HELP: {
            glUseProgram(program.programID);
            viewMatrix.identity();
            program.setViewMatrix(viewMatrix);
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program.positionAttribute);
            glEnableVertexAttribArray(program.texCoordAttribute);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, 3, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "Press Space to Go Back to the Game", 0.5f, -0.28f);

            modelMatrix.identity();
            modelMatrix.Translate(-4.8, 2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "1. Use arrows to move.", 0.5f, -0.28f);
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, 1, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "2. Use space to jump.", 0.5f, -0.28f);
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, 0, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "3. Use C to shoot.", 0.5f, -0.28f);
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, -1, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "4. Collect 7 keys in a level to win!", 0.5f, -0.28f);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "You can jump higher if you step on:", 0.5f, -0.28f);
            modelMatrix.identity();
            modelMatrix.Translate(-4.8, -3, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, asc, "You will lose a life if you step on:", 0.5f, -0.28f);
            
            modelMatrix.identity();
            modelMatrix.Translate(3.5, -1.8, 0);
            modelMatrix.Scale(0.8, 0.8, 0);
            program.setModelMatrix(modelMatrix);
            DrawSpriteSheetSprite(&program, ssID, 84, 16, 8);
            modelMatrix.identity();
            modelMatrix.Translate(3.5, -3, 0);
            modelMatrix.Scale(0.8, 0.8, 0);
            program.setModelMatrix(modelMatrix);
            DrawSpriteSheetSprite(&program, ssID, 100, 16, 8);
            
            glDisableVertexAttribArray(program.positionAttribute);
            glDisableVertexAttribArray(program.texCoordAttribute);
            
            break;}


        }
}


void Initiate(ShaderProgram &program){
    
    program.setModelMatrix(modelMatrix);
    program.setProjectionMatrix(projectionMatrix);
    program.setViewMatrix(viewMatrix);
    
    asc  = LoadTexture(RESOURCE_FOLDER"asc.png");
    ssID  = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    pID = LoadTexture(RESOURCE_FOLDER"cute.png");
    startID = LoadTexture(RESOURCE_FOLDER"start.png");
    lifeID = LoadTexture(RESOURCE_FOLDER"life.png");
    overID = LoadTexture(RESOURCE_FOLDER"sad.png");
    
    jumpP = LoadTexture(RESOURCE_FOLDER"star.png");
    playerP = LoadTexture(RESOURCE_FOLDER"playerP.png");
    shooterP = LoadTexture(RESOURCE_FOLDER"smoke.png");
    enemyP = LoadTexture(RESOURCE_FOLDER"smoke.png");
    keyP = LoadTexture(RESOURCE_FOLDER"weapon.png");
    bulletID = LoadTexture(RESOURCE_FOLDER"moon.png");
    
    l1 = LoadTexture(RESOURCE_FOLDER"level1.png");
    l2 = LoadTexture(RESOURCE_FOLDER"level2.png");
    l3 = LoadTexture(RESOURCE_FOLDER"level3.png");
    frame = LoadTexture(RESOURCE_FOLDER"frame.png");
    waterID = LoadTexture(RESOURCE_FOLDER"water.png");
    start = false;
    isSolid = {1, 2, 3, 4, 5, 6, 16, 17, 18, 19, 20, 32, 33, 34, 35, 36,93,94, 102, 103,105, 106,107,108,109,110, 119,121,122,123,124};
    counter = 0;
    screenShakeValue = 0;
    screenShakeSpeed = 30;
    screenShakeIntensity = mapValue(screenShakeValue, 0, 2, 5, 0.1);
    player.life = 7;
    score = 0;
    endGame = false;
    winGame = false;
    spike = false;
    jump_high = false;
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    jumpS = Mix_LoadWAV("jumpS.wav");
    coinS = Mix_LoadWAV("coinS.wav");
    shootS = Mix_LoadWAV("shootS.wav");
    explodeS = Mix_LoadWAV("explode.wav");
    painS = Mix_LoadWAV("pain1.wav");
    winS = Mix_LoadWAV("clap.wav");
    
    
    music = Mix_LoadMUS("M.mp3");

    
    ParticleEmitter temp1(25, PARTICLE_JUMP, player);
    jumpPE = temp1;
    ParticleEmitter temp2(40, PARTICLE_PLAYER, player);
    shootPlayerPE = temp2;
    ParticleEmitter temp3(40, PARTICLE_ENEMY, player);
    shootEnemyPE = temp3;
    ParticleEmitter temp4(40, PARTICLE_KEY, player);
    getKeyPE = temp4;
    ParticleEmitter temp5(40, PARTICLE_SHOOTER, player);
    shootShooterPE = temp5;
    
    enemyBuffer = spikeBuffer = 2;
    winBuffer = 0;
    //gameLevel = GAMELEVEL1;

    
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
    levelData = nullptr;
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            ProcessEvents(event, done);
        }
        /*if (state != STATE_GAME){
            LoadLevel();
        }*/
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClear(GL_COLOR_BUFFER_BIT);
        if (state == STATE_GAME){
            glClearColor(0.44902f, 0.647059f, 0.847059f, 0.0f);
        } else if (state == STATE_START || state == STATE_GAME_OVER){
            glClearColor(0, 0, 0, 0.0f);
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        program2.setModelMatrix(modelMatrix);
        program2.setProjectionMatrix(projectionMatrix);
        program2.setViewMatrix(viewMatrix);
        
        spike = jump_high = false;
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
    Mix_FreeChunk(explodeS);
    Mix_FreeChunk(painS);
    Mix_FreeChunk(winS);
    Mix_FreeMusic(music);
    SDL_Quit();
    return 0;
}
