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

#define DEBUG

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
#if RELEASE
#define glOKORDIE
#else
#define glOKORDIE _glOkOrDie(__FILE__, __LINE__)
#endif
#define arrsize(arr) (sizeof((arr))/sizeof(*(arr)))
typedef unsigned int Uint;
typedef unsigned char Byte;
#define swap(a,b) {auto tmp = *a; *a = *b; *b = tmp;}
#define local_persist static
#define MegaBytes(x) ((x)*1024LL*1024LL)
#define unimplemented printf("unimplemented method %s:%u\n", __FILE__, __LINE__)


namespace {

  #include "shaders.cpp"

  using glm::vec2;
  using glm::vec3;

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

  // Math
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

  // Physics
  struct Rect {
    float x,y,w,h;
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
  struct Sprite {
    union {
      struct {
        Rect screen, tex;
      };
      struct {
        Rect _; // texture
        struct { // Allocator information
          Sprite* next;
          uint size;
        };
      };
      struct {
        GLfloat x,y,w,h, tx,ty,tw,th;
      };
    };
  };
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
  const Uint CHARACTERS_MAX = 1024;
  bool textDirty = false;
  Sprite textSprites[CHARACTERS_MAX];
  Sprite* freeChars;
  Sprite* textSpritesMax = textSprites;
  struct TextRef {
    // TODO: squish together these bits?
    Sprite* block;
    uint size;
  };
  inline bool isnull(TextRef ref) {return !ref.block;}
  // text can only have one parent, multiple frees are not supported
  TextRef addText(const char* text, vec2 pos, float scale, bool center = false) {
    TextRef result = {};
    SDL_assert(text);
    if (!text) return result;

    uint len = strlen(text);
    // find large enough block (first fit)
    // TODO: best fit?
    Sprite** p = &freeChars;
    while (*p && (*p)->size < len) {
      p = &(*p)->next;
    }
    if (*p) {
      Sprite* block = *p;

      // set dirty flag
      textDirty = true;

      // free space
      if (len < (*p)->size) {
        // add the space that is left over
        Sprite* newBucket = *p + len;
        newBucket->size = (*p)->size - len;
        newBucket->next = (*p)->next;
        *p = newBucket;
      } else {
        // skip to next block
        *p = (*p)->next;
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
      Sprite* bucket = block;
      for (const char* c = text; *c; ++c, ++bucket) {
        uint i = *c;
        *bucket = Sprite{
          Rect{
            pos.x + (glyphs[i].dim.x - glyphs[i].dim.w/2) * scale,
            pos.y,
            glyphs[i].dim.w * scale,
            glyphs[i].dim.h * scale,
          },
          glyphs[i].tex
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
        ref.block[i].screen = invalidSpritePos();
      }
      // free block
      // we want the freelist to be ordered, find the free block before this block
      ref.block->size = ref.size;
      ref.block->next = 0;
      Sprite* leftFree = ref.block;
      if (!freeChars) {
        freeChars = ref.block;
      } else if (freeChars > ref.block) {
        if (freeChars == ref.block + ref.size) {
          // merge
          ref.block->size += freeChars->size;
        }
        else {
          // append
          ref.block->next = freeChars;
        }
        freeChars = ref.block;
      } else {
        // Find the block before the freed block
        Sprite* l = freeChars;
        Sprite* m = ref.block;
        while (l->next && l->next < m) {
          l = l->next;
        }
        Sprite* r = l->next;
        // block is to the left, block->next is to the right of the freed block
        l->next = m;
        m->next = r;
        SDL_assert((l < m) && (!r || m < r));
        // merge right
        if (r && m + m->size == r) {
          m->size += r->size;
          m->next = r->next;
        }
        // merge left
        if (l + l->size == m) {
          l->size += m->size;
          l->next = m->next;
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
      textSprites[i].screen = invalidSpritePos();
    }
    freeChars = textSprites;
    freeChars->size = CHARACTERS_MAX;
    freeChars->next = 0;
    return result;
  }

  const Uint SPRITES_MAX = 1024;
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
    if (numSprites == SPRITES_MAX) {
      return 0;
    }
    Sprite* sprite = sprites + numSprites;
    // create a reference to the new sprite
    SpriteRefSlot* slot;
    if (freeSpriteRefs) {
      slot = freeSpriteRefs;
      freeSpriteRefs = freeSpriteRefs->next;
    } else {
      SDL_assert(numSpriteRefs < SPRITES_MAX); // TODO: return null here if it fails
      if (numSpriteRefs == SPRITES_MAX) {
        return 0;
      } else {
        slot = spriteReferences + numSpriteRefs++;
      }
    }
    *sprite = s;
    slot->sprite = sprite;
    spriteReferenceLocation[numSprites] = slot;
    ++numSprites;
    return slot;
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
  enum EntityType : Uint {
    EntityType_Player,
    EntityType_Fairy,
    EntityType_Wall,
  };
  enum EntityFlag : Uint32 {
    EntityFlag_NoCollide = 1 << 0,
    EntityFlag_Removed = 1 << 1,
  };
  struct Entity;
  struct EntityRef {
    Uint id;
    Entity* entity;
  };
  struct Entity {
    Uint id; //TODO: should this be 64 bit?
    EntityType type;
    Uint32 flags;

    vec2 pos;
    vec2 vel;
    SpriteRef sprite;
    Rect hitBox;

    // familiar stuff
    EntityRef follow;

    // player stuff
    Uint health;
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
  struct EntitySlot {
    bool used;
    union {
      Entity entity;
      EntitySlot* next;
    };
  };
  const Uint ENTITIES_MAX = 1024;
  Uint numEntities = 0;
  EntitySlot* freeEntities = 0;
  Uint entityId = 0;
  EntitySlot entities[ENTITIES_MAX] = {};
  Entity* addEntity() {
    EntitySlot* slot;
    if (freeEntities) {
      slot = freeEntities;
      freeEntities = freeEntities->next;
    } else {
      SDL_assert(numEntities < ENTITIES_MAX);
      slot = entities + numEntities++;
    }
    slot->used = true;
    slot->entity = {};
    slot->entity.id = entityId++;
    return &slot->entity;
  };
  inline EntityRef createRef(Entity* target) {
    return {target->id, target};
  };
  inline Entity* get(EntityRef ref) {
    Entity* result = 0;
    if (!ref.entity) {
      result = NULL;
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
  inline void free(Entity* e) { // invalidates all references to this entity
    SDL_assert(slotOf(e) >= entities && slotOf(e) < entities+ENTITIES_MAX);
    EntitySlot* slot = slotOf(e);
    slot->used = false;
    freeEntities = slot;
    slot->next = freeEntities;
    if (e->sprite) {
      free(e->sprite);
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
  void* getCurrStackPos() {
    return stackCurr;
  }
  #define push(type) _push(sizeof(type))
  #define pushArr(type, count) _push(sizeof(type)*count)
  #define pop(type) _pop(sizeof(type))
  #define popArr(type, count) _pop(sizeof(type)*count)

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
      printf("%u %i %i\n", r->id, r->x, r->y);
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
      printf("%c ", i);
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        printf("failed to load char %c at %u\n", i, __LINE__);
        continue;
      }

      #ifdef DEBUG
        unsigned char* c = face->glyph->bitmap.buffer;
        for (uint i = 0; i < face->glyph->bitmap.width; ++i) {
          auto p = c;
          for (uint j = 0; j < face->glyph->bitmap.rows; ++j) {
            printf("%u ", *p);
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
      auto w = face->glyph->bitmap.width;
      auto h = face->glyph->bitmap.rows;
      float fheight = (float) height;
      printf("bx: %i by: %i advx: %li advy: %li\n", face->glyph->bitmap_left, face->glyph->bitmap_top, face->glyph->advance.x, face->glyph->advance.y);
      glyphs[(int) i] = Glyph{
        Rect{
          (float(face->glyph->bitmap_left) + w/2.0f)/fheight,
          (float(face->glyph->bitmap_top) + h/2.0f)/fheight,
          (float) w/fheight,
          (float) h/fheight
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

    if (checkTypeAndSwap(&a, &b, EntityType_Player, EntityType_Fairy)) {
      // a is now player, b is fairy
      printf("Passing through\n");
      a->health += b->health;
      remove(b);
      result = true;
    }

    return result;
  }

  bool tests() {
    for (Uint i = 0; i < 10; ++i) {
      SDL_assert(log2(1 << i) == i);
    }
    struct X {Rect a,b;};
    SDL_assert(sizeof(X) == sizeof(Sprite));
    return true;
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
  Entity* player;
  {
    const float w = 0.1, h = 0.1;
    player = addEntity();
    player->pos = {};
    // TODO: get spritesheet info from external source
    player->sprite = addSprite({rectAround(player->pos, w, h), Rect{11, 7, 70, 88}});
    player->type = EntityType_Player;
    player->hitBox = rectAround({}, w, h);
    player->health = 10;
  }

  // init fairies
  {
    for (int i = 0; i < 10; ++i) {
      Entity* fairy = addEntity();
      fairy->sprite = addSprite({0, 0, 0.1, 0.1, 585, 0, 94, 105});
      fairy->follow = createRef(player);
      fairy->type = EntityType_Fairy;
      fairy->health = 5;
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
    for (Uint i = 0; i < arrsize(walls); ++i) {
      Entity* wall = addEntity();
      wall->pos = center(walls[i]);
      wall->sprite = addSprite({walls[i], 0, 282, 142, 142});
      wall->hitBox = rectAround({}, walls[i].w, walls[i].h);
      wall->type = EntityType_Wall;
      putchar('\n');
    }
  }

  // init some text
  {
    addText("Hello world 1234567890", {}, 0.15, true);
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
    glBufferData(GL_ARRAY_BUFFER, numSprites*sizeof(Sprite), sprites, GL_DYNAMIC_DRAW);
    glOKORDIE;

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (4 * sizeof(GLfloat)));
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

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) (4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
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
    for (EntityIter iter = iterEntities(); iter; next(&iter)) {
      Entity* entity = get(iter);
      switch (entity->type) {
        case EntityType_Player: {
          const float ACCELLERATION = 6;
          const float GRAVITY = 6;
          const float JUMPPOWER = 10;
          const float DRAG = 0.90;

          // input movement
          entity->vel.x += ACCELLERATION*dt*isDown[KEY_RIGHT] - ACCELLERATION*dt*isDown[KEY_LEFT];
          if (wasPressed[KEY_UP]) {
            entity->vel.y = JUMPPOWER;
          }

          // physics movement
          entity->vel *= DRAG;
          entity->vel.y -= GRAVITY * dt;
        } break;
        case EntityType_Fairy: {
          const float ACCELLERATION = 3.0f + frand(-2, 2);
          const float DRAG = 0.97;
          const float NOISE = 0.2;
          Entity* target = get(entity->follow);
          vec2 diff = target->pos - entity->pos + v2rand(NOISE);
          entity->vel += ACCELLERATION * dt * glm::normalize(diff);
          entity->vel *= DRAG;
        } break;
        case EntityType_Wall: {
        } break;
      };
      // collision
      if (!isset(entity, EntityFlag_NoCollide)) {
        // find the possible collisions for the move vector
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
        Uint numHits = 0;
        Entity** hits = 0;
        for (auto target_iter = iterEntities(); target_iter; next(&target_iter)) {
          if (iter == target_iter) continue;
          Entity* target = get(target_iter);
          Rect padded = pad(moveBox, target->hitBox);
          /*
          if (target == entities+numEntities-1 && entity == entities) {
            printf("%u %u\n", entity->type, target->type);
            print("moveBox", moveBox);
            print("padded", padded);
            print("hitBox", entity->hitBox);
            print("pos", target->pos);
            printf("%u %u\n", isset(target, EntityFlag_Collides), collision(padded, target->pos));
          }
          */
          if (!isset(target, EntityFlag_NoCollide) && collision(padded, target->pos)) {
            ++numHits;
            Entity** ptr = (Entity**) push(Entity*);
            *ptr = target;
            if (!hits) {
              hits = ptr;
            }
          }
        }

        if (hits) {
          for (int iters = 0; iters < 4; ++iters) {
            float t = 1.0f;
            Entity** hit = 0;
            vec2 closestWall;
            vec2 endPoint = oldPos + move;
            for (Uint j = 0; j < numHits; ++j) {
              // find the shortest T for collision
              // TODO: Optimize: If we only use horizontal and vertical lines, we can optimize. Also precalculate corners, we do it twice now
              Rect padded = pad(hits[j]->pos + hits[j]->hitBox, entity->hitBox);
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
                  hit = hits + j;
                  closestWall = line2vector(lines[l]);
                }
              }
              SDL_assert(t >= 0);
            }
            if (hit) {
              #if 0
                printf("Hit!\n");
                printf("%u %u\n", (Uint) entity->type, (Uint) entity->type);
                print("moveBox", moveBox);
                print("hitBox", entity->hitBox);
                print("move", move);
                printf("t: %f\n", t);
              #endif
              Entity* hitEntity = *hit;
              bool passThrough = processCollision(entity, hitEntity, oldPos, endPoint, collision(hitEntity->hitBox + hitEntity->pos, endPoint));
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
                hits[hit-hits] = hits[--numHits];
                pop(Entity*);
              }
            } else {
              break;
            }
          };
          popArr(Entity*, numHits);
          SDL_assert(getCurrStackPos() == (void*) hits);
        }
        entity->vel = move/dt;
      }
      vec2 move = entity->vel * dt;
      const float moveEpsilon = 0.001;
      Sprite* sprite = get(entity->sprite);
      if (glm::abs(move.x) > moveEpsilon) {
        entity->pos.x += move.x;
        sprite->screen.x += move.x;
      }
      if (glm::abs(move.y) > moveEpsilon) {
        entity->pos.y += move.y;
        sprite->screen.y += move.y;
      }
    }

    // randomly add some text
    local_persist TextRef texts[16];
    local_persist uint numTexts = 0;
    if (multipleOf(rand(), 30) && numTexts < 16) {
      // add text
      char* text = (char*) stackCurr;
      int n = sprintf(text, "It's over %u!", rand());
      pushArr(int, n);
      texts[numTexts++] = addText(text, {frand(-1, 1), frand(-1, 1)}, 0.05, true);
      popArr(int, n);
    } else if (numTexts == 16 || (multipleOf(rand(), 20) && numTexts > 0)) {
      // remove text
      uint i = uint(rand())%numTexts;
      remove(texts[i]);
      texts[i] = texts[--numTexts];
    }

    // clean up removed entities
    for (auto iter = iterEntities(); iter; next(&iter)) {
      SDL_assert(get(iter));
      if (isset(get(iter), EntityFlag_Removed)) {
        free(get(iter));
      }
    }

    // begin draw
    {
      glClearColor(1,0,0,1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    // draw sprites
    {
      glUseProgram(spriteShader);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, spriteSheet);

      glBindVertexArray(spritesVAO);
      glBindBuffer(GL_ARRAY_BUFFER, spritesVBO);
      glBufferData(GL_ARRAY_BUFFER, numSprites*sizeof(Sprite), sprites, GL_DYNAMIC_DRAW);
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
        glBufferSubData(GL_ARRAY_BUFFER, 0, (textSpritesMax - textSprites)*sizeof(Sprite), textSprites);
      }
      glUniform2f(viewLocation, 0, 0);
      glDrawArrays(GL_POINTS, 0, (textSpritesMax - textSprites));
      glOKORDIE;

      textDirty = false;
    }

    SDL_GL_SwapWindow(window);

    #ifdef DEBUG
      if (!(loopCount%100)) {
        printf("%f fps\n", 1/dt);
      }
    #endif
  }
}
