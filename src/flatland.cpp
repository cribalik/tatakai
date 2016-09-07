#include <GL/glew.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <cstdlib>
#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_RECT_PACK_IMPLEMENTATION 1
#include <stb_rect_pack.h>

// #define DEBUG
#define DEBUG_FONT

typedef unsigned char Byte;

void _glOkOrDie(const char* file, int line) {
  GLenum errorCode = glGetError();
  if (errorCode != GL_NO_ERROR) {
    const char* error;
    switch (errorCode)
    {
      case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
      case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
      case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
      case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
      case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
      case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
      case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
      default: error = "unknown error";
    };
    printf("GL error at %s:%u. Code: %u - %s\n", file, line, errorCode, error);
    exit(1);
  }
}

#ifdef DEBUG
  #define DEBUGLOG(...) printf(__VA_ARGS__)
#else
  #define DEBUGLOG(...)
#endif

#if RELEASE
  #define glOKORDIE
#else
  #define glOKORDIE _glOkOrDie(__FILE__, __LINE__)
#endif

#define FOR(it, start) for (auto it = start; it; next(&it))
#define arrsize(arr) (sizeof((arr))/sizeof(*(arr)))
#define swap(a,b) {auto tmp = *a; *a = *b; *b = tmp;}

typedef unsigned int Uint;
typedef unsigned char Byte;
#define MegaBytes(x) ((x)*1024LL*1024LL)
#define local_persist static
#define unimplemented printf("unimplemented method %s:%u\n", __FILE__, __LINE__)


namespace {

  #include "shaders.cpp"

  using glm::vec2;
  using glm::vec3;

  // constants
  float GRAVITY = 12;

  // Init
  int screen_w = 800;
  int screen_h = 600;
  FT_Library ft;
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (FT_Init_FreeType(&ft)) {
      puts("Failed to init freetype\n");
      exit(1);
    }

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
  uint currMilliseconds = 0;
  inline uint getMilliseconds() {
    return currMilliseconds;
  }

  // Math
  struct Color {GLfloat r,g,b,a;};
  #define MAX(a, b) ((a) < (b) ? (b) : (a))
  inline Uint log2(Uint x) {
    Uint result = 0;
    while (x >>= 1) {
      ++result;
    }
    return result;
  }
  inline float frand(float upper) {
    return float(rand())/float(RAND_MAX)*upper;
  }
  inline float frand(float lower, float upper) {
    return float(rand())/float(RAND_MAX)*(upper-lower) + lower;
  }
  inline float frand() {
    return float(rand())/float(RAND_MAX);
  }
  inline vec2 v2rand(float size) {
    return vec2(frand(-size, size), frand(-size, size));
  }
  inline float cross(vec2 a, vec2 b) {
    return a.x*b.y - a.y*b.x;
  }
  inline float lengthSqr(vec2 x) {
    return x.x*x.x + x.y*x.y;
  }
  inline Uint32 nextPowerOfTwo(Uint32 x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
  }
  #define multipleOf(a, b) (!((a)%(b)))
  inline uint hammondWeight(uint x) {
    uint result = 0;
    while (x) {
      result += x&1;
      x >>= 1;
    }
    return result;
  }
  inline bool isPowerOfTwo(Uint64 x) {
    return hammondWeight(x) == 1;
  }

  // Physics
  struct Rect {
    float x,y,w,h;
  };
  struct Recti {
    int x,y,w,h;
  };
  inline Rect operator+(Rect r, vec2 v) {
    return Rect{r.x + v.x, r.y + v.y, r.w, r.h};
  }
  inline Rect operator+(vec2 v, Rect r) {
    return r + v;
  }
  inline Rect operator*(Rect r, float m) {
    return Rect{r.x, r.y, r.w * m, r.h * m};
  }
  inline Rect rectAround(vec2 pos, float w, float h) {
    return {pos.x - w/2, pos.y - h/2, w, h};
  }
  inline vec2 size(Rect x) {
    return {x.w,x.h};
  }
  inline vec2 center(Rect x) {
    return {x.x + x.w/2, x.y + x.h/2};
  }
  inline Rect pad(Rect a, Rect b) {
    a.x -= b.w+b.x;
    a.y -= b.h+b.y;
    a.w += b.w;
    a.h += b.h;
    return a;
  }
  inline vec2 bottomLeft(Rect x) {
    return {x.x, x.y};
  }
  inline vec2 topLeft(Rect x) {
    return {x.x, x.y+x.h};
  }
  inline vec2 topRight(Rect x) {
    return {x.x+x.w, x.y+x.h};
  }
  inline vec2 bottomRight(Rect x) {
    return {x.x+x.w, x.y};
  }
  inline bool collision(Rect a, Rect b) {
    return !(a.y > (b.y+b.h) || (a.y+a.h) < b.y || a.x > (b.x+b.w) || (a.x+a.w) < b.x);
  }
  inline bool collision(Rect r, vec2 p) {
    return !(p.x < r.x || p.x > (r.x + r.w) || p.y > (r.y + r.h) || p.y < r.y);
  }
  inline bool collision(vec2 p, Rect r) {
    return collision(r, p);
  }
  struct Line {vec2 a,b;};
  inline vec2 line2vector(Line l) {
    return l.b - l.a;
  }
  struct RayCollisionAnswer {
    bool hit;
    float t;
  };
  RayCollisionAnswer rayCollision(Line ray, Line target) {
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
  vec2 viewPosition = {};

  // Sprites
  // OpenGL layout
  struct Character;
  struct Sprite {
    union {
      struct {
        Rect screen, tex;
      };
      struct {
        Rect _; // texture
        struct { // Allocator information for texts
          Character* next;
          uint size;
        };
      };
      struct {
        GLfloat x,y,w,h, tx,ty,tw,th;
      };
    };
  };
  inline Sprite operator+(Sprite s, vec2 d) {
    return Sprite{s.screen + d, s.tex};
  }
  inline Rect invalidSpritePos() {
    return {-100, -100};
  }

  // Text
  // Using general allocator for text strings, we wont be using crazy amounts of text so it's fine
  // TODO: generate closest-point texture to save space and enable shadows and glows and stuff
  // TODO: if we run out of memory, maybe the newest text should get priority, and the old references should be invalidated?
  struct Glyph {
    Rect dim;
    float advance;
    Rect tex;
  };
  Glyph glyphs[256];
  struct Character {
    Sprite sprite;
    Color color;
  };
  const Uint CHARACTERS_MAX = 1024;
  bool textDirty = false;
  Character textSprites[CHARACTERS_MAX];
  Character* freeChars;
  Character* textSpritesMax = textSprites;
  struct TextRef {
    // TODO: squish together these bits?
    Character* block;
    uint size;
  };
  inline bool isnull(TextRef ref) {return !ref.block;}
  // text can only have one parent, multiple frees are not supported
  TextRef addText(const char* text, vec2 pos, float scale, Color color, bool center = false) {
    TextRef result = {};
    SDL_assert(text);
    if (!text) return result;

    uint len = strlen(text);
    // find large enough block (first fit)
    // TODO: best fit?
    Character** p = &freeChars;
    while (*p && (*p)->sprite.size < len) {
      p = &(*p)->sprite.next;
    }
    if (*p) {
      Character* block = *p;

      // set dirty flag
      textDirty = true;

      // free space
      if (len < (*p)->sprite.size) {
        // add the space that is left over
        Character* newBucket = *p + len;
        newBucket->sprite.size = (*p)->sprite.size - len;
        newBucket->sprite.next = (*p)->sprite.next;
        *p = newBucket;
      } else {
        // skip to next block
        *p = (*p)->sprite.next;
      }

      #ifdef DEBUG
        printf("Allocating text block at %li of size %u", block - textSprites, len);
      #endif

      // align to center
      if (center) {
        float width = 0.0f;
        for (const char* c = text; *c; ++c) {
          width += glyphs[(int) *c].advance * scale;
        }
        pos.x -= width/2;
      }

      // fill the sprites
      Character* bucket = block;
      for (const char* c = text; *c; ++c, ++bucket) {
        uint i = *c;
        *bucket = Character{
          Rect{
            pos.x + glyphs[i].dim.x * scale,
            pos.y + glyphs[i].dim.y * scale,
            glyphs[i].dim.w * scale,
            glyphs[i].dim.h * scale,
          },
          glyphs[i].tex,
          color,
        };
        pos.x += glyphs[i].advance * scale;
      }

      // did this block increase the max?
      textSpritesMax = MAX(textSpritesMax, block+len);
      #ifdef DEBUG
        printf(" end is %li\n", textSpritesMax - textSprites);
      #endif
      result = {block, len};
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Failed to allocate text block");
    }
    return result;
  }
  void remove(TextRef ref) {
    if (!isnull(ref)) {
      #ifdef DEBUG
        printf("Removed text block at %li of size %u", ref.block - textSprites, ref.size);
      #endif
      // hide the removed sprites
      for (uint i = 0; i < ref.size; ++i) {
        ref.block[i].sprite.screen = invalidSpritePos();
      }
      // free block
      // we want the freelist to be ordered, find the free block before this block
      ref.block->sprite.size = ref.size;
      ref.block->sprite.next = 0;
      Character* leftFree = ref.block;
      if (!freeChars) {
        freeChars = ref.block;
      } else if (freeChars > ref.block) {
        if (freeChars == ref.block + ref.size) {
          // merge
          ref.block->sprite.size += freeChars->sprite.size;
        }
        else {
          // append
          ref.block->sprite.next = freeChars;
        }
        freeChars = ref.block;
      } else {
        // Find the block before the freed block
        Character* l = freeChars;
        Character* m = ref.block;
        while (l->sprite.next && l->sprite.next < m) {
          l = l->sprite.next;
        }
        Character* r = l->sprite.next;
        // block is to the left, block->next is to the right of the freed block
        l->sprite.next = m;
        m->sprite.next = r;
        SDL_assert((l < m) && (!r || m < r));
        // merge right
        if (r && m + m->sprite.size == r) {
          m->sprite.size += r->sprite.size;
          m->sprite.next = r->sprite.next;
        }
        // merge left
        if (l + l->sprite.size == m) {
          l->sprite.size += m->sprite.size;
          l->sprite.next = m->sprite.next;
          leftFree = l;
        }
      }

      // update max
      if (textSpritesMax == ref.block+ref.size) {
        textSpritesMax = leftFree;
      } else {
        // if we popped data, then we don't have to mark as dirty, we only draw upwards
        textDirty = true;
      }
      #ifdef DEBUG
        printf(" end is %li\n", textSpritesMax - textSprites);
      #endif
    }
  }
  GLuint loadFont(const char* filename, uint height);
  GLuint initText() {
    GLuint result = loadFont("assets/monaco.ttf", 48);
    for (uint i = 0; i < CHARACTERS_MAX; ++i) {
      textSprites[i].sprite.screen = invalidSpritePos();
    }
    freeChars = textSprites;
    freeChars->sprite.size = CHARACTERS_MAX;
    freeChars->sprite.next = 0;
    return result;
  }

  // Sprites
  const Uint SPRITES_MAX = 8192;
  Uint numSprites = 0;
  Sprite sprites[SPRITES_MAX];
  union SpriteRefSlot {
    Sprite* sprite;
    SpriteRefSlot* next;
  };
  typedef SpriteRefSlot* SpriteRef;
  SpriteRefSlot* freeSpriteRefs = 0;
  uint numSpriteRefs = 0;
  SpriteRefSlot spriteReferences[SPRITES_MAX]; // TODO: use hashmap instead?
  SpriteRefSlot* spriteReferenceLocation[SPRITES_MAX];
  SpriteRef addSprite(Sprite s) {
    SDL_assert(numSprites < SPRITES_MAX);
    SpriteRef result = 0;
    if (numSprites < SPRITES_MAX) {
      Sprite* sprite = sprites + numSprites;

      // create a reference to the new sprite
      SpriteRefSlot* slot = 0;

      if (freeSpriteRefs) {
        slot = freeSpriteRefs;
        freeSpriteRefs = freeSpriteRefs->next;
      } else if (numSpriteRefs < SPRITES_MAX) {
        slot = spriteReferences + numSpriteRefs++;
      } else {
        DEBUGLOG("SpriteRef buffer is full\n");
      }

      if (slot) {
        *sprite = s;
        slot->sprite = sprite;
        spriteReferenceLocation[numSprites] = slot;
        ++numSprites;

        result = slot;
      }
    } else {
      DEBUGLOG("Sprite buffer is full\n");
    }
    return result;
  }
  inline void free(SpriteRef ref) { // invalidates references to this sprite
    SDL_assert(ref->sprite >= sprites && ref->sprite < sprites + SPRITES_MAX);
    // swap sprite data with the last one
    uint index = ref->sprite - sprites;
    sprites[index] = sprites[--numSprites];
    // redirect the reference for the last one
    SpriteRefSlot* slot = spriteReferenceLocation[numSprites];
    slot->sprite = ref->sprite;
    // remember the reference position for the last one
    spriteReferenceLocation[index] = spriteReferenceLocation[numSprites];
    // and add the reference to the freelist
    ref->next = freeSpriteRefs;
    freeSpriteRefs = ref;
  }
  inline Sprite* get(SpriteRef ref) {
    SDL_assert(ref->sprite >= sprites && ref->sprite < sprites + SPRITES_MAX);
    Sprite* result = 0;
    if (ref) {
      result = ref->sprite;
    }
    return result;
  }

  // Colors
  vec3 FIRE = {226/265.0f, 120/265.0f, 34/265.0f};

  // Entities
  enum EntityType : Uint32 {
    EntityType_Player,
    EntityType_Fairy,
    EntityType_Wall,
    EntityType_Max,
  };
  enum EntityFlag : Uint32 {
    EntityFlag_NoCollide = 1 << 0,
    EntityFlag_Removed = 1 << 1,
    EntityFlag_Gravity = 1 << 2,
  };
  struct Entity;
  struct EntityRef {
    Uint id;
    Entity* entity;
  };
  struct Entity {
    Uint id; //TODO: should this be 64 bit to minimize odds of a reference to a wraparound id?
    Uint timestamp;
    EntityType type;
    EntityFlag flags;

    vec2 pos;
    vec2 vel;
    // SpriteRef sprite;
    float width, height;
    Rect hitBox;

    // familiar stuff
    EntityRef follow;

    // player stuff
    char direction; // -1 left, 1 right
    Uint health;
  };
  inline bool isset(Entity* e, EntityFlag flag) {
    return e->flags & flag;
  }
  inline void set(Entity* e, EntityFlag flag) {
    e->flags = (EntityFlag) (e->flags | flag);
  }
  inline void unset(Entity* e, EntityFlag flag) {
    e->flags = (EntityFlag) (~flag & e->flags);
  }
  struct EntitySlot {
    bool used;
    union {
      Entity entity;
      EntitySlot* next;
    };
  };
  const Uint ENTITIES_MAX = 8192;
  Uint numEntities = 0;
  EntitySlot* freeEntities = 0;
  Uint entityId = 0;
  // FIXME: Se use only an entity id to check if references are outdated. This can wrap around, allowing for errors :(
  EntitySlot entities[ENTITIES_MAX] = {};
  Entity* addEntity() {
    EntitySlot* slot = 0;
    if (freeEntities) {
      slot = freeEntities;
      // FIXME: segfault here, freeEntities not pointing within array
      freeEntities = freeEntities->next;
    } else if (numEntities < ENTITIES_MAX) {
      slot = entities + numEntities++;
    }
    SDL_assert(slot);

    Entity* result = 0;
    if (slot) {
      slot->used = true;
      slot->entity = {};
      slot->entity.id = entityId++;
      slot->entity.timestamp = getMilliseconds();
      result = &slot->entity;
    }
    return result;
  };
  inline EntityRef createRef(Entity* target) {
    return {target->id, target};
  };
  inline Entity* get(EntityRef ref) {
    Entity* result = 0;
    if (!ref.entity) {
      result = 0;
    }
    else if (ref.id == ref.entity->id) {
      result = ref.entity;
    }
    return result;
  };
  inline EntitySlot* slotOf(Entity* e) {
    return (EntitySlot*) (((Byte*) e) - offsetof(EntitySlot, entity));
  }
  // safely flags for removal at end of frame
  inline void remove(Entity* e) {
    SDL_assert(numEntities > 0);
    set(e, EntityFlag_Removed);
  };
  // invalidates all references to this entity
  inline void free(Entity* e) {
    SDL_assert(slotOf(e) >= entities && slotOf(e) < entities+ENTITIES_MAX);
    if (e) {
      EntitySlot* slot = slotOf(e);
      slot->used = false;
      slot->next = freeEntities;
      freeEntities = slot;
    }
  };
  // should warn us about double frees separated by frame. (The same frame is OK)
  inline void remove(EntityRef ref) {
    SDL_assert(ref.id == ref.entity->id);
    if (ref.id == ref.entity->id) {
      remove(ref.entity);
    }
  };
  // Iterate entities
  typedef EntitySlot* EntityIter;
  void next(EntityIter* iter) {
    SDL_assert((*iter) >= entities && (*iter) < entities+ENTITIES_MAX);
    do {
      ++(*iter);
    } while (!(*iter)->used && (*iter) != entities+ENTITIES_MAX);
    if ((*iter) == entities + ENTITIES_MAX) {
      (*iter) = 0;
    }
  }
  EntityIter iterEntities() {
    EntitySlot* iter = entities;
    while (!iter->used && iter != entities+ENTITIES_MAX) ++iter;
    if (iter == entities+ENTITIES_MAX) iter = 0;
    return iter;
  }
  Entity* get(EntityIter iter) {
    SDL_assert(iter->used);
    return &iter->entity;
  }

  // memory
  struct Block32 {
    union {
      Uint32 uints[8];
    };
    Block32* next;
    const static uint NUM_UINTS = arrsize(uints);
  };
  const Uint STACK_MAX = MegaBytes(128);
  Byte stack[STACK_MAX];
  Byte* stackCurr = stack;
  inline void* _push(Uint size) {
    SDL_assert(stackCurr + size < stack + STACK_MAX);
    stackCurr += size;
    return stackCurr-size;
  }
  inline void _pop(Uint size) {
    SDL_assert(stackCurr - size >= stack);
    stackCurr -= size;
  }
  #define push(type) (type*) _push(sizeof(type))
  #define pushArr(type, count) (type*) _push(sizeof(type)*count)
  #define pop(type) _pop(sizeof(type))
  #define popArr(type, count) _pop(sizeof(type)*count)
  int pushStr(const char* fmt, ...) {
    va_list(args);
    va_start(args, fmt);
    int n = vsprintf((char*) stackCurr, fmt, args);
    pushArr(char, n);
    return n;
  }
  #define popStr _pop

  // openGL
  GLuint compileShader(const char* vertexShaderSrc, const char* geometryShaderSrc, const char* fragmentShaderSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, 0);
    glCompileShader(vertexShader);
    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(vertexShader, 512, 0, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n", infoLog);
      }
    }

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSrc, 0);
    glCompileShader(geometryShader);
    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(geometryShader, 512, 0, infoLog);
        printf("ERROR::SHADER::GEOMETRY::COMPILATION_FAILED: %s\n", infoLog);
      }
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, 0);
    glCompileShader(fragmentShader);
    {
      GLint success;
      GLchar infoLog[512];
      glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, 0, infoLog);
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
        glGetProgramInfoLog(shaderProgram, 512, 0, infoLog);
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
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glOKORDIE;

    int w,h;
    Byte* image = SOIL_load_image(filename, &w, &h, 0, SOIL_LOAD_RGBA);
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

  // Fonts
  const char START_CHAR = 32;
  const char END_CHAR = 127;
  GLuint loadFont(const char* filename, uint height) {

    // load font file
    FT_Face face;
    if (FT_New_Face(ft, filename, 0, &face)) {
      printf("Failed to load font at %s\n", filename);
      exit(1);
    }
    FT_Set_Pixel_Sizes(face, 0, height);

    // pack glyphs
    const int pad = 2;
    stbrp_rect rects[END_CHAR-START_CHAR];
    for (uint i = START_CHAR; i < END_CHAR; ++i) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        printf("failed to load char %c\n", i);
        continue;
      }
      Uint id = i-START_CHAR;
      rects[id].id = id;
      rects[id].w = face->glyph->bitmap.width + 2*pad; // pad with 2 pixels on each side
      rects[id].h = face->glyph->bitmap.rows + 2*pad;
    }
    stbrp_context c;
    stbrp_init_target(&c, 1024, 1024, (stbrp_node*) stackCurr, 1024);
    stbrp_pack_rects(&c, rects, arrsize(rects));

    // get the max width and height
    Uint maxW = 0, maxH = 0;
    for (stbrp_rect* r = rects; r < rects+arrsize(rects); ++r) {
      SDL_assert(r->was_packed);
      if (!r->was_packed) {
        puts("Failed to pack font");
      }
      maxW = glm::max((uint) r->x + r->w, maxW);
      maxH = glm::max((uint) r->y + r->h, maxW);
    }
    maxW = nextPowerOfTwo(maxW);
    maxH = nextPowerOfTwo(maxH);
    printf("Font texture dimensions: %u %u\n", maxW, maxH);
    maxW = 1024;
    maxH = 1024;

    // check if opengl support that size of texture
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    SDL_assert((Uint) maxTextureSize >= maxW && (Uint) maxTextureSize >= maxH);
    printf("%u %u %u\n", maxTextureSize, maxW, maxH);

    // create texture
    glActiveTexture(GL_TEXTURE0);
    glOKORDIE;
    GLuint tex;
    glGenTextures(1, &tex);
    glOKORDIE;
    glBindTexture(GL_TEXTURE_2D, tex);
    glOKORDIE;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glOKORDIE;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, maxW, maxH, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    glOKORDIE;

    // load glyphs into texture
    for (char i = START_CHAR; i < END_CHAR; ++i) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        DEBUGLOG("failed to load char %c at %u\n", i, __LINE__);
        continue;
      }

      #ifdef DEBUG_FONT
        unsigned char* c = face->glyph->bitmap.buffer;
        for (uint i = 0; i < face->glyph->bitmap.width; ++i) {
          auto p = c;
          for (uint j = 0; j < face->glyph->bitmap.rows; ++j) {
            ++p;
          }
          c += face->glyph->bitmap.pitch;
        }
      #endif

      stbrp_rect* rect = rects + i-START_CHAR;
      SDL_assert(rect->x + rect->w <= maxW && rect->y + rect->h <= maxH);
      SDL_assert(rect->id == (int) (i-START_CHAR));
      SDL_assert((rect->w - 2*pad) * (rect->h - 2*pad) == (int) (face->glyph->bitmap.rows * face->glyph->bitmap.width));
      glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x+pad, rect->y+pad, rect->w - 2*pad, rect->h - 2*pad, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
      glOKORDIE;

      // save glyph offsets
      int w = face->glyph->bitmap.width;
      int h = face->glyph->bitmap.rows;
      float fheight = height;
      #ifdef DEBUG_FONT
      printf("%c: bx: %i by: %i h: %i w: %i advx: %li advy: %li\n", i, face->glyph->bitmap_left, face->glyph->bitmap_top, h, w, face->glyph->advance.x, face->glyph->advance.y);
      #endif
      glyphs[(int) i] = Glyph{
        Rect{
          face->glyph->bitmap_left/fheight,
          (face->glyph->bitmap_top - h)/fheight,
          w/fheight,
          h/fheight
        },
        float(face->glyph->advance.x)/fheight/64.0f,
        Rect{(float) rect->x, (float) rect->y, (float) rect->w, (float) rect->h},
      };
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return tex;
  }

  // Prints
  void print(glm::mat4 in) {
    const float* mat = glm::value_ptr(in);
    for (Uint i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        printf("%f ", mat[i*4 + j]);
      }
      printf("\n");
    }
  }
  void print(const char* str, Rect r) {
    printf("%s: %f %f %f %f\n", str, r.x, r.y, r.w, r.h);
  }
  void print(const char* str, vec2 v) {
    printf("%s: %f %f\n", str, v.x, v.y);
  }
  void print(const char* str, Sprite s) {
    printf("%s: ", str);
    print("", s.screen);
    print(" - ", s.tex);
  }

  // Levels
  // basically loads in a bunch of walls
  void loadLevel(uint i) {
    char* filename = (char*) stackCurr;
    int n = pushStr("assets/%u.lvl", i);
    DEBUGLOG("Opening level file %s", filename);
    auto f = fopen(filename, "r");
    if (f) {
      while (1) {
        Rect r;
        int res = fscanf(f, "%f%f%f%f", &r.x,&r.y,&r.w,&r.h);
        if (res && res != EOF) {
          Entity* e = addEntity();
          if (e) {
            e->pos = center(r);
            e->type = EntityType_Wall;
            e->hitBox = rectAround({}, r.w, r.h);
          } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate entity while loading level");
          }
        } else {
          break;
        }
      }
      fclose(f);
    } else {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to open level file \"%s\"\n", filename);
    }
    popStr(n);
  }

  // Gameplay
  // Check if a and b has types at and bt, and swaps a and b if needed so that a->type == at and b->type == bt
  inline bool checkTypeAndSwap(Entity** a, Entity** b, EntityType at, EntityType bt) {
    SDL_assert((*a)->type <= (*b)->type);
    if (at > bt) {
      swap(a,b);
    }
    bool result = (*a)->type == at && (*b)->type == bt;
    return result;
  }
  inline void orderByType(Entity** a, Entity** b) {
    if ((*a)->type > (*b)->type) {
      swap(a,b);
    }
  }
  bool processCollision(Entity* a, Entity* b, vec2 start, vec2 end, bool entered) {
    bool result = false;
    orderByType(&a, &b);

    if (a->type == EntityType_Wall || b->type == EntityType_Wall) {
      DEBUGLOG("Entered wall: %u", entered);
      result = entered;
    }

    else if (checkTypeAndSwap(&a, &b, EntityType_Player, EntityType_Fairy)) {
      // a is now player, b is fairy
      result = true; // always pass through
      if (!entered) {
        a->health += b->health;
        // remove(b);
      }
    }
    else if (checkTypeAndSwap(&a, &b, EntityType_Fairy, EntityType_Player)) {
      printf("WHAAAT");
    }

    return result;
  }

  bool tests() {
    for (Uint i = 0; i < 10; ++i) {
      SDL_assert(log2(1 << i) == i);
    }
    struct X {Rect a,b;};
    SDL_assert(sizeof(X) == sizeof(Sprite));
    SDL_assert(sizeof(Block32) == 32 + sizeof(void*));
    return true;
  }

  // visual models
  struct SpriteAnim {
    Rect modelTransform;
    Recti texOrig;
    Block32* delay;
    uint numFrames;
    uint cols;
  };
  enum ModelType {
    ModelType_Sprite,
    ModelType_Anim,
  };
  struct Model {
    ModelType type;
    union {
      Sprite sprite;
      SpriteAnim anim;
    };
    Model* next;
  };
  // TODO: extend models to be graphs of animation transitions
  Model* models[EntityType_Max] = {};
  void loadModel(EntityType type) {
    // TODO: get this stuff from file
    if (!models[type]) {
      Model* model = 0;
      switch (type) {

        case EntityType_Player: {

          model = push(Model);
          *model = {};
          model->type = ModelType_Anim;
          model->anim.modelTransform = rectAround({}, 0.1 ,0.1);
          model->anim.texOrig = Recti{11,7,70,88};
          model->anim.delay = push(Block32);
          model->anim.numFrames = 8;
          for (uint i = 0; i < model->anim.numFrames; ++i) {
            model->anim.delay->uints[i] = 100;
          }
          model->anim.cols = 4;

        } break;

        case EntityType_Fairy: {

          model = push(Model);
          *model = {};
          model->type = ModelType_Sprite;
          model->sprite = {rectAround({}, 0.1, 0.1), Rect{0, 282, 142, 142}};

        } break;

        case EntityType_Wall: {

          model = push(Model);
          *model = {};
          model->type = ModelType_Sprite;
          model->sprite = {Rect{}, Rect{0, 282, 142, 142}};

        } break;

        case EntityType_Max: break;
      }
      models[type] = model;
    }
  }
  Sprite getSprite(uint time , SpriteAnim anim) {
    // calculate sum of delays
    // TODO: cache this value?
    uint delaySum = 0;
    SDL_assert(isPowerOfTwo(Block32::NUM_UINTS));
    Block32* block = anim.delay;
    for (uint i = 0; i < anim.numFrames; ++i) {
      if (i&Block32::NUM_UINTS) block = block->next;
      delaySum += block->uints[i&(Block32::NUM_UINTS-1)];
    }

    // find the current frame
    uint dt = time % delaySum;
    block = anim.delay;
    uint i = 0;
    for (; i < anim.numFrames; ++i) {
      if (i&Block32::NUM_UINTS) block = block->next;
      if (dt < block->uints[i&(Block32::NUM_UINTS-1)]) {
        break;
      }
      dt -= block->uints[i&(Block32::NUM_UINTS-1)];
    }
    SDL_assert(i < anim.numFrames-1);

    Rect texPos = {
      anim.texOrig.x + float(anim.texOrig.w*(i%anim.cols)),
      anim.texOrig.y + float(anim.texOrig.h*(i/anim.cols)),
      (float) anim.texOrig.w,
      (float) anim.texOrig.h,
    };
    return Sprite{anim.modelTransform, texPos};
  }
};

int main(int, const char*[]) {
  #ifndef DEBUG
    srand(SDL_GetPerformanceCounter());
  #endif
  SDL_Window* window = initSubSystems();
  SDL_assert(tests());

  SDL_assert(sizeof(Sprite) == 8*sizeof(GLfloat));

  GLuint fontTexture = initText();

  // init player
  EntityRef player;
  {
    const float w = 0.1, h = 0.1;
    Entity* playerP = addEntity();
    playerP->pos = {};
    // TODO: get spritesheet info from external source
    // playerP->sprite = addSprite({rectAround(playerP->pos, w, h), Rect{11, 7, 70, 88}});
    playerP->type = EntityType_Player;
    playerP->hitBox = rectAround({}, w, h);
    playerP->health = 10;
    player = createRef(playerP);
  }

  loadModel(EntityType_Player);
  loadModel(EntityType_Fairy);
  loadModel(EntityType_Wall);
  // init level
  loadLevel(1);


  // init some text
  {
    addText("HejsanSvejsan", {0, 0}, 0.10, {0.5,0.7,0.2,1}, true);
    // textSprites[0] = {rectAround({0, 0}, 2, 2), glyphs['m'].tex};
    // textSprites[1] = {rectAround({0, 0}, 2, 2), Rect{0, 0, 1024, 1024}};
  }

  // create sprite buffer
  GLuint spritesVAO;
  GLuint spritesVBO;
  {
    glGenVertexArrays(1, &spritesVAO);
    glBindVertexArray(spritesVAO);
    glGenBuffers(1, &spritesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, spritesVBO);
    glBufferData(GL_ARRAY_BUFFER, SPRITES_MAX*sizeof(Sprite), 0, GL_DYNAMIC_DRAW);
    glOKORDIE;

    const auto stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*) (4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glOKORDIE;
  }

  // create text buffer
  GLuint textVAO;
  GLuint textVBO;
  {
    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);
    glGenBuffers(1, &textVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, CHARACTERS_MAX*sizeof(Sprite), 0, GL_DYNAMIC_DRAW);
    glOKORDIE;

    const auto stride = 12 * sizeof(GLfloat);
    SDL_assert(sizeof(Character) == stride);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*) (4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*) (8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glOKORDIE;
  }

  // compile shaders and set static uniforms
  GLuint spriteShader;
  GLint viewLocation;
  GLuint spriteSheet;
  GLuint textShader;
  {
    spriteShader = compileShader(sprite_vertex_shader_src, sprite_geometry_shader_src, sprite_fragment_shader_src);
    glUseProgram(spriteShader);
    glOKORDIE;
    // bind the spritesheet
    spriteSheet = loadTexture("assets/spritesheet.png");
    glOKORDIE;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, spriteSheet);
    glUniform1i(glGetUniformLocation(spriteShader, "uTexture"), 0);
    glOKORDIE;
    // get locations
    viewLocation = glGetUniformLocation(spriteShader, "uView");
    GLuint ambientLightLocation = glGetUniformLocation(spriteShader, "uAmbientLight");
    glUniform3f(ambientLightLocation, 1, 1, 1);
    glOKORDIE;
    textShader = compileShader(text_vertex_shader_src, text_geometry_shader_src, text_fragment_shader_src);
  }

  // SDL_SetRelativeMouseMode(SDL_TRUE);
  // SDL_GetRelativeMouseState(0, 0);

  // main loop
  Timer loopTime = startTimer();
  int loopCount = 0;
  while (true)
  {
    currMilliseconds = SDL_GetTicks();
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
            if (event.key.keysym.sym == SDLK_ESCAPE) {
              exit(0);
            }
            for (int i = 0; i < KEY_MAX; ++i) {
              if (event.key.keysym.sym == keyCodes[i]) {

                wasPressed[i] = !isDown[i];
                isDown[i] = true;
                break;
                #ifdef DEBUG
                  if (wasPressed[i]) DEBUGLOG("A key was pressed\n");
                #endif
              }
            }
          } break;
          case SDL_KEYUP: {
            DEBUGLOG("A key was released\n");
            for (int i = 0; i < KEY_MAX; ++i) {
              if (event.key.keysym.sym == keyCodes[i]) {
                isDown[i] = false;
                break;
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
    FOR(iter, iterEntities()) {
      Entity* entity = get(iter);

      // gravity
      if (isset(entity, EntityFlag_Gravity)) {
        entity->vel.y -= 0.02 * GRAVITY * dt;
      }

      // per-type update
      switch (entity->type) {

        case EntityType_Player: {

          const float ACCELLERATION = 6;
          const float JUMPPOWER = 10;
          const float DRAG = 0.90;

          // input movement
          if (isDown[KEY_RIGHT]) {
            entity->vel += ACCELLERATION*dt;
            entity->direction = 1;
          } else if (isDown[KEY_LEFT]) {
            entity->vel -= ACCELLERATION*dt;
            entity->direction = -1;
          }

          if (wasPressed[KEY_UP]) {
            entity->vel.y = JUMPPOWER;
          }

          // physics movement
          entity->vel *= DRAG;
          entity->vel.y -= GRAVITY * dt;

          // shoot a fairy (so to speak)
          if (isDown[KEY_A]) {
            for (int i = 0; i < 10; ++i) {
              Entity* fairy = addEntity();
              printf("created fairy");
              if (fairy) {
                auto pos = entity->pos;
                fairy->pos = {pos.x, pos.y};
                fairy->follow = player;
                fairy->type = EntityType_Fairy;
                fairy->hitBox = rectAround({}, 0.05, 0.05);
                fairy->health = 5;
                fairy->vel = vec2(entity->vel.x + entity->direction * 2.0f, entity->vel.y + frand(-1, 1));
                set(fairy, EntityFlag_Gravity);
                set(fairy, EntityFlag_NoCollide);
              }
            }
          }

        } break;

        case EntityType_Fairy: {

          const float ACCELLERATION = 2.0f + frand(-2, 2);
          const float DRAG = 0.97;
          const float NOISE = 0.2;
          Entity* target = get(entity->follow);
          if (target) {
            vec2 diff = target->pos - entity->pos + v2rand(NOISE);
            entity->vel += ACCELLERATION * dt * glm::normalize(diff);
          }
          entity->vel *= DRAG;
        } break;

        case EntityType_Wall: {
        } break;

        case EntityType_Max: break;
      };

      // collision
      if (!isset(entity, EntityFlag_NoCollide)) {
        // find the possible collisions for the move vector

        // get the bounding box for the movement
        vec2 oldPos = entity->pos;
        vec2 newPos = oldPos + (entity->vel * dt);
        vec2 move = newPos - oldPos;
        Rect moveBox = {
          glm::min(oldPos.x, newPos.x),
          glm::min(oldPos.y, newPos.y),
          glm::abs(move.x),
          glm::abs(move.y),
        };
        moveBox = pad(moveBox, entity->hitBox);

        // find possible collisions
        Uint numHits = 0;
        Entity** hits = (Entity**) stackCurr;
        FOR(target_iter, iterEntities()) {
          if (iter == target_iter) continue;
          Entity* target = get(target_iter);
          Rect padded = pad(moveBox, target->hitBox);
          if (!isset(target, EntityFlag_NoCollide) && collision(padded, target->pos)) {
            ++numHits;
            auto ptr = (Entity**) push(Entity*);
            *ptr = target;
          }
        }

        // process the found boxes
        if (numHits) {
          for (int iters = 0; iters < 4; ++iters) {
            float t = 1.0f;
            vec2 endPoint = oldPos + move;

            Entity** hit = 0;
            vec2 closestWall;
            for (Entity** targetPtr = hits; targetPtr < hits+numHits; ++targetPtr) {
              Entity* target = *targetPtr;
              // find the shortest T for collision
              // TODO: Optimize: If we only use horizontal and vertical lines, we can optimize. Also precalculate corners, we do it twice now
              Rect padded = pad(target->pos + target->hitBox, entity->hitBox);
              // TODO: In the future, maybe we can have all sides of a polygon here instead?
              Line lines[4] = {
                Line{bottomLeft(padded), bottomRight(padded)},
                Line{bottomRight(padded), topRight(padded)},
                Line{topRight(padded), topLeft(padded)},
                Line{topLeft(padded), bottomLeft(padded)},
              };
              for (Uint l = 0; l < arrsize(lines); ++l) {
                RayCollisionAnswer r = rayCollision(Line{oldPos, endPoint}, lines[l]);
                if (r.hit && r.t < t) {
                  const float epsilon = 0.001;
                  t = glm::max(0.0f, r.t-epsilon);
                  hit = targetPtr;
                  closestWall = line2vector(lines[l]);
                }
              }
              SDL_assert(t >= 0);
            }

            if (hit) {
              Entity* target = *hit;
              bool passThrough = processCollision(entity, target, oldPos, endPoint, collision(target->hitBox + target->pos, entity->hitBox + entity->pos));
              if (!passThrough) {
                auto glide = dot(move, closestWall)*(1.0f-t) * closestWall / lengthSqr(closestWall);
                // move up to wall
                move *= glm::max(0.0f, t);
                // and add glide
                move += glide;
                // go on to recalculate wall collisions, this time with the new move
              } else {
                // We remove this from hittable things and continue towards other collision
                --iters;
                *hit = hits[--numHits];
                pop(Entity*);
              }
            }
            else {
              break;
            }

          };
          popArr(Entity*, numHits);
          SDL_assert(stackCurr == (void*) hits);
        }
        entity->vel = move/dt;
      }

      // and move according to collision
      vec2 move = entity->vel * dt;
      const float moveEpsilon = 0.001;
      if (glm::abs(move.x) > moveEpsilon) {
        entity->pos.x += move.x;
      }
      if (glm::abs(move.y) > moveEpsilon) {
        entity->pos.y += move.y;
      }
    }

    // draw entities
    Sprite* sprite = sprites;
    FOR(it, iterEntities()) {
      Entity* e = get(it);
      auto model = models[e->type];
      switch (e->type) {
        case EntityType_Player: {
          *sprite++ = getSprite(e->timestamp, model->anim) + e->pos;
        } break;
        case EntityType_Fairy: {
          *sprite++ = model->sprite + e->pos;
        } break;
        case EntityType_Wall: {
          *sprite++ = {e->hitBox + e->pos, model->sprite.tex};
        } break;
        case EntityType_Max: break;
      };
    }
    numSprites = sprite-sprites;

    // randomly add some text
    #ifdef DEBUG
      local_persist TextRef texts[512];
      local_persist uint numTexts = 0;
      if (multipleOf(rand(), 2) && numTexts < 512) {
        // add text
        char* text = (char*) stackCurr;
        int n = sprintf(text, "It's over %u!", rand());
        pushArr(int, n);
        texts[numTexts++] = addText(text, {frand(-1, 1), frand(-1, 1)}, 0.05, true);
        popArr(int, n);
      } else if (numTexts == 512 || (multipleOf(rand(), 70) && numTexts > 0)) {
        // remove text
        uint i = uint(rand())%numTexts;
        remove(texts[i]);
        texts[i] = texts[--numTexts];
      }
    #endif

    // clean up removed entities
    FOR(iter, iterEntities()) {
      SDL_assert(get(iter));
      if (isset(get(iter), EntityFlag_Removed)) {
        free(get(iter));
      }
    }

    // begin draw
    {
      glClearColor(0.1,0.1,0.1,1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    // draw sprites
    {
      glUseProgram(spriteShader);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, spriteSheet);

      glBindVertexArray(spritesVAO);
      glBindBuffer(GL_ARRAY_BUFFER, spritesVBO);
      glBufferSubData(GL_ARRAY_BUFFER, 0, numSprites*sizeof(Sprite), sprites);
      glUniform2f(viewLocation, viewPosition.x, viewPosition.y);
      glDrawArrays(GL_POINTS, 0, numSprites);
      glOKORDIE;
    }

    // draw text
    {
      glUseProgram(textShader);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, fontTexture);

      glBindVertexArray(textVAO);
      glBindBuffer(GL_ARRAY_BUFFER, textVBO);
      if (textDirty) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (textSpritesMax - textSprites)*sizeof(textSprites[0]), textSprites);
      }
      glUniform2f(viewLocation, 0, 0);
      glDrawArrays(GL_POINTS, 0, (textSpritesMax - textSprites));
      glOKORDIE;

      textDirty = false;
    }

    SDL_GL_SwapWindow(window);

    if (!(loopCount%100)) {
      printf("num Entities: %u\n", numEntities);
      printf("%f fps\n", 1/dt);
    }
  }
}
