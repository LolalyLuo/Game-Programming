
//Shuyuan Luo
/*
 Game rules: 
 1. Red box and green box are colliding if the player doesn't come to the middle
 2. The player is free to move using left, right, up and down keys
 3. Player can try to collid with the boxes from all directions
 4. you can push the box around... have fun!!
 
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
#include <cmath>
using namespace std;
#define PI 3.14159265
#define FIXED_TIMESTEP 0.00366666f
#define MAX_TIMESTEPS 1

#define SPEED 25


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

enum GameState { STATE_START, STATE_GAME, STATE_GAME_OVER, STATE_WIN};

enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY};

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
    void normalize(){
        if (this->length() !=0 ){
            float len = this->length();
            x /= len;
            y /= len;
            z /= len;
        }
    }
    Vector operator * (Matrix &mat){
        Vector r;
        r.x = mat.m[0][0]*x + mat.m[1][0]*y + mat.m[2][0]*z + mat.m[3][0];
        r.y = mat.m[0][1]*x + mat.m[1][1]*y + mat.m[2][1]*z + mat.m[3][1];
        r.z = mat.m[0][2]*x + mat.m[1][2]*y + mat.m[2][2]*z + mat.m[3][2];
        
        return r;
    }
    float x;
    float y;
    float z;
};

class Entity {
public:
    Entity(){};
    
    Matrix matrix;
    Vector position;
    Vector scale;
    float rotation;
    vector<Vector> points;
    
    void Update(float elapsed);
    void Render(ShaderProgram *program);
    bool collidesWith(Entity *entity);
   
    //float width;
    //float height;
    float velocity_x;
    float velocity_y;
    //float acceleration_x;
    //float acceleration_y;
    GLuint ID;
    
    EntityType entityType;
    
};


SDL_Window* displayWindow;
float elapsed;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

GLuint grect, rrect,tria;
Entity player, grec, rrec;


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
    displayWindow = SDL_CreateWindow("Collision!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
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


bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
    float normalX = -edgeY;
    float normalY = edgeX;
    float len = sqrtf(normalX*normalX + normalY*normalY);
    normalX /= len;
    normalY /= len;
    
    std::vector<float> e1Projected;
    std::vector<float> e2Projected;
    
    for(int i=0; i < points1.size(); i++) {
        e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
    }
    for(int i=0; i < points2.size(); i++) {
        e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
    }
    
    std::sort(e1Projected.begin(), e1Projected.end());
    std::sort(e2Projected.begin(), e2Projected.end());
    
    float e1Min = e1Projected[0];
    float e1Max = e1Projected[e1Projected.size()-1];
    float e2Min = e2Projected[0];
    float e2Max = e2Projected[e2Projected.size()-1];
    
    float e1Width = fabs(e1Max-e1Min);
    float e2Width = fabs(e2Max-e2Min);
    float e1Center = e1Min + (e1Width/2.0);
    float e2Center = e2Min + (e2Width/2.0);
    float dist = fabs(e1Center-e2Center);
    float p = dist - ((e1Width+e2Width)/2.0);
    
    if(p >= 0) {
        return false;
    }
    
    float penetrationMin1 = e1Max - e2Min;
    float penetrationMin2 = e2Max - e1Min;
    
    float penetrationAmount = penetrationMin1;
    if(penetrationMin2 < penetrationAmount) {
        penetrationAmount = penetrationMin2;
    }
    
    penetration.x = normalX * penetrationAmount;
    penetration.y = normalY * penetrationAmount;
    
    return true;
}

bool penetrationSort(Vector &p1, Vector &p2) {
    return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
    std::vector<Vector> penetrations;
    for(int i=0; i < e1Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e1Points.size()-1) {
            edgeX = e1Points[0].x - e1Points[i].x;
            edgeY = e1Points[0].y - e1Points[i].y;
        } else {
            edgeX = e1Points[i+1].x - e1Points[i].x;
            edgeY = e1Points[i+1].y - e1Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    for(int i=0; i < e2Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e2Points.size()-1) {
            edgeX = e2Points[0].x - e2Points[i].x;
            edgeY = e2Points[0].y - e2Points[i].y;
        } else {
            edgeX = e2Points[i+1].x - e2Points[i].x;
            edgeY = e2Points[i+1].y - e2Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    
    std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
    penetration = penetrations[0];
    
    Vector e1Center;
    for(int i=0; i < e1Points.size(); i++) {
        e1Center.x += e1Points[i].x;
        e1Center.y += e1Points[i].y;
    }
    e1Center.x /= (float)e1Points.size();
    e1Center.y /= (float)e1Points.size();
    
    Vector e2Center;
    for(int i=0; i < e2Points.size(); i++) {
        e2Center.x += e2Points[i].x;
        e2Center.y += e2Points[i].y;
    }
    e2Center.x /= (float)e2Points.size();
    e2Center.y /= (float)e2Points.size();
    
    Vector ba;
    ba.x = e1Center.x - e2Center.x;
    ba.y = e1Center.y - e2Center.y;
    
    if( (penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
        penetration.x *= -1.0f;
        penetration.y *= -1.0f;
    }
    
    return true;
}

void Update(float elapsed, ShaderProgram& program){
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if(keys[SDL_SCANCODE_LEFT]) {
        player.velocity_x = -SPEED;
    } else if (keys[SDL_SCANCODE_RIGHT]) {
        player.velocity_x = SPEED;
    } else if (keys[SDL_SCANCODE_DOWN]){
        player.velocity_y = -SPEED;
    } else if (keys[SDL_SCANCODE_UP]){
        player.velocity_y = SPEED;
    } else{
        player.velocity_y =0;
        player.velocity_x = 0;
        
    }
    player.position.x += player.velocity_x * elapsed;
    player.position.y += player.velocity_y * elapsed;
    
    grec.position.x += grec.velocity_x * elapsed;
    grec.position.y += grec.velocity_y * elapsed;
    
    rrec.position.x += rrec.velocity_x * elapsed;
    rrec.position.y += rrec.velocity_y * elapsed;

    
    modelMatrix.identity();
    modelMatrix.Translate(grec.position.x, grec.position.y, grec.position.z);
    modelMatrix.Rotate(grec.rotation);
    modelMatrix.Scale(grec.scale.x, grec.scale.y, grec.scale.z);
    grec.matrix = modelMatrix;
    
    modelMatrix.identity();
    modelMatrix.Translate(rrec.position.x, rrec.position.y, rrec.position.z);
    modelMatrix.Rotate(rrec.rotation);
    modelMatrix.Scale(rrec.scale.x, rrec.scale.y, rrec.scale.z);
    rrec.matrix = modelMatrix;

    modelMatrix.identity();
    modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
    modelMatrix.Rotate(player.rotation);
    modelMatrix.Scale(player.scale.x, player.scale.y, player.scale.z);
    player.matrix = modelMatrix;
    
    vector<Vector> playerP, grecP, rrecP;
    Vector temp;
    temp.x = temp.y = -1;
    temp.z = 0;
    playerP.push_back(temp * player.matrix);
    grecP.push_back(temp * grec.matrix);
    rrecP.push_back(temp * rrec.matrix);
    temp.x = 1;
    temp.y = -1;
    temp.z = 0;
    playerP.push_back(temp * player.matrix);
    grecP.push_back(temp * grec.matrix);
    rrecP.push_back(temp * rrec.matrix);
    temp.x = 1;
    temp.y = 1;
    temp.z = 0;
    playerP.push_back(temp * player.matrix);
    grecP.push_back(temp * grec.matrix);
    rrecP.push_back(temp * rrec.matrix);
    temp.x = -1;
    temp.y = 1;
    temp.z = 0;
    grecP.push_back(temp * grec.matrix);
    rrecP.push_back(temp * rrec.matrix);
    
    Vector pene;
    if (checkSATCollision(playerP, grecP, pene)){
        player.position.x += pene.x /2;
        player.position.y += pene.y /2;
        grec.position.x -= pene.x/2;
        grec.position.y -= pene.y/2;
        player.velocity_x =0;
        player.velocity_y =0;
        grec.velocity_y =0;
        grec.velocity_x = 0;
    }
    if(checkSATCollision(playerP, rrecP, pene)){
        player.position.x += pene.x /2;
        player.position.y += pene.y /2;
        rrec.position.x -= pene.x/2;
        rrec.position.y -= pene.y/2;
        player.velocity_x =0;
        player.velocity_y =0;
        rrec.velocity_y =0;
        rrec.velocity_x = 0;
    }
    if (checkSATCollision(grecP, rrecP, pene)){
        grec.position.x += pene.x /2;
        grec.position.y += pene.y /2;
        rrec.position.x -= pene.x/2;
        rrec.position.y -= pene.y/2;
        grec.velocity_x =0;
        grec.velocity_y =0;
        rrec.velocity_y =0;
        rrec.velocity_x = 0;
    }

}

void Render(ShaderProgram& program){
    //viewMatrix.identity();
    //viewMatrix.Translate(-player.x, -player.y, 0);
    //program.setViewMatrix(viewMatrix);
    
    
    float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    
    program.setModelMatrix(grec.matrix);
    glBindTexture(GL_TEXTURE_2D, grec.ID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    
    program.setModelMatrix(rrec.matrix);
    glBindTexture(GL_TEXTURE_2D, rrec.ID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    
    float vertices2[] = {-1, -1, 1, -1, 1, 1};
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
    float texCoords2[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0};
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
    
    program.setModelMatrix(player.matrix);
    glBindTexture(GL_TEXTURE_2D, player.ID);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}


void Initiate(ShaderProgram program){
    glEnableVertexAttribArray(program.positionAttribute);
    glEnableVertexAttribArray(program.texCoordAttribute);
    grect = LoadTexture(RESOURCE_FOLDER"rec.png");
    rrect = LoadTexture(RESOURCE_FOLDER"redrec.png");
    tria = LoadTexture(RESOURCE_FOLDER"tri.png");
    
    grec.entityType = ENTITY_ENEMY;
    grec.ID = grect;
    grec.rotation = 30 * (PI / 180.0);
    grec.position.x = 3.5;
    grec.position.y = -1.2;
    grec.position.z = 0;
    grec.scale.x = grec.scale.y = 1.3;
    grec.scale.z = 1;
    grec.velocity_x = -5;
    grec.velocity_y =0;
    
    rrec.entityType = ENTITY_ENEMY;
    rrec.ID = rrect;
    rrec.rotation = 45 * (PI / 180.0);
    rrec.position.x = -3.5;
    rrec.position.y = 1.2;
    rrec.position.z = 0;
    rrec.scale.x = rrec.scale.y = 0.7;
    rrec.scale.z = 1;
    rrec.velocity_x = 5;
    rrec.velocity_y = 0;
    
    player.entityType = ENTITY_PLAYER;
    player.ID = tria;
    player.position.x = player.position.y = player.position.z = 0;
    player.scale.x =player.scale.y = 0.5;
    player.rotation = 25 * (PI / 180.0);
    player.scale.z = 1;
    
    
}


int main(int argc, char *argv[])
{
    //set up the window and matrix
    Setup();
    glViewport(0, 0, 1600, 900);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    projectionMatrix.setOrthoProjection(-6.4f, 6.4f, -3.6f, 3.6f, -1.0f, 1.0f);
    glUseProgram(program.programID);


    
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
