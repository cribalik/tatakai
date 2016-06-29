#if 1
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <stdio.h>
#include "shaders.cpp"
#include <iostream>

typedef unsigned char byte;
#define glOKORDIE {GLenum __flatland_gl_error = glGetError(); if (__flatland_gl_error != GL_NO_ERROR) {printf("GL error at %s:%u: %u\n", __FILE__, __LINE__, __flatland_gl_error);}}
#define PI 3.141592651f

namespace {
  void print(glm::mat4 in) {
    const float* mat = glm::value_ptr(in);
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        printf("%f ", mat[i*4 + j]);
      }
      printf("\n");
    }
  }
};

int main(int, const char*[]) {
  int res;

  res = SDL_Init(SDL_INIT_EVERYTHING);
  SDL_assert(!res);

  res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_assert(!res);
  res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_assert(!res);
  res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_assert(!res);

  SDL_Window* window = SDL_CreateWindow("My window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 800, SDL_WINDOW_OPENGL);
  SDL_assert(window);

  SDL_GL_CreateContext(window);

  glewExperimental = GL_TRUE;
  res = glewInit();
  SDL_assert(res == GLEW_OK);
  glGetError();
  glOKORDIE;

  // create a square
  GLuint square_VAO;
  {
    const GLfloat vertices[] = {
         // pos          //color       //texture
      -0.5, -0.5, 0,  0.0,  0.5, 0,    0.0,  0.0,
      -0.5,  0.5, 0,  0.5,  0.5, 0,    0.0,  1.0,
      0.5,  -0.5, 0,  0.5,  0.0, 0,    1.0,  0.0,
      0.5,   0.5, 0,  0.5,  0.5, 0.5,  1.0,  1.0,
    };
    GLuint square_VBO;
    glGenVertexArrays(1, &square_VAO);
    glBindVertexArray(square_VAO);
    glGenBuffers(1, &square_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, square_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glOKORDIE;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
  }
  glOKORDIE;

  // create a texture
  GLuint wallTexture;
  GLuint smileyTexture;
  {
    glCreateTextures(GL_TEXTURE_2D, 1, &wallTexture);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glOKORDIE;

    int w,h;
    byte* image = SOIL_load_image("assets/wall.jpg", &w, &h, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);
    glOKORDIE;

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glOKORDIE;

    // smiley face
    glCreateTextures(GL_TEXTURE_2D, 1, &smileyTexture);
    glBindTexture(GL_TEXTURE_2D, smileyTexture);
    glOKORDIE;

    image = SOIL_load_image("assets/awesomeface.png", &w, &h, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);
    glOKORDIE;

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glOKORDIE;
  }

  // compile shaders
  GLuint shaderProgram;
  {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
    glCompileShader(vertexShader);

    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n", infoLog);
      }
    }


    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
    glCompileShader(fragmentShader);

    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n", infoLog);
      }
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fragmentShader);
    glAttachShader(shaderProgram, vertexShader);
    glLinkProgram(shaderProgram);

    {
      GLint success;
      GLchar infoLog[512];
      glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED: %s\n", infoLog);
      }
    }

    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
  }
  glOKORDIE;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, wallTexture);
  glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, smileyTexture);
  glUniform1i(glGetUniformLocation(shaderProgram, "uTexture2"), 1);
  glOKORDIE;

  glm::mat4 transform;
  transform = glm::rotate(transform, 90.0f*2*PI/360, glm::vec3(0.0, 0.0, 1.0));
  transform = glm::scale(transform, glm::vec3(0.2, 0.2, 0.2));
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uTransform"), 1, GL_FALSE, glm::value_ptr(transform));
  glOKORDIE;

  while (true)
  {
    glClearColor(1,1,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    transform = glm::rotate(glm::mat4(), SDL_GetTicks()/360.f, glm::vec3(0.0, 0.0, 1.0));
    transform = glm::scale(transform, glm::vec3(0.2, 0.2, 0.2));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uTransform"), 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(square_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SDL_GL_SwapWindow(window);

    // parse events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN) {
        printf("A key was pressed\n");
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          exit(0);
        }
      }
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        exit(0);
      }
    }
  }
}
