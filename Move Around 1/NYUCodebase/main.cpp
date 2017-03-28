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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Three Bunnies", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    
    glViewport(0, 0, 640, 360);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    GLuint storm  = LoadTexture(RESOURCE_FOLDER"pinkstar.png");
    GLuint bunny1  = LoadTexture(RESOURCE_FOLDER"bunny.png");
    GLuint bunny2  = LoadTexture(RESOURCE_FOLDER"bunny2.png");
    GLuint bunny3  = LoadTexture(RESOURCE_FOLDER"bunny3.png");
    
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    glUseProgram(program.programID);
    
    float lastFrameTicks = 0.0f;
    float scale1 = 0;
    float angle1 = 0;
    float move2 = 0;
    float move3 = 0;
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
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

        glBindTexture(GL_TEXTURE_2D, storm);
        float vertices[] = {-3.5, -3.5, 3.5, -3.5, 3.5, 3.5, -3.5, -3.5, 3.5, 3.5, -3.5, 3.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // code to set up the first(main) bunny
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        scale1 += 0.1* elapsed;
        angle1 += -300* elapsed;
        modelMatrix.identity();
        modelMatrix.Scale(scale1, scale1 , 1.0f);
        modelMatrix.Rotate(angle1* (3.1415926 / 180.0));
        program.setModelMatrix(modelMatrix);

        glBindTexture(GL_TEXTURE_2D, bunny1);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // code to set up the other bunny
        move2 += -2.5*elapsed;
        modelMatrix.identity();
        modelMatrix.Translate(2.5f, -1.2f, 0.0f);
        modelMatrix.Scale(0.2f, 0.2f, 0.2f);
        modelMatrix.Translate(move2, 0.0f, 0.0f);
        program.setModelMatrix(modelMatrix);
        glBindTexture(GL_TEXTURE_2D, bunny2);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // there is even another bunny?!
        move3 += 2.5*elapsed;
        modelMatrix.identity();
        modelMatrix.Translate(-2.5f, -1.2f, 0.0f);
        modelMatrix.Scale(0.2f, 0.2f, 0.2f);
        modelMatrix.Translate(move3, 0.0f, 0.0f);
        program.setModelMatrix(modelMatrix);
        glBindTexture(GL_TEXTURE_2D, bunny3);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
