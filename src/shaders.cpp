const char* vertexShaderSrc = R"(
  #version 330 core

  layout (location = 0) in vec3 position;
  layout (location = 1) in vec3 color;
  layout (location = 2) in vec2 texCoord;

  out vec3 fColor;
  out vec2 fTexCoord;

	uniform mat4 uTransform;

  void main()
  {
    gl_Position = uTransform * vec4(position, 1.0);
    fColor = color;
    fTexCoord = texCoord;
  }
)";

const char* fragmentShaderSrc = R"(
  #version 330 core

  in vec3 fColor;
  in vec2 fTexCoord;

  uniform sampler2D uTexture;
  uniform sampler2D uTexture2;

  out vec4 color;

  void main()
  {
    color = mix(texture(uTexture, fTexCoord), texture(uTexture2, vec2(fTexCoord.x, 1.0-fTexCoord.y)), 0.2);
  }
)";
