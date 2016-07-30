const char* sprite_vertex_shader_src = R"(

  #version 330 core

  layout (location = 0) in vec4 pos;
  layout (location = 1) in vec4 texCoord;

  out VERTEX_DATA {
    vec4 texCoord;
  } myOutput;

  uniform vec2 uView;

  void main()
  {
    gl_Position = pos + vec4(uView, 0, 0);
    myOutput.texCoord = texCoord;
  }

)";

const char* sprite_geometry_shader_src = R"(
  #version 330 core

  layout (points) in;
  layout (triangle_strip, max_vertices = 4) out;

  in VERTEX_DATA {
    vec4 texCoord;
  } myInput[];

  out vec2 texCoord;

  void main() {
    // we swap texture y coordinates here
    vec2 texOrig = vec2(myInput[0].texCoord.x, myInput[0].texCoord.y + myInput[0].texCoord.w);
    vec2 texSize = vec2(myInput[0].texCoord.z, -myInput[0].texCoord.w);

    vec2 orig = gl_in[0].gl_Position.xy - gl_in[0].gl_Position.zw/2;
    vec2 size = gl_in[0].gl_Position.zw;

    gl_Position = vec4(orig, 0, 1);
    texCoord = texOrig;
    EmitVertex();
    gl_Position = vec4(orig + vec2(0, size.y), 0, 1);
    texCoord = texOrig + vec2(0, texSize.y);
    EmitVertex();
    gl_Position = vec4(orig + vec2(size.x, 0), 0, 1);
    texCoord = texOrig + vec2(texSize.x, 0);
    EmitVertex();
    gl_Position = vec4(orig + size, 0, 1);
    texCoord = texOrig + texSize;
    EmitVertex();
    EndPrimitive();
  }
)";

const char* sprite_fragment_shader_src = R"(

  #version 330 core

  in vec2 texCoord;

  uniform sampler2D uTexture;
  uniform vec3 uAmbientLight;

  out vec4 color;

  void main()
  {
    vec4 texColor = texture(uTexture, vec2(texCoord.x/1024, texCoord.y/1024));
    if (texColor.a == 0) {
      discard;
    } else {
      color = vec4(uAmbientLight, 1) * texColor;
    }
  }

)";

const char* particle_vertex_shader_src = R"(

  #version 330 core

  layout (location = 0) in vec3 model; // pos and size
  layout (location = 1) in vec3 texCoord; // pos and size

  out VS_OUT {
    vec4 texCoord;
  } myInput;

  out vec3 vTexCoord;

  uniform vec2 uView;

  void main()
  {
    gl_Position = vec4(model.xy + uView, 0, 1);
    gl_PointSize = model.z;
    vTexCoord = texCoord;
  }

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
