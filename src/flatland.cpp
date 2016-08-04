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

  // Init
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

  // Time
  typedef Uint64 Timer;
  inline Timer startTimer() {
    return SDL_GetPerformanceCounter();
  }
  inline float getDuration(Timer t) {
    return float(SDL_GetPerformanceCounter() - t)/SDL_GetPerformanceFrequency();
  }

  // Math
  inline float frand(float upper) {
    return float(rand())/float(RAND_MAX)*upper;
  }
  inline float frand(float lower, float upper) {
    return float(rand())/float(RAND_MAX)*(upper-lower) + lower;
  }
  inline float frand() {
    return float(rand())/float(RAND_MAX);
  }
  inline glm::vec2 v2rand(float size) {
    return glm::vec2(frand(-size, size), frand(-size, size));
  }
  inline float cross(glm::vec2 a, glm::vec2 b) {
    return a.x*b.y - a.y*b.x;
  }
  inline float lengthSqr(glm::vec2 x) {
    return x.x*x.x + x.y*x.y;
  }

  // openGL
  GLuint compileShader(const char* vertexShaderSrc, const char* geometryShaderSrc, const char* fragmentShaderSrc) {
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

  GLuint loadTexture(const char* filename) {
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

  // Physics
  struct Rect {
    float x,y,w,h;
  };
  inline Rect operator+(Rect r, glm::vec2 v) {
    return Rect{r.x + v.x, r.y + v.y, r.w, r.h};
  }
  inline Rect operator+(glm::vec2 v, Rect r) {
    return r + v;
  }
  inline Rect rectAround(glm::vec2 pos, float w, float h) {
    return {pos.x - w/2, pos.y - h/2, w, h};
  }
  inline glm::vec2 size(Rect x) {
    return {x.w,x.h};
  }
  inline glm::vec2 center(Rect x) {
    return {x.x + x.w/2, x.y + x.h/2};
  }
  inline Rect pad(Rect a, Rect b) {
    a.x -= b.w+b.x;
    a.y -= b.h+b.y;
    a.w += b.w;
    a.h += b.h;
    return a;
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
  inline bool checkCollision(Rect r, glm::vec2 p) {
    return !(p.x < r.x || p.x > (r.x + r.w) || p.y > (r.y + r.h) || p.y < r.y);
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

  // Input
  enum {
    KEY_A,KEY_B,KEY_X,KEY_Y,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_MAX
  };
  bool isDown[KEY_MAX] = {};
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

  // Camera
  glm::vec2 viewPosition;

  // Sprites
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
  Sprite* addSprite(Sprite s) {
    sprites[numSprites] = s;
    Sprite* result = sprites + numSprites;
    ++numSprites;
    return result;
  }

  // Colors
  glm::vec3 FIRE = {226/265.0f, 120/265.0f, 34/265.0f};

  // Entities
  enum class EntityType : uint {
    player,
    fairy,
    wall,
  };
  enum EntityFlag : Uint32 {
    EntityFlag_Collides = 1 << 0,
  };
  struct Entity {
    EntityType type;
    Uint32 flags;

    glm::vec2 pos;
    glm::vec2 vel;
    Sprite* sprite;
    Rect hitBox;

    // Familiar stuff
    Entity** follow;

    // reference location
    Entity** _referer;

  };

  inline bool isset(Entity* e, EntityFlag flag) {
    return e->flags & flag;
  }
  inline void set(Entity* e, EntityFlag flag) {
    e->flags |= flag;
  }
  inline void unset(Entity* e, EntityFlag flag) {
    e->flags &= ~flag;
  }

  const uint ENTITIES_MAX = 1024;
  uint numEntities = 0;
  Entity entities[ENTITIES_MAX];
  uint numReferences = 0;
  Entity* referer[ENTITIES_MAX];
  Entity* references[ENTITIES_MAX];

  Entity* addEntity() {
    SDL_assert(numEntities < ENTITIES_MAX);
    // TODO: should we reset this?
    entities[numEntities] = {};
    referer[numEntities] = NULL;
    return entities + numEntities++;
  };
  Entity** addReference(Entity* target) {
    references[numReferences] = target;
    target->_referer = references + numReferences;
    return references + numReferences++;
  };
  void removeEntity(Entity* e) {
    SDL_assert(numEntities > 0);
    // swap in the last entity
    *e = entities[numEntities-1];
    // update reference position
    *e->_referer = e;

    --numEntities;
  };

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


  void print(glm::mat4 in) {
    const float* mat = glm::value_ptr(in);
    for (uint i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        printf("%f ", mat[i*4 + j]);
      }
      printf("\n");
    }
  }
  void print(const char* str, Rect r) {
    printf("%s: %f %f %f %f\n", str, r.x, r.y, r.w, r.h);
  }
  void print(const char* str, glm::vec2 v) {
    printf("%s: %f %f\n", str, v.x, v.y);
  }
};

int main(int, const char*[]) {
  srand(time(0));
  SDL_Window* window = initSubSystems();

  SDL_assert(sizeof(Sprite) == 8*sizeof(GLfloat));

  // init player
  Entity* player;
  {
    const float w = 0.1, h = 0.1;
    player = addEntity();
    player->pos = {};
    // TODO: get spritesheet info from external source
    player->sprite = addSprite({rectAround(player->pos, w, h), 11, 7, 70, 88});
    player->type = EntityType::player;
    set(player, EntityFlag_Collides);
    player->hitBox = rectAround({}, w, h);
  }

  // init fairies
  {
    for (int i = 0; i < 10; ++i) {
      Entity* fairy = addEntity();
      fairy->sprite = addSprite({0, 0, 0.1, 0.1, 585, 0, 94, 105});
      fairy->follow = addReference(player);
      fairy->type = EntityType::fairy;
    }
  }

  // init level
  {
    Rect walls[] = {
      {-1, -1, 2, 0.1},
      {-1, -1, 0.1, 2},
      {-1, 0.9, 2, 0.1},
      {0.9, -1, 0.1, 2},
    };
    for (uint i = 0; i < arrsize(walls); ++i) {
      Entity* wall = addEntity();
      wall->pos = center(walls[i]);
      wall->sprite = addSprite({walls[i], 0, 282, 142, 142});
      set(wall, EntityFlag_Collides);
      wall->hitBox = rectAround({}, walls[i].w, walls[i].h);
      wall->type = EntityType::wall;
      print("hitbox", wall->hitBox);
      print("pos", wall->pos);
      putchar('\n');
    }
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

    // update entities
    for (int i = 0; i < numEntities; ++i) {
      Entity* e = entities + i;
      switch (e->type) {
        case EntityType::player: {
          const float ACCELLERATION = 6;
          const float GRAVITY = 6;
          const float JUMPPOWER = 10;
          const float DRAG = 0.90;

          // input movement
          e->vel.x += ACCELLERATION*dt*isDown[KEY_RIGHT] - ACCELLERATION*dt*isDown[KEY_LEFT];
          // e->vel.y += ACCELLERATION*dt*isDown[KEY_UP] - ACCELLERATION*dt*isDown[KEY_DOWN];
          /*
          if (wasPressed[KEY_A] && numWalls > 0) {
            // get closest wall and move towards it
            glm::vec2 p = center(*walls);
            float minD = glm::distance(e->pos, p);
            for (int i = 1; i < numWalls; ++i) {
              auto d = glm::distance(e->pos, center(walls[i]));
              if (d < minD) {
                minD = d;
                p = center(walls[i]);
              }
            }
            const float FORCE = 3;
            e->vel = glm::normalize(p - e->pos) * FORCE;
          }
          */
          if (wasPressed[KEY_UP]) {
            e->vel.y = JUMPPOWER;
          }

          // physics movement
          e->vel *= DRAG;
          e->vel.y -= GRAVITY * dt;
        } break;
        case EntityType::fairy: {
          const float ACCELLERATION = 3.0f + frand(-2, 2);
          const float DRAG = 0.97;
          const float NOISE = 0.2;
          Entity* target = *e->follow;
          glm::vec2 diff = target->pos - e->pos + v2rand(NOISE);
          e->vel += ACCELLERATION * dt * glm::normalize(diff);
          e->vel *= DRAG;
        } break;
        case EntityType::wall: {
        } break;
      };
      // collision
      if (isset(e, EntityFlag_Collides))
      {
        // find the possible collisions for the move vector
        glm::vec2 oldPos = e->pos;
        glm::vec2 newPos = oldPos + (e->vel * dt);
        glm::vec2 move = newPos - oldPos;
        Rect moveBox = {
          glm::min(oldPos.x, newPos.x),
          glm::min(oldPos.y, newPos.y),
          glm::abs(move.x),
          glm::abs(move.y),
        };
        moveBox = pad(moveBox, e->hitBox);
        uint numHits = 0;
        Entity** hits = NULL;
        for (uint j = 0; j < numEntities; ++j) {
          if (i == j) continue;
          Entity* target = entities+j;
          Rect padded = pad(moveBox, target->hitBox);
          if (target == entities+numEntities-1 && e == entities) {
            printf("%u %u\n", e->type, entities[j].type);
            print("moveBox", moveBox);
            print("padded", padded);
            print("hitBox", e->hitBox);
            print("pos", target->pos);
            printf("%u %u\n", isset(target, EntityFlag_Collides), checkCollision(padded, target->pos));
          }
          if (isset(target, EntityFlag_Collides) && checkCollision(padded, target->pos)) {
            ++numHits;
            Entity** ptr = (Entity**) push(Entity*);
            *ptr = target;
            if (!hits) {
              hits = ptr;
            }
          }
        }
        if (numHits > 1) {
          printf("numHits: %u\n", numHits);
        }

        bool hit;
        do {
          float t = 1.0f;
          glm::vec2 endPoint = oldPos + move;
          hit = false;
          glm::vec2 closestWall;
          for (uint j = 0; j < numHits; ++j) {
            // find the shortest T for collision
            // TODO: Optimize: If we only use horizontal and vertical lines, we can optimize. Also precalculate corners, we do it twice now
            Rect padded = pad(hits[j]->pos + hits[j]->hitBox, e->hitBox);
            /*
            print("padded rect", padded);
            print("old pos", oldPos);
            print("new pos", endPoint);
            print("move", move);
            */
            // TODO: Maybe we can have all sides of a polygon here instead?
            Line lines[4] = {
              Line{bottomLeft(padded), bottomRight(padded)},
              Line{bottomRight(padded), topRight(padded)},
              Line{topRight(padded), topLeft(padded)},
              Line{topLeft(padded), bottomLeft(padded)},
            };
            for (uint l = 0; l < arrsize(lines); ++l) {
              RayCollisionAnswer r = findRayCollision(Line{oldPos, endPoint}, lines[l]);
              if (r.hit && r.t < t) {
                const float epsilon = 0.001;
                t = glm::max(0.0f, r.t-epsilon);
                hit = true;
                closestWall = line2vector(lines[l]);
              }
            }
            SDL_assert(t >= 0);
          }
          if (hit) {
            auto glide = dot(move, closestWall)*(1.0f-t) * closestWall / lengthSqr(closestWall);
            // move up to wall
            move *= glm::max(0.0f, t);
            // and add glide
            move += glide;
            // go on to recalculate wall collisions, this time with the new move
            printf("Hit!\n");
            printf("%u %u\n", (uint) e->type, (uint) entities[i].type);
            print("moveBox", moveBox);
            print("hitBox", e->hitBox);
            print("glide", glide);
            print("move", move);
            printf("t: %f\n", t);
          }
        } while (hit);
        popArr(Entity*, numHits);
        SDL_assert(!hits || getCurrStackPos() == (void*) hits);
        e->vel = move/dt;
      }
      glm::vec2 move = e->vel * dt;
      const float moveEpsilon = 0.001;
      if (glm::abs(move.x) > moveEpsilon) {
        e->pos.x += move.x;
        e->sprite->screen.x += move.x;
      }
      if (glm::abs(move.y) > moveEpsilon) {
        e->pos.y += move.y;
        e->sprite->screen.y += move.y;
      }
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
