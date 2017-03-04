
//Shuyuan Luo
/*
 Game rules: 
 1. press space to start the game
 2. on the left corner is your current score, and on the right corner is your current life(start with 3)
 3. press left and right to move and press space to shoot (you can only shoot once per second)
 4. monsters will drop bullets randomly, you will lose one life if they hit you!
 Note: I tried to not let dead monster drop bullet, but then it became really hard to pick a random alive monster, so the bullet could be droped
 anywhere from that block of monster
 5. if you get touched by a monster or the monster went over you, you lose (to a game over screen)
 6. If you hit the extra SCARY monster, you will get 9999 more points!
 7. if you kill all the monsters, you WIN! (to a game win screen)
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
using namespace std;
#define PI 3.14159265
#define FIXED_TIMESTEP 0.00366666f
#define MAX_TIMESTEPS 1

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

class Entity {
public:
    Entity (){
        direction_y = 0;
        direction_x = 0;
        dead = true;
    }
    Entity(float tx, float ty){
        //this is used to initialize a ghost!
        x = tx;
        y = ty;
        width = height = 0.15;
        speed = 30;
        direction_x = 1;
        direction_y = 0;
        dead = false;
    }
    void Draw(){
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 6);

    }
    float start;
    float x;
    float y;
    float rotation;
    int textureID;
    float width;
    float height;
    float speed;
    float direction_x;
    float direction_y;
    bool dead;
    int index;
};

Entity sailor_moon;
SDL_Window* displayWindow;
float elapsed;
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
float angle;
int life;
int counter;
int bl_count;
float multiplier;
GLuint logoID, up_bl, down_bl;
float animationElapsed, bulletElasped, bl_timer_up;
int score, state;
bool start, win;


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
    displayWindow = SDL_CreateWindow("Ghost Ivader!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
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

enum GameState { STATE_START, STATE_GAME, STATE_GAME_OVER, STATE_WIN};

void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX,
                           int spriteCountY) {
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
    
    GLuint monster  = LoadTexture(RESOURCE_FOLDER"sprite_mon.png");
    glBindTexture(GL_TEXTURE_2D, monster);
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
    GLuint asc  = LoadTexture(RESOURCE_FOLDER"asc.png");
    glBindTexture(GL_TEXTURE_2D, asc);
    glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);
}

void shootBullet(vector<Entity> &bad_bullet, float tx, float ty, float dy) {
    Entity *newBullet =  new Entity;
    newBullet->x = tx; // where the bullet starts X
    newBullet->y = ty;     // where the bullet starts Y
    newBullet->direction_y = dy;
    newBullet->speed = 70;
    newBullet->width = newBullet->height = 0.2;
    newBullet->dead = false;
    bad_bullet.push_back(*newBullet);
    bl_count++;
}

bool if_win(vector<Entity*> &monsters) {
    for (int i = 0; i < monsters.size(); i++){
        if(monsters[i]->dead == false){
            return false;
        }
    }
    return true;
}

void Update(float elapsed, ShaderProgram& program, vector<Entity*> &monsters, vector<Entity> &bad_bullet ){
    if (start == false ){
        state = STATE_START;
    } else if (sailor_moon.dead == true ){
        state = STATE_GAME_OVER;
    }else if(win == true){
        state = STATE_WIN;
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
            modelMatrix.Translate(0, 1, 0);
            modelMatrix.Scale(2.2, 2.2, 1);
            program.setModelMatrix(modelMatrix);
            GLuint ss  = LoadTexture(RESOURCE_FOLDER"start.png");
            glBindTexture(GL_TEXTURE_2D, ss);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            modelMatrix.identity();
            modelMatrix.Translate(-3, -2, 0);
            program.setModelMatrix(modelMatrix);
            string s_score = to_string(score);
            DrawText(&program, "Press Space to Start!", 0.5f, -0.2f);
            break;}
            
            
        case STATE_GAME: {
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            
            // title
            modelMatrix.identity();
            modelMatrix.Translate(0, 3.2, 0);
            modelMatrix.Scale(1, 0.4, 1);
            program.setModelMatrix(modelMatrix);
            glBindTexture(GL_TEXTURE_2D, logoID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            //the life of sailor moon
            for(int i = 1; i <= life; i ++){
                modelMatrix.identity();
                modelMatrix.Translate(3+ 0.8*i, 3.2, 0);
                modelMatrix.Scale(0.3, 0.3, 1);
                program.setModelMatrix(modelMatrix);
                glBindTexture(GL_TEXTURE_2D, sailor_moon.textureID);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            // sailor moon!
            bl_timer_up += elapsed;
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if(keys[SDL_SCANCODE_LEFT]) {
                sailor_moon.x -= elapsed * sailor_moon.speed;
            } else if (keys[SDL_SCANCODE_RIGHT]) {
                sailor_moon.x += elapsed * sailor_moon.speed;
            }
            /*else if (keys[SDL_SCANCODE_SPACE] && bl_timer_up > 1/5){
                shootBullet(bad_bullet, sailor_moon.x, sailor_moon.y, 1);
                bl_timer_up = 0;
            }*/
            if (life >0 && sailor_moon.dead == false ){
                modelMatrix.identity();
                modelMatrix.Translate(sailor_moon.x, sailor_moon.y, 0);
                modelMatrix.Scale(0.5, 0.5, 1);
                program.setModelMatrix(modelMatrix);
                glBindTexture(GL_TEXTURE_2D, sailor_moon.textureID);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            
            
            //draw all the bullets
            for (int i = 0; i < bad_bullet.size(); i++) {
                Entity b = bad_bullet[i];
                if (b.direction_y !=0 && b.dead == false){
                    bad_bullet[i].y = b.y + elapsed * b.speed * b.direction_y;
                    modelMatrix.identity();
                    modelMatrix.Translate(b.x, bad_bullet[i].y, 0);

                    if (sailor_moon.dead == false && b.direction_y>0){
                        modelMatrix.Scale(0.1, 0.1, 1);
                        program.setModelMatrix(modelMatrix);
                        glBindTexture(GL_TEXTURE_2D, up_bl);
                        glDrawArrays(GL_TRIANGLES, 0, 6);

                    }else {
                        modelMatrix.Scale(0.06, 0.13, 1);
                        program.setModelMatrix(modelMatrix);
                        glBindTexture(GL_TEXTURE_2D, down_bl);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                }
            }
            
            // if  bullet hits any enemy!
            for (int i = 0; i < bad_bullet.size(); i++){
                for (int j = 0; j < monsters.size(); j++){
                    if (if_collision (bad_bullet[i], *monsters[j]) && bad_bullet[i].direction_y >0){
                        bad_bullet[i].dead = true;
                        monsters[j]->dead = true;
                        score = score + 1000;
                        if (monsters[j]->index == 7){
                            score = score + 9999;
                        }
                    }
                }
            }
            
            //all the ghost
            if (monsters[0]->x - monsters[0]->start >3.5){
                for(int i = 0; i < monsters.size(); i++){
                    monsters[i]->direction_x = -1;
                    monsters[i]->y -= 0.08;
                    monsters[i]->speed += multiplier;
                    counter = 0;
                }
            }
            if (monsters[0]->x < monsters[0]->start){
                for(int i = 0; i < monsters.size(); i++){
                    monsters[i]->direction_x = 1;
                    monsters[i]->y -= 0.08;
                    monsters[i]->speed += multiplier;
                    counter = 0;
                }
            }
            for (int i = 0; i < monsters.size(); i++){
                Entity target = *monsters[i];
                if (target.dead == false){
                    monsters[i]->x += elapsed*target.speed* target.direction_x;
                    modelMatrix.identity();
                    modelMatrix.Translate(monsters[i]->x, monsters[i]->y, 0);
                    modelMatrix.Scale(0.45, 0.45, 1);
                    program.setModelMatrix(modelMatrix);
                    counter++;
                    animationElapsed += elapsed;
                    if(animationElapsed > 1.0/35) {
                        monsters[i]->index = (monsters[i]->index + 1)%8;
                        animationElapsed = 0.0;
                    }
                    DrawSpriteSheetSprite(&program, monsters[i]->index, 5,5);
                }
            }
            
            // let a random monster to shoot a bullet
            int random= rand() % monsters.size();
           // while (monsters[random]->dead == true){
                srand (time(NULL));
                random  = rand() % monsters.size();
           // }
            bulletElasped += elapsed;
            if(bulletElasped > 1.0/15) {
                shootBullet(bad_bullet, monsters[random]->x, monsters[random]->y, -1);
                bulletElasped = 0.0;
            }
            
            //see if the bullet hit sailor moon, NO!!!!
            for (int i = 0; i < bad_bullet.size(); i++){
                if (if_collision(sailor_moon, bad_bullet[i])&& sailor_moon.dead == false && bad_bullet[i].dead == false && bad_bullet[i].direction_y < 0 ){
                    life--;
                    bad_bullet[i].dead = true;
                }
            }
            if(life <= 0){
                sailor_moon.dead = true;
            }
            // if the ghost hit sailor moon, or passed her (OH NO!!!)
            for (int i = 0; i < monsters.size(); i++){
                if (if_collision(sailor_moon, *monsters[i])){
                    sailor_moon.dead = true;
                    life = 0;
                }
            }
            
            
            win = if_win(monsters);
            if (monsters[0]->y < sailor_moon.y - sailor_moon.height/2 && win == false ){
                sailor_moon.dead = true;
                life = 0;
            }
            
            
            modelMatrix.identity();
            modelMatrix.Translate(2.3, 3.2, 0);
            program.setModelMatrix(modelMatrix);
            DrawText(&program, "Life: ", 0.3f, -0.1f);

            modelMatrix.identity();
            modelMatrix.Translate(-5.1, 3.2, 0);
            program.setModelMatrix(modelMatrix);
            string s_score = to_string(score);
            string the_score = "Score: " + s_score;
            DrawText(&program, the_score, 0.3f, -0.1f);
            break;}
        case STATE_GAME_OVER :{
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            
            modelMatrix.identity();
            modelMatrix.Translate(0, 1, 0);
            modelMatrix.Scale(2.2, 2.2, 1);
            program.setModelMatrix(modelMatrix);
            GLuint sad  = LoadTexture(RESOURCE_FOLDER"sad.png");
            glBindTexture(GL_TEXTURE_2D, sad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4.2, -2, 0);
            program.setModelMatrix(modelMatrix);
            string s_score = to_string(score);
            DrawText(&program, "Hope You Can Do Better Next Time!", 0.5f, -0.2f);
            break;}
            
        case STATE_WIN :{
            float vertices[] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
            glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
            glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            
            modelMatrix.identity();
            modelMatrix.Translate(0, 1, 0);
            modelMatrix.Scale(2.2, 2.2, 1);
            program.setModelMatrix(modelMatrix);
            GLuint sad  = LoadTexture(RESOURCE_FOLDER"sailor.png");
            glBindTexture(GL_TEXTURE_2D, sad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            modelMatrix.identity();
            modelMatrix.Translate(-4.2, -2, 0);
            program.setModelMatrix(modelMatrix);
            string s_score = to_string(score);
            DrawText(&program, "You Are Awesome!", 0.5f, -0.2f);
            break;}


        }
}
    
    


void Initiate(ShaderProgram program, vector<Entity*> & monsters, vector<Entity> & bad_bullet){
    glEnableVertexAttribArray(program.positionAttribute);
    glEnableVertexAttribArray(program.texCoordAttribute);

    GLuint sailor_moonID  = LoadTexture(RESOURCE_FOLDER"sailor.png");
    up_bl  = LoadTexture(RESOURCE_FOLDER"moon.png");
    down_bl  = LoadTexture(RESOURCE_FOLDER"bullet.png");
    logoID  = LoadTexture(RESOURCE_FOLDER"title.png");
    
    sailor_moon.textureID = sailor_moonID;
    sailor_moon.x = 0;
    sailor_moon.y = -3;
    sailor_moon.width = sailor_moon.height = 0.6;
    sailor_moon.speed = 60;
    sailor_moon.dead = false;
    life = 3;
    counter = 0;
    multiplier = 4;
    bl_count = 0;
    bl_timer_up = 1.0/20;
    animationElapsed = 0;
    bulletElasped = 1.0/15;
    score = 0;
    start = false;
    win = false;
    
    int c_index =0;
    //generate all the monsters to start with!
    for (int i = 0; i < 40; i++){
        if (i == 0){
            Entity *temp = new Entity(-5.7, 2.2);
            temp->start = temp->x;
            temp->index = c_index%8;
            c_index++;
            monsters.push_back(temp);
        }else {
            Entity *temp;
            if (i % 10 ==0){
                temp = new Entity(-5.7 ,monsters[i-1]->y-0.25);
                temp->start = temp->x;
                temp->index = c_index%8;
                c_index++;

            }else {
                temp = new Entity(monsters[i-1]->x + 0.8 ,monsters[i-1]->y);
                temp->start = temp->x;
                temp->index = c_index%8;
                c_index++;

            }
            monsters.push_back(temp);
        }
    }

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
    
    
    
    vector<Entity*> monsters;
    vector<Entity> bad_bullet;

    Initiate(program, monsters, bad_bullet);

    
    while (!done) {
        bl_timer_up += elapsed;
        while (SDL_PollEvent(&event)) {
            ProcessEvents(event, done);
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if (keys[SDL_SCANCODE_SPACE] && state == STATE_START){
                start = true;
            }
            if (keys[SDL_SCANCODE_SPACE] && bl_timer_up > 1 && state == STATE_GAME){
                shootBullet(bad_bullet, sailor_moon.x, sailor_moon.y, 1);
                bl_timer_up = 0;
            }

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
        
        

        //set up background
        float fixedElapsed = elapsed;
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
            Update(FIXED_TIMESTEP,program, monsters, bad_bullet);
        }
        Update(fixedElapsed, program, monsters, bad_bullet);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    for (int i = 0; i < monsters.size(); i++){
         delete monsters[i];
    }
    SDL_Quit();
    return 0;
}
