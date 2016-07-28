#include <GL/glew.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <stdio.h>

typedef unsigned char byte;
#if RELEASE
#define glOKORDIE 
#else
#define glOKORDIE {GLenum __flatland_gl_error = glGetError(); if (__flatland_gl_error != GL_NO_ERROR) {printf("GL error at %s:%u: %u\n", __FILE__, __LINE__, __flatland_gl_error);}}
#endif
#define arrsize(arr) (sizeof((arr))/sizeof(*(arr)))
typedef unsigned int uint;
#define local_persist static


namespace {

# include "shaders.cpp"

  typedef Uint64 Timer;
  inline Timer startTimer() {
    return SDL_GetPerformanceCounter();
  }

  inline float getDuration(Timer t) {
    return float(SDL_GetPerformanceCounter() - t)/SDL_GetPerformanceFrequency();
  }

  inline float time2fps(float seconds) {
    return 1/seconds;
  }

  int screenW = 800;
  int screenH = 600;

  // floating point rand functions
  inline float frand(float upper) {
    return float(rand())/float(RAND_MAX)*upper;
  }
  inline float frand(float lower, float upper) {
    return float(rand())/float(RAND_MAX)*(upper-lower) + lower;
  }
  inline float frand() {
    return float(rand())/float(RAND_MAX);
  }

  void print(glm::mat4 in)
  {
    const float* mat = glm::value_ptr(in);
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        printf("%f ", mat[i*4 + j]);
      }
      printf("\n");
    }
  }

  GLuint compileShader(const char* vertexShaderSrc, const char* fragmenShaderSrc)
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

    GLuint shaderProgram = glCreateProgram();
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

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glOKORDIE;
    return shaderProgram;
  }

  SDL_Window* initSubSystems()
  {
    int res;
    res = SDL_Init(SDL_INIT_EVERYTHING);
    SDL_assert(!res);

    res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_assert(!res);
    res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_assert(!res);
    res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_assert(!res);

    SDL_Window* window = SDL_CreateWindow("My window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, SDL_WINDOW_OPENGL);
    SDL_assert(window);

    SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    res = glewInit();
    SDL_assert(res == GLEW_OK);
    glGetError();
    glOKORDIE;

    return window;
  }

  GLuint loadTexture(const char* filename)
  {
    GLuint result;
    glCreateTextures(GL_TEXTURE_2D, 1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glOKORDIE;

    int w,h;
    byte* image = SOIL_load_image(filename, &w, &h, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);
    glOKORDIE;

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glOKORDIE;

    return result;
  }

  enum {
    KEY_A,KEY_B,KEY_X,KEY_Y,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_MAX
  };

  int keyCodes[KEY_MAX] = {
    SDLK_t,
    SDLK_y,
    SDLK_r,
    SDLK_e,
    SDLK_UP,
    SDLK_DOWN,
    SDLK_LEFT,
    SDLK_RIGHT,
  };

  bool isDown[KEY_MAX] = {};

  glm::vec2 viewPosition;

  const int LIGHTS_MAX = 1024;
  int numLights = 64;
  glm::vec2 lightPositions[LIGHTS_MAX];
  glm::vec3 lightColors[LIGHTS_MAX];

  const int SPRITES_MAX = 1024;
  int numSprites = 1;

  struct Sprite {
    GLfloat x, y, w, h, tx, ty, tw, th;
  };

  Sprite sprites[SPRITES_MAX];

  // some colors
  glm::vec3 FIRE = {226/265.0f, 120/265.0f, 34/265.0f};
};

int main(int, const char*[]) {
  SDL_Window* window = initSubSystems();

  SDL_assert(sizeof(Sprite) == 8*sizeof(GLfloat));
  for (int i = 0; i < numSprites; ++i) {
    sprites[i] = {
      frand(-1,1), frand(-1,1),
      0.2, 0.2,
      0,0,
      1,1,
    };
  }
  // sprites[0] = {0, 0, 1, 1, 0, 0, 6, 6};

  // create a square
  GLuint sprites_VAO;
  {
    GLuint sprites_VBO;
    glGenVertexArrays(1, &sprites_VAO);
    glBindVertexArray(sprites_VAO);
    glGenBuffers(1, &sprites_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, sprites_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sprites), sprites, GL_DYNAMIC_DRAW);
    glOKORDIE;

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glOKORDIE;
  }
  
  // initialize lights
  {
    for (int i = 0; i < numLights; ++i) {
      lightColors[i] = FIRE;
    }
  }

  // compile shaders and set static uniforms
  GLuint spriteShader;
  GLint viewLocation;
  GLint lightPositionLocation;
  GLint lightColorLocation;
  GLint numLightsLocation;
  {
    spriteShader = compileShader(vertexShaderSrc, fragmentShaderSrc);
    glUseProgram(spriteShader);
    glOKORDIE;
    // bind the spritesheet
    GLuint wallTexture = loadTexture("assets/wall.jpg");
    glOKORDIE;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glUniform1i(glGetUniformLocation(spriteShader, "uTexture"), 0);
    glOKORDIE;
    // set lights
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_GetRelativeMouseState(NULL, NULL);
    // get locations
    viewLocation = glGetUniformLocation(spriteShader, "uView");
    // lightPositionLocation = glGetUniformLocation(spriteShader, "uLightPos");
    // lightColorLocation = glGetUniformLocation(spriteShader, "uLightColor");
    // numLightsLocation = glGetUniformLocation(spriteShader, "uNumLights");
    GLuint ambientLightLocation = glGetUniformLocation(spriteShader, "uAmbientLight");
    glUniform3f(ambientLightLocation, 0.3, 0.3, 0.3);
    glOKORDIE;
  }

  glViewport(0, 0, screenW, screenH);

  while (true)
  {
    Timer loopTime = startTimer();

    glClearColor(1,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // parse events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type) {
        case SDL_KEYDOWN: {
          printf("A key was pressed\n");
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            exit(0);
          }
          for (int i = 0; i < KEY_MAX; ++i) {
            if (event.key.keysym.sym == keyCodes[i]) {
              isDown[i] = true;
            }
          }
        } break;
        case SDL_KEYUP: {
          printf("A key was released\n");
          for (int i = 0; i < KEY_MAX; ++i) {
            if (event.key.keysym.sym == keyCodes[i]) {
              isDown[i] = false;
            }
          }
        } break;
        case SDL_WINDOWEVENT: {
          if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
            exit(0);
          }
        } break;
        default: break;
      }
    }
    int mouseDX, mouseDY;
    SDL_GetRelativeMouseState(&mouseDX, &mouseDY);

    const float SPEED = 0.01;
    if (isDown[KEY_RIGHT]) {
      viewPosition.x += SPEED;
      printf("Moving right\n");
    }
    if (isDown[KEY_LEFT]) {
      viewPosition.x -= SPEED;
      printf("Moving left\n");
    }

    glUseProgram(spriteShader);
    if (mouseDX != 0 || mouseDY != 0) {
      for (int i = 0; i < numLights; ++i) {
        lightPositions[i] += glm::vec2((float) mouseDX/screenW, (float) -mouseDY/screenH);
        lightPositions[i] = glm::clamp(lightPositions[i], glm::vec2(-1,-1), glm::vec2(1,1));
      }
    } else {
      local_persist glm::vec2 directions[LIGHTS_MAX];
      for (int i = 0; i < numLights; ++i) {
        float SPEED = 0.01f;
        if (!(rand()%20)) {
          float alpha = frand(0, 2*glm::pi<float>());
          directions[i] = SPEED * glm::vec2(glm::cos(alpha), glm::sin(alpha));
        }
        lightPositions[i] += directions[i];
        lightPositions[i] = glm::clamp(lightPositions[i], glm::vec2(-1,-1), glm::vec2(1,1));
      }
    }
    // glUniform2fv(lightPositionLocation, numLights, (GLfloat*) &lightPositions[0]);
    // glUniform3fv(lightColorLocation, numLights, (GLfloat*) lightColors);

    glUniform2f(viewLocation, viewPosition.x, viewPosition.y);
    glBindVertexArray(sprites_VAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numSprites);
    glOKORDIE;

    SDL_GL_SwapWindow(window);

    printf("%f fps\n", time2fps(getDuration(loopTime)));
  }
}
