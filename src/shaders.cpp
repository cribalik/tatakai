const char* sprite_vertex_shader_src = R"(

  #version 330 core

  layout (location = 0) in vec4 pos;
  layout (location = 1) in vec4 texCoord;

  out vec4 vTexCoord;
  out vec4 vPos;

  uniform vec2 uView;

  void main()
  {
    gl_Position = vec4(fPos + uView, 0, 1);
    vTexCoord = texCoord;
    vPos = pos;
  }

)";

const char* particle_vertex_shader_src = R"(

  #version 330 core

  layout (location = 0) in vec3 model; // pos and size
  layout (location = 1) in vec3 texCoord; // pos and size

  out vec3 vTexCoord;

  uniform vec2 uView;

  void main()
  {
    gl_Position = vec4(model.xy + uView, 0, 1);
    gl_PointSize = model.z;
    vTexCoord = texCoord;
  }

)";

const char* sprite_geometry_shader_src = R"(
  #version 330 core

  layout (points) in;
  layout (triangle_strip, max_vertices = 4) out;

  void main() {  }
)";


const char* sprite_fragment_shader_src = R"(

  #version 330 core

  in vec2 fTexCoord;
  in vec2 fPos;

  uniform sampler2D uTexture;
  uniform vec3 uAmbientLight;

  out vec4 color;

  void main()
  {

    vec4 texColor = texture(uTexture, 1-fTexCoord);
    // color = (diffuse + vec4(uAmbientLight,1)) * texColor;
    color = vec4(uAmbientLight, 1) * texColor;
  }

)";

const char* point_light_geometry_shader_src = R"(
  #version 330 core
)";

const char* point_light_fragment_shader_src = R"(
  #version 330 core

  int vec2 fPos;
  in vec2 fCenterPos;
  in vec3 fColor;

  out vec4 color;

  void main() {
    float constant = 1;
    float quadratic = 1.0;
    float linear = 0.0;

    vec2 diff = fPos - fCenterPos;
    float multiplier = 1/(constant + linear*diff + quadratic*diff*diff);
    vec4 color = vec4(color, multiplier);
  }

)";

#if 0
const char* lightShader = R"(

  #version 330 core

  #define LIGHTS_MAX 1024
  uniform int uNumLights;
  uniform vec2 uLightPos[LIGHTS_MAX];
  uniform vec3 uLightColor[LIGHTS_MAX];
  uniform vec3 uAmbientLight;

  void main() {

    float constant = 1;
    float quadratic = 1.0;
    float linear = 0.0;
    float zDistance = 0.01; // distance between lights and sprites

    vec4 ambient = vec4(0);
    vec4 diffuse = vec4(0);

    for (int i = 0; i < uNumLights; ++i) {
      float diff = length(uLightPos[i]-fPos);
      float attenuation = zDistance/(zDistance*constant + linear*diff + quadratic*diff*diff); // point light intensity decreases with distance

      float diffuseStrength = attenuation; // TODO: We should add another length factor here
      diffuse += vec4(diffuseStrength * uLightColor[i], 1);
      // ambient += vec4(uLightColor[i] * attenuation,1);
    }
  }

)";
#endif
