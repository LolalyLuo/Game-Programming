
//Shuyuan Luo
/*
 Game rules: 
 1. press space to start the game
 2. press left and right to move and press space to jump (can only jump from the floor)
 3. touch enemy and keys to kill them / collect them
 
 Question:
 1. how to only jump from the floor
 2. how to stop slowly when not pressing anything
 3. why some times it fall into the tile a little bit then came out? or even stuck :(
 
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
#include <vector>
#include <time.h>
#include <string>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
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

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

enum GameState { STATE_START, STATE_GAME, STATE_GAME_OVER, STATE_WIN};

enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY,
    ENTITY_KEY};
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
        friction =  FRICTION;
        if (type == "good"){
            entityType = ENTITY_KEY;
            index = 86;
        } else if (type == "bad"){
            entityType = ENTITY_ENEMY;
            index = 80;
        } else {
            entityType = ENTITY_PLAYER;
            index = 98;
        }
    }
    
    
    void Update(float elapsed);
    void Render(ShaderProgram *program);
    bool collidesWith(Entity *entity);
    //SheetSprite sprite;
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
    
    EntityType entityType;
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
};


SDL_Window* displayWindow;
float elapsed;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float angle;
int life;
int counter;
bool start;
int state;
GLuint asc;
GLuint ssID;
int mapWidth, mapHeight;
unsigned char **levelData;
vector<Entity> keys;
vector<Entity> enemies;
vector<unsigned char> isSolid;
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

void Setup() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Advanture!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
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


void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY) {
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
    
    glBindTexture(GL_TEXTURE_2D, ssID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void DrawText(ShaderProgram *program, std::string text, float size, float spacing) {
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
    glBindTexture(GL_TEXTURE_2D, asc);
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

void ProcessEvents(const SDL_Event& event, bool& done){
    if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
        done = true;
    } else if(event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_START){
            start = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && state == STATE_GAME && player.collidedBottom == true){
            player.velocity_y = JUMP;
        }
        
    }
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(-worldY / TILE_SIZE);
}
void collisionMap(){
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

void collisionEntity(){
    for (int i = 0; i < enemies.size(); i++){
        if (if_collision(player, enemies[i])){
            enemies[i].dead = true;
        }
    }
    for (int i = 0; i < keys.size(); i++){
        if (if_collision(player, keys[i])){
            keys[i].dead = true;
        }
    }

}
void Update(float elapsed, ShaderProgram& program){
    if (state == STATE_GAME){
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        
        player.acceleration_x = 0.0f;
        if(keys[SDL_SCANCODE_LEFT]) {
            player.acceleration_x = -ACC;
        } else if (keys[SDL_SCANCODE_RIGHT]) {
            player.acceleration_x = ACC;
        } else {
        
        }
    
        
        player.velocity_x = lerp(player.velocity_x, 0.0f, elapsed * player.friction);
        //velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
        
        if (player.velocity_x <= MAX_SPEED || player.velocity_x >= -MAX_SPEED){
            player.velocity_x += player.acceleration_x * elapsed;
        }
        player.velocity_y += player.acceleration_y * elapsed;
        player.velocity_y += player.gravity * elapsed;
        player.x += player.velocity_x * elapsed;
        player.y += player.velocity_y * elapsed;
    
        collisionMap();
        
        collisionEntity();
    }
    
}

void Render(ShaderProgram& program){
    if (start == false ){
        state = STATE_START;
    } else{
        state = STATE_GAME;
    }
    
    switch(state){
        case STATE_START: {
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            
            modelMatrix.identity();
            //modelMatrix.Translate(player.x, player.y, 0);
            modelMatrix.Scale(2, 2, 1);
            program.setModelMatrix(modelMatrix);
            DrawSpriteSheetSprite(&program, player.index, 16, 8);
            
            modelMatrix.identity();
            modelMatrix.Translate(-3, -2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, "Press Space to Start!", 0.5f, -0.2f);
            break;}
            
            
            
        case STATE_GAME: {
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            
            viewMatrix.identity();
            viewMatrix.Translate(-player.x, -player.y, 0);
            program.setViewMatrix(viewMatrix);

            modelMatrix.identity();
            program.setModelMatrix(modelMatrix);
            RenderMap (&program);
            
            for (int i = 0; i < enemies.size(); i++){
                if (enemies[i].dead == false){
                    modelMatrix.identity();
                    modelMatrix.Translate(enemies[i].x, enemies[i].y, 0);
                    modelMatrix.Scale(TILE_SIZE, TILE_SIZE, 1);
                    program.setModelMatrix(modelMatrix);
                    DrawSpriteSheetSprite(&program, enemies[i].index, 16, 8);
                }
            }
            for (int i = 0; i < keys.size(); i++){
                if (keys[i].dead == false){
                    modelMatrix.identity();
                    modelMatrix.Translate(keys[i].x, keys[i].y, 0);
                    modelMatrix.Scale(TILE_SIZE, TILE_SIZE, 1);
                    program.setModelMatrix(modelMatrix);
                    DrawSpriteSheetSprite(&program, keys[i].index, 16, 8);
                }
            }
            
            modelMatrix.identity();
            modelMatrix.Translate(player.x, player.y, 0);
            modelMatrix.Scale(TILE_SIZE, TILE_SIZE, 1);
            program.setModelMatrix(modelMatrix);
            DrawSpriteSheetSprite(&program, player.index, 16, 8);
            
            break;}

        }
}


void Initiate(ShaderProgram program){
    glEnableVertexAttribArray(program.positionAttribute);
    glEnableVertexAttribArray(program.texCoordAttribute);
    asc  = LoadTexture(RESOURCE_FOLDER"asc.png");
    ssID  = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    start = false;
    isSolid = {1, 2, 3, 4, 5, 6, 16, 17, 18, 19, 20, 32, 33, 34, 35, 36};
    
    ifstream infile("/Users/lollipop/NYU Spring 2017/Game Design/Game-Programming/Platform Game 4/NYUCodebase/mymap.txt");
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
    
    RenderMap (&program);
    
}


int main(int argc, char *argv[])
{
    //set up the window and matrix
    Setup();
    glViewport(0, 0, 1600, 900);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    projectionMatrix.setOrthoProjection(-6.4f, 6.4f, -3.6f, 3.6f, -1.0f, 1.0f);
    glUseProgram(program.programID);

/*
    levelData = new unsigned char*[LEVEL_HEIGHT];
    for(int i = 0; i < LEVEL_HEIGHT; ++i) {
        levelData[i] = new unsigned char[LEVEL_WIDTH];
    }
*/

    
    //initialize other variables
    float lastFrameTicks = 0.0f;
    SDL_Event event;
    bool done = false;
    
    
    
    Initiate(program);

    
    while (!done) {
   //     bl_timer_up += elapsed;
        while (SDL_PollEvent(&event)) {
            ProcessEvents(event, done);
        }
    
        float ticks = (float)SDL_GetTicks()/1000.0f;
        elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        player.collidedRight = player.collidedLeft = player.collidedBottom = player.collidedTop = false;

        //set up background
        float fixedElapsed = elapsed;
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
            Update(FIXED_TIMESTEP, program);
        }
        Update(fixedElapsed, program);
        Render(program);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
    SDL_Quit();
    return 0;
}
