#include <GL/glew.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>

typedef unsigned char byte;
#if RELEASE
#define glOKORDIE 
#else
#define glOKORDIE {GLenum __flatland_gl_error = glGetError(); if (__flatland_gl_error != GL_NO_ERROR) {printf("GL error at %s:%u: %u\n", __FILE__, __LINE__, __flatland_gl_error); exit(1);}}
#endif
#define arrsize(arr) (sizeof((arr))/sizeof(*(arr)))
typedef unsigned int uint;
typedef unsigned char byte;
#define local_persist static
#define MegaBytes(x) ((x)*1024LL*1024LL)


namespace {

# include "shaders.cpp"

  // init
  int screen_w = 800;
  int screen_h = 600;
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

    SDL_Window* window = SDL_CreateWindow("My window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_w, screen_h, SDL_WINDOW_OPENGL);
    SDL_assert(window);

    SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    res = glewInit();
    SDL_assert(res == GLEW_OK);
    glGetError();
    glOKORDIE;

    glViewport(0, 0, screen_w, screen_h);

    return window;
  }

  // time
  typedef Uint64 Timer;
  inline Timer startTimer() {
    return SDL_GetPerformanceCounter();
  }
  inline float getDuration(Timer t) {
    return float(SDL_GetPerformanceCounter() - t)/SDL_GetPerformanceFrequency();
  }

  // math
  inline float frand(float upper) {
    return float(rand())/float(RAND_MAX)*upper;
  }
  inline float frand(float lower, float upper) {
    return float(rand())/float(RAND_MAX)*(upper-lower) + lower;
  }
  inline float frand() {
    return float(rand())/float(RAND_MAX);
  }
  inline float cross(glm::vec2 a, glm::vec2 b) {
    return a.x*b.y - a.y*b.x;
  }
  inline float lengthSqr(glm::vec2 x) {
    return x.x*x.x + x.y*x.y;
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

  // openGL
  GLuint compileShader(const char* vertexShaderSrc, const char* geometryShaderSrc, const char* fragmentShaderSrc)
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

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSrc, NULL);
    glCompileShader(geometryShader);
    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(geometryShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::GEOMETRY::COMPILATION_FAILED: %s\n", infoLog);
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
    glAttachShader(shaderProgram, geometryShader);
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

    /*
    glGenerateMipmap(GL_TEXTURE_2D);
    */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glOKORDIE;

    return result;
  }

  // physics
  struct Rect {
    float x,y,w,h;
  };
  inline Rect point2rect(glm::vec2 pos, float w, float h) {
    return {pos.x - w/2, pos.y - h/2, w, h};
  }
  inline Rect center(Rect x) {
    return {x.x + x.w/2, x.y + x.h/2, x.w, x.h};
  }
  inline glm::vec2 centerPoint(Rect x) {
    return {x.x + x.w/2, x.y + x.h/2};
  }
  inline Rect pad(Rect x, float w, float h) {
    return {x.x-w/2, x.y-h/2, x.w+w, x.h+h};
  }
  inline glm::vec2 bottomLeft(Rect x) {
    return {x.x, x.y};
  }
  inline glm::vec2 topLeft(Rect x) {
    return {x.x, x.y+x.h};
  }
  inline glm::vec2 topRight(Rect x) {
    return {x.x+x.w, x.y+x.h};
  }
  inline glm::vec2 bottomRight(Rect x) {
    return {x.x+x.w, x.y};
  }
  inline bool checkCollision(Rect a, Rect b) {
    return !(a.y > (b.y+b.h) || (a.y+a.h) < b.y || a.x > (b.x+b.w) || (a.x+a.w) < b.x);
  }
  struct Line {glm::vec2 a,b;};
  inline glm::vec2 line2vector(Line l) {
    return l.b - l.a;
  }
  struct RayCollisionAnswer {
    bool hit;
    float t;
  };
  RayCollisionAnswer findRayCollision(Line ray, Line target) {
    const float epsilon = 0.0001;
    using namespace glm;
    RayCollisionAnswer result = {};
    auto r = ray.b - ray.a;
    auto s = target.b - target.a;
    auto d = cross(r,s);
    auto c = target.a-ray.a;
    if (abs(d) >= epsilon) {
      float t = cross(c,s)/d;
      float u = cross(c,r)/d;
      if (t >= 0 && u >= 0 && u <= 1) {
        result.hit = true;
        result.t = t;
      }
    } else {
      // TODO: check for colinearity?
    }
    return result;
  }

  // input
  enum {
    KEY_A,KEY_B,KEY_X,KEY_Y,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_MAX
  };
  bool isDown[KEY_MAX] = {};
  bool wasPressed[KEY_MAX] = {};
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

  // camera
  glm::vec2 viewPosition;

  // sprites
  const int SPRITES_MAX = 1024;
  int numSprites = 1;
  struct Sprite {
    union {
      struct {
        Rect screen, tex;
      };
      struct {
        GLfloat x,y,w,h, tx,ty,tw,th;
      };
    };
  };
  Sprite sprites[SPRITES_MAX];
  Sprite* pushSprite(Sprite s) {
    sprites[numSprites] = s;
    Sprite* result = sprites + numSprites;
    ++numSprites;
    return result;
  }

  // colors
  glm::vec3 FIRE = {226/265.0f, 120/265.0f, 34/265.0f};

  // Player stuff
  struct Player {
    glm::vec2 p;
    glm::vec2 v;
    Sprite* sprite;
  };
  Player player {{}, {}, sprites};

  // Level
  const int WALLS_MAX = 16;
  int numWalls = 1;
  Rect walls[WALLS_MAX] = {{0, 0, 0.4, 0.4}};

  // memory
  const uint STACK_MAX = MegaBytes(128);
  byte _stack_data[STACK_MAX];
  byte* _stack_curr = _stack_data;
  inline void* _push(uint size) {
    SDL_assert(_stack_curr + size < _stack_data + STACK_MAX);
    byte* p = _stack_curr;
    _stack_curr += size;
    return p;
  }
  inline void _pop(uint size) {
    SDL_assert(_stack_curr - size >= _stack_data);
    _stack_curr -= size;
  }
  void* getCurrStackPos() {
    return _stack_curr;
  }
#define push(type) _push(sizeof(type))
#define pushArr(type, count) _push(sizeof(type)*count)
#define pop(type) _pop(sizeof(type))
#define popArr(type, count) _pop(sizeof(type)*count)

};

int main(int, const char*[]) {
  srand(time(0));
  SDL_Window* window = initSubSystems();

  // init player
  {
    player.p = {-0.5, -0.5};
    player.sprite = pushSprite({-0.5, -0.5, 0.2 ,0.2, 11, 7, 70, 88});
  }

  SDL_assert(sizeof(Sprite) == 8*sizeof(GLfloat));
#if 0
  for (int i = 0; i < numSprites; ++i) {
    sprites[i] = {
      {frand(-1,1), frand(-1,1),
      0.2, 0.2,},
      {11,  7,
      70,  88,}
    };
    printf("%f %f\n", sprites[i].screen.x, sprites[i].screen.y);
  }
#endif
  for (int i = 0; i < numWalls; ++i) {
    pushSprite({center(walls[i]), 0, 282, 142, 142});
  }


  // create sprite buffer
  GLuint sprites_VAO;
  GLuint sprites_VBO;
  {
    glGenVertexArrays(1, &sprites_VAO);
    glBindVertexArray(sprites_VAO);
    glGenBuffers(1, &sprites_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, sprites_VBO);
    glBufferData(GL_ARRAY_BUFFER, numSprites*sizeof(Sprite), sprites, GL_DYNAMIC_DRAW);
    glOKORDIE;

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glOKORDIE;
  }
  
  // compile shaders and set static uniforms
  GLuint spriteShader;
  GLint viewLocation;
  {
    spriteShader = compileShader(sprite_vertex_shader_src, sprite_geometry_shader_src, sprite_fragment_shader_src);
    glUseProgram(spriteShader);
    glOKORDIE;
    // bind the spritesheet
    GLuint wallTexture = loadTexture("assets/spritesheet.png");
    glOKORDIE;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glUniform1i(glGetUniformLocation(spriteShader, "uTexture"), 0);
    glOKORDIE;
    // get locations
    viewLocation = glGetUniformLocation(spriteShader, "uView");
    GLuint ambientLightLocation = glGetUniformLocation(spriteShader, "uAmbientLight");
    glUniform3f(ambientLightLocation, 1, 1, 1);
    glOKORDIE;
  }

  // set lights
  SDL_SetRelativeMouseMode(SDL_TRUE);
  SDL_GetRelativeMouseState(NULL, NULL);

  Timer loopTime = startTimer();
  int loopCount = 0;
  while (true)
  {
    ++loopCount;
    float dt = getDuration(loopTime);
    loopTime = startTimer();

    // parse events
    int mouseDX, mouseDY;
    bool wasPressed[KEY_MAX] = {};
    {
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
                wasPressed[i] = !isDown[i];
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
      SDL_GetRelativeMouseState(&mouseDX, &mouseDY);
    }

    // move player
    {
      const float ACCELLERATION = 3;
      const float GRAVITY = 2;
      const float JUMPPOWER = 0.1;
      player.v.x += ACCELLERATION*dt*isDown[KEY_RIGHT] - ACCELLERATION*dt*isDown[KEY_LEFT];
      // player.v.y = SPEED*dt*isDown[KEY_UP] - SPEED*dt*isDown[KEY_DOWN];
      if (wasPressed[KEY_A] && numWalls > 0) {
        glm::vec2 p = centerPoint(*walls);
        float minD = glm::distance(player.p, p);
        for (int i = 1; i < numWalls; ++i) {
          auto d = glm::distance(player.p, centerPoint(walls[i]));
          if (d < minD) {
            minD = d;
            p = centerPoint(walls[i]);
          }
        }
        const float FORCE = 3;
        player.v = glm::normalize(p - player.p) * FORCE;
      }
      /*
      if (wasPressed[KEY_UP]) {
        player.v.y = JUMPPOWER;
      }
      */
      player.v.y -= GRAVITY * dt;

      // TODO: collision
      {
        // find the possible collisions for the move vector
        glm::vec2 oldPos = player.p;
        glm::vec2 newPos = oldPos + (player.v * dt);
        glm::vec2 move = newPos - oldPos;
        glm::vec2 hitboxSize = glm::vec2(player.sprite->w, player.sprite->h);
        Rect moveBox = {
          glm::min(oldPos.x, newPos.x) - hitboxSize.x/2,
          glm::min(oldPos.y, newPos.y) - hitboxSize.y/2,
          glm::abs(move.x) + hitboxSize.x,
          glm::abs(move.y) + hitboxSize.y,
        };
        int numW = 0;
        Rect* w = 0;
        for (int i = 0; i < numWalls; ++i) {
          if (checkCollision(moveBox, walls[i])) {
            Rect* wall = (Rect*) push(Rect);
            *wall = walls[i];
            ++numW; 
            if (!w) {
              w = wall;
            }
          }
        }

        bool hit;
        do {
          float t = 1.0f;
          glm::vec2 endPoint = oldPos + move;
          hit = false;
          glm::vec2 wall;
          for (int i = 0; i < numW; ++i) {
            // find the shortest T for collision
            // TODO: Optimize: If we only use horizontal and vertical lines, we can optimize. Also precalculate corners, we do it twice now
            Rect padded = pad(w[i], player.sprite->w, player.sprite->h);
            Line lines[4] = {
              Line{bottomLeft(padded), bottomRight(padded)},
              Line{bottomRight(padded), topRight(padded)},
              Line{topRight(padded), topLeft(padded)},
              Line{topLeft(padded), bottomLeft(padded)},
            };
            for (int l = 0; l < 4; ++l) {
              RayCollisionAnswer r = findRayCollision(Line{oldPos, endPoint}, lines[l]);
              if (r.hit && r.t < t) {
                t = r.t;
                hit = true;
                wall = line2vector(lines[l]);
              }
            }
            SDL_assert(t >= 0);
          }
          if (hit) {
            auto glide = dot(move, wall)*(1.0f-t) * wall / lengthSqr(wall);
            // move up to wall
            const float epsilon = 0.1;
            move *= glm::max(0.0f, t-epsilon);
            // and add glide
            move += glide;
            // go on to recalculate wall collisions, this time with the new move
          }
        } while (hit);
        popArr(Rect, numW);
        SDL_assert(!w || getCurrStackPos() == (void*) w);
        player.v = move/dt;
      }
      player.p += player.v * dt;
      player.sprite->screen.x = player.p.x;
      player.sprite->screen.y = player.p.y;
    }

    // draw sprites
    {
      glClearColor(1,0,0,1);
      glClear(GL_COLOR_BUFFER_BIT);
      glUseProgram(spriteShader);
      glBindBuffer(GL_ARRAY_BUFFER, sprites_VBO);
      glBufferData(GL_ARRAY_BUFFER, numSprites*sizeof(Sprite), sprites, GL_DYNAMIC_DRAW);
      glUniform2f(viewLocation, viewPosition.x, viewPosition.y);
      glBindVertexArray(sprites_VAO);
      glDrawArrays(GL_POINTS, 0, numSprites);
      glOKORDIE;
    }

    SDL_GL_SwapWindow(window);

    if (!(loopCount%100)) {
      printf("%f fps\n", 1/dt);
    }
  }
}
