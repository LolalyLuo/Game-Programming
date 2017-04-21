//
//  helper.h
//  NYUCodebase
//
//  Created by Lolaly Luo on 4/20/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#ifndef helper_h
#define helper_h

#include <SDL_mixer.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <math.h>
#include <vector>
#include <time.h>
#include <string>
#include <fstream>
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


enum GameState { STATE_START, STATE_GAME, STATE_GAME_OVER, STATE_WIN};

enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_KEY};


SDL_Window* displayWindow;
float elapsed;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float angle, scale_x, scale_y;
int life,score;
int counter;
bool start, endGame;
int state;
GLuint asc;
GLuint ssID, pID, startID, lifeID, overID;
Mix_Chunk *jumpS;
Mix_Chunk *coinS;
Mix_Chunk *shootS;
Mix_Music *music;
int mapWidth, mapHeight;
unsigned char **levelData;
vector<unsigned char> isSolid;
float screenShakeValue, screenShakeSpeed, screenShakeIntensity;
float enemyBuffer;

class Entity;
bool collisionMapDot(Entity &player);

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}


float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
    float retVal = dstMin + ((value - srcMin)/(srcMax-srcMin) * (dstMax-dstMin));
    if(retVal < dstMin) {
        retVal = dstMin;
    }
    if(retVal > dstMax) {
        retVal = dstMax;
    }
    return retVal;
}

void DrawSpriteSheetSprite(ShaderProgram *program, GLuint ID, int index, int spriteCountX, int spriteCountY) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    GLfloat texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
        -0.5f, 0.5f, -0.5f};
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    
    glBindTexture(GL_TEXTURE_2D, ID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

class Entity {
public:
    Entity(){};
    Entity(string type, float at_x, float at_y){
        x = at_x;
        y = at_y;
        width = height = TILE_SIZE;
        velocity_x = velocity_y = acceleration_x = acceleration_y = 0;
        isStatic = false;
        dead = false;
        collidedTop = collidedLeft = collidedBottom = collidedRight = false;
        gravity = GRAVITY;
        friction = FRICTION;
        if (type == "good"){
            entityType = ENTITY_KEY;
            index = 86;
        } else if (type == "bad"){
            acceleration_x = -ACC;
            entityType = ENTITY_ENEMY;
            index = 80;
        } else if (type == "person") {
            entityType = ENTITY_PLAYER;
            index = 8;
            life = 3;
        }
    }
    
    void Update(float elapsed){
        if (entityType == ENTITY_PLAYER){
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            acceleration_x = 0.0f;
            if(keys[SDL_SCANCODE_LEFT]) {
                acceleration_x = -ACC;
            } else if (keys[SDL_SCANCODE_RIGHT]) {
                acceleration_x = ACC;
            } else {
                
            }
            velocity_x = lerp(velocity_x, 0.0f, elapsed * friction);
            //velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
            if (velocity_x <= MAX_SPEED || velocity_x >= -MAX_SPEED){
                velocity_x += acceleration_x * elapsed;
            }
            velocity_y += acceleration_y * elapsed;
            velocity_y += gravity * elapsed;
            x += velocity_x * elapsed;
            y += velocity_y * elapsed;
            scale_y = mapValue(fabs(velocity_y), 0.0, 5.0, 1.0, 1.2);
            scale_x = mapValue(fabs(velocity_y), 5.0, 0.0, 0.8, 1.0);
            
        } else if (entityType == ENTITY_ENEMY){
            if (dead == false ){
                Entity dotLeft = Entity("",x- TILE_SIZE, y-TILE_SIZE/2);
                Entity dotRight = Entity("",x+TILE_SIZE, y-TILE_SIZE/2);
                
                velocity_x = lerp(velocity_x, 0.0f, elapsed * friction);
                if (velocity_x <= MAX_SPEED || velocity_x >= -MAX_SPEED){
                    velocity_x += acceleration_x * elapsed;
                }
                velocity_y += acceleration_y * elapsed;
                velocity_y += gravity * elapsed;
                if (collidedLeft || (!collisionMapDot(dotLeft))){
                    collidedLeft = false;
                    x += 0.001;
                    acceleration_x = ACC;
                }
                if (collidedRight|| (!collisionMapDot(dotRight))){
                    collidedRight = false;
                    x -= 0.001;
                    acceleration_x = -ACC;
                }
                x += velocity_x * elapsed;
                y += velocity_y * elapsed;
                scale_y = mapValue(fabs(velocity_y), 0.0, 5.0, 1.0, 1.2);
                scale_x = mapValue(fabs(velocity_y), 5.0, 0.0, 0.8, 1.0);
            }
        } else if (entityType == ENTITY_KEY){
            
        }
    }
    void Render(ShaderProgram program){
        glUseProgram(program.programID);
        if (entityType == ENTITY_ENEMY || entityType == ENTITY_KEY){
            if (dead == false ){
                float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
                glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
                float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
                glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
                modelMatrix.identity();
                modelMatrix.Translate(x, y, 0);
                modelMatrix.Scale(TILE_SIZE, TILE_SIZE, 1);
                program.setModelMatrix(modelMatrix);
                DrawSpriteSheetSprite(&program, ssID, index, 16, 8);
            }
        } else if (entityType == ENTITY_PLAYER){
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            int dir = 1;
            if (velocity_x < 0 ){
                dir = -1;
            }
            if (velocity_x < 0.1 && velocity_x > -0.1 ){
                index = 0;
            } else{
                if (counter > 20){
                    index = (index + 1) %8 +8;
                    if (index > 11){
                        index = 8;
                    }
                    counter = 0;
                } else {
                    counter ++;
                }
            }
            
            modelMatrix.identity();
            modelMatrix.Translate(x, y, 0);
            modelMatrix.Scale(dir*(TILE_SIZE+0.1)*scale_x, (TILE_SIZE+0.1)*scale_y, 1);
            program.setModelMatrix(modelMatrix);
            DrawSpriteSheetSprite(&program, pID, index, 4, 4);
        }
    }
    float x;
    float y;
    float width;
    float height;
    float velocity_x;
    float velocity_y;
    float acceleration_x;
    float acceleration_y;
    float gravity;
    float friction;
    bool isStatic;
    bool dead;
    int index;
    int life;
    
    EntityType entityType;
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;

};


vector<Entity> keys;
vector<Entity> enemies;
Entity player;


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

void DrawText(ShaderProgram *program, GLuint ID, std::string text, float size, float spacing) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glBindTexture(GL_TEXTURE_2D, ID);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);
}

bool readHeader(std::ifstream &stream) {
    string line;
    mapWidth = -1;
    mapHeight = -1;
    while(getline(stream, line)) {
    if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "width") {
            mapWidth = atoi(value.c_str());
        } else if(key == "height"){
            mapHeight = atoi(value.c_str());
        }
    }
    if(mapWidth == -1 || mapHeight == -1) {
        return false;
    } else { // allocate our map data
        levelData = new unsigned char*[mapHeight];
        for(int i = 0; i < mapHeight; ++i) {
            levelData[i] = new unsigned char[mapWidth];
        }
        return true;
    }
}

bool readLayerData(std::ifstream &stream) {
    string line;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "data") {
            for(int y=0; y < LEVEL_HEIGHT; y++) {
                getline(stream, line);
                istringstream lineStream(line);
                string tile;
                for(int x=0; x < LEVEL_WIDTH; x++) {
                    getline(lineStream, tile, ',');
                    unsigned char val = (unsigned char)atoi(tile.c_str());
                    if(val > 0) {
                        // be careful, the tiles in this format are indexed from 1 not 0
                        levelData[y][x] = val-1;
                    } else {
                        levelData[y][x] = 0;
                    }
                }
            }
        }
    }
    return true;
}

void placeEntity (string type, float placeX, float placeY){
    Entity temp(type, placeX, placeY);
    if (temp.entityType == ENTITY_ENEMY) {
        enemies.push_back(temp);
    } else if (temp.entityType == ENTITY_KEY){
        keys.push_back(temp);
    } else {
        player = temp;
    }
}

bool readEntityData(std::ifstream &stream) {
    string line;
    string type;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "type") {
            type = value;
        } else if(key == "location") {
            istringstream lineStream(value);
            string xPosition, yPosition;
            getline(lineStream, xPosition, ',');
            getline(lineStream, yPosition, ',');
            float placeX = atoi(xPosition.c_str())*TILE_SIZE;
            float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
            placeEntity(type, placeX, placeY);
        }
    }
    return true;
}

void RenderMap (ShaderProgram *program) {
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int y=0; y < LEVEL_HEIGHT; y++) {
        for(int x=0; x < LEVEL_WIDTH; x++) {
            if(levelData[y][x] != 0) {
                float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
                vertexData.insert(vertexData.end(), {
                    static_cast<float>(TILE_SIZE * x), static_cast<float>(-TILE_SIZE * y),
                    static_cast<float>(TILE_SIZE * x), static_cast<float>((-TILE_SIZE * y)-TILE_SIZE),
                    static_cast<float>((TILE_SIZE * x)+TILE_SIZE), static_cast<float>((-TILE_SIZE * y)-TILE_SIZE),
                    static_cast<float>(TILE_SIZE * x), static_cast<float>(-TILE_SIZE * y),
                    static_cast<float>((TILE_SIZE * x)+TILE_SIZE), static_cast<float>((-TILE_SIZE * y)-TILE_SIZE),
                    static_cast<float>((TILE_SIZE * x)+TILE_SIZE), static_cast<float>(-TILE_SIZE * y)
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
            }
        }
    }
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glBindTexture(GL_TEXTURE_2D, ssID);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);
    
}


void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(-worldY / TILE_SIZE);
}


void collisionMap(Entity &player){
    int left, right, bot, top, xCenter, yCenter;
    worldToTileCoordinates(player.x- (player.width/2), player.y - (player.height/2), &left, &bot);
    worldToTileCoordinates(player.x+ (player.width/2), player.y + (player.height/2), &right, &top);
    worldToTileCoordinates(player.x, player.y , &xCenter, &yCenter);
    float penetration;
    for (int i = 0; i < isSolid.size(); i++){
        if(top >= 0 && top < LEVEL_HEIGHT && xCenter >= 0 && xCenter <= LEVEL_WIDTH  && isSolid[i] == levelData[top][xCenter]){
            penetration = player.y + (player.height/2) - ((-TILE_SIZE * top) - TILE_SIZE);
            player.collidedTop = true;
            player.y -= (penetration + 0.00005);
            player.velocity_y = player.acceleration_y = 0;
        } else if (bot >= 0 && bot < LEVEL_HEIGHT && xCenter >= 0 && xCenter <= LEVEL_WIDTH && isSolid[i] == levelData[bot][xCenter]){
            penetration = (-TILE_SIZE * bot) - (player.y - (player.height/2));
            player.collidedBottom =true;
            player.y += (penetration + 0.00005);
            player.velocity_y = 0;
        } else if (yCenter >= 0 && yCenter < LEVEL_HEIGHT && left >= 0 && left <= LEVEL_WIDTH && isSolid[i] == levelData[yCenter][left]){
            penetration = ((TILE_SIZE * left) + TILE_SIZE) - (player.x- (player.width/2));
            player.collidedLeft = true;
            player.x += (penetration + 0.00005);
            player.velocity_x = player.acceleration_x = 0;
        } else if (yCenter >= 0 && yCenter < LEVEL_HEIGHT && right >= 0 && right <= LEVEL_WIDTH && isSolid[i] == levelData[yCenter][right]){
            penetration = player.x+ (player.width/2) - (TILE_SIZE * right);
            player.collidedRight = true;
            player.x -= (penetration + 0.00005);
            player.velocity_x = player.acceleration_x = 0;
        } else {}
    }
}

bool collisionMapDot(Entity &player){
    int left, right, bot, top, xCenter, yCenter;
    worldToTileCoordinates(player.x- (player.width/2), player.y - (player.height/2), &left, &bot);
    worldToTileCoordinates(player.x+ (player.width/2), player.y + (player.height/2), &right, &top);
    worldToTileCoordinates(player.x, player.y , &xCenter, &yCenter);
    bool retval = false;
    for (int i = 0; i < isSolid.size(); i++){
        if(top >= 0 && top < LEVEL_HEIGHT && xCenter >= 0 && xCenter <= LEVEL_WIDTH  && isSolid[i] == levelData[top][xCenter]){
            retval = true;
        } else if (bot >= 0 && bot < LEVEL_HEIGHT && xCenter >= 0 && xCenter <= LEVEL_WIDTH && isSolid[i] == levelData[bot][xCenter]){
            retval = true;
        } else if (yCenter >= 0 && yCenter < LEVEL_HEIGHT && left >= 0 && left <= LEVEL_WIDTH && isSolid[i] == levelData[yCenter][left]){
            retval = true;
        } else if (yCenter >= 0 && yCenter < LEVEL_HEIGHT && right >= 0 && right <= LEVEL_WIDTH && isSolid[i] == levelData[yCenter][right]){
            retval = true;
        } else {}
    }
    return retval;
}

bool if_collision (Entity& lhs, Entity& rhs){
    if (lhs.dead == true || rhs.dead == true){
        return false;
    }
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

/*===========================================================================================================================*/
GLuint colorAttribute;
GLuint jumpP;


class Vector {
public:
    Vector(){};
    Vector(float tx, float ty, float tz) {
        x= tx;
        y= ty;
        z= tz;
    }
    float const length(){
        return sqrt(x*x + y*y);
    }
    float x;
    float y;
    float z;
};

class Color {
public:
    Color(){};
    Color(float tr, float tg, float tb, float ta ){
        r = tr;
        g = tg;
        b = tb;
        a = ta;
    }
    float r;
    float g;
    float b;
    float a;
};
class Particle {
public:
    Vector position;
    Vector velocity;
    float lifetime;
    float sizeDeviation;
};

float rand_FloatRange(float a, float b){
    srand (time(NULL));
    return ((b-a)*((float)rand()/RAND_MAX))+a;
}

enum ParticleType { PARTICLE_JUMP};

class ParticleEmitter {
public:
    ParticleEmitter(unsigned int particleCount, ParticleType thetype, Entity theE){
        type = thetype;
        
        if (type == PARTICLE_JUMP){
            position.x = theE.x;
            position.y = theE.y;
            velocity.x = 0;
            velocity.y = 0;
            velocityDeviation.x = 5;
            velocityDeviation.y = 5;
            startSize = 1;
            endSize = 3;
            startColor = Color(1,0,0,1);
            endColor = Color(0,0,1,1);
            maxLifetime =2;
            gravity.y = GRAVITY;
            for (int i=0; i < particleCount; i++){
                Particle temp;
                temp.position.x = position.x;
                temp.position.y = position.y;
                temp.lifetime = 3;
                sizeDeviation = 3;
                particles.push_back(temp);
            }
        }
    }
    
    ParticleEmitter(){}
    ~ParticleEmitter(){}
    
    
    void Trigger(){
        for (int i = 0; i < particles.size(); i++){
            particles[i].velocity.x = rand_FloatRange(velocity.x - velocityDeviation.x, velocity.x - velocityDeviation.x);
            particles[i].velocity.y = rand_FloatRange(velocity.y - velocityDeviation.y, velocity.y - velocityDeviation.y);
            particles[i].lifetime = rand_FloatRange(0.0f, maxLifetime);
        }
    }
    
    void Update(float elapsed){
        if (type == PARTICLE_JUMP){
            
            for (int i = 0; i < particles.size(); i++){
                particles[i].velocity.y += elapsed*gravity.y;
                particles[i].position.x += elapsed*particles[i].velocity.x;
                particles[i].position.y += elapsed*particles[i].velocity.y;
            }
        }
    }
    
    ParticleType type;
    Vector position;
    Vector gravity;
    float maxLifetime;
    Vector velocity;
    Vector velocityDeviation;
    Color startColor;
    Color endColor;
    float startSize;
    float endSize;
    float sizeDeviation;
    std::vector<Particle> particles;
    
    void Render(ShaderProgram &program ){
        glUseProgram(program.programID);
        vector<float> vertices;
        vector<float> texCoords;
        vector<float> colors;
        for(int i=0; i < particles.size(); i++) {
            //if(particles[i].lifetime < maxLifetime){
                float m = (particles[i].lifetime/maxLifetime);
                float size = 0.1;//lerp(startSize, endSize, m) + particles[i].sizeDeviation;
                
                vertices.insert(vertices.end(), {
                    particles[i].position.x - size, particles[i].position.y + size,
                    particles[i].position.x - size, particles[i].position.y - size,
                    particles[i].position.x + size, particles[i].position.y + size,
                    particles[i].position.x + size, particles[i].position.y + size,
                    particles[i].position.x - size, particles[i].position.y - size,
                    particles[i].position.x + size, particles[i].position.y - size
                });
                texCoords.insert(texCoords.end(), {
                    0.0f, 0.0f,
                    0.0f, 1.0f,
                    1.0f, 0.0f,
                    1.0f, 0.0f,
                    0.0f, 1.0f,
                    1.0f, 1.0f
                });
                for(int j=0; j < 6; j++) {
                    colors.push_back(lerp(startColor.r, endColor.r, m));
                    colors.push_back(lerp(startColor.g, endColor.g, m));
                    colors.push_back(lerp(startColor.b, endColor.b, m));
                    colors.push_back(lerp(startColor.a, endColor.a, m));
                } }
        //}
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, false, 0, colors.data());
        glEnableVertexAttribArray(colorAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        glBindTexture(GL_TEXTURE_2D, jumpP);
        
        glDrawArrays(GL_TRIANGLES, 0, vertices.size()/2);
    }
};

ParticleEmitter jumpPE;

float easeIn(float from, float to, float time) {
    float tVal = time*time*time*time*time;
    return (1.0f-tVal)*from + tVal*to;
}

float easeOut(float from, float to, float time) {
    float oneMinusT = 1.0f-time;
    float tVal =  1.0f - (oneMinusT * oneMinusT * oneMinusT *
                          oneMinusT * oneMinusT);
    return (1.0f-tVal)*from + tVal*to;
}

float easeInOut(float from, float to, float time) {
    float tVal;
    if(time > 0.5) {
        float oneMinusT = 1.0f-((0.5f-time)*-2.0f);
        tVal =  1.0f - ((oneMinusT * oneMinusT * oneMinusT * oneMinusT *
                         oneMinusT) * 0.5f);
    } else {
        time *= 2.0;
        tVal = (time*time*time*time*time)/2.0;
    }
    return (1.0f-tVal)*from + tVal*to;
}

float easeOutElastic(float from, float to, float time) {
    float p = 0.3f;
    float s = p/4.0f;
    float diff = (to - from);
    return from + diff + (diff*pow(2.0f,-10.0f*time) * sin((time-s)*(2*3.1415926)/p));
}


#endif /* helper_h */
