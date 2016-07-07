const char* vertexShaderSrc = R"(
  #version 330 core

  layout (location = 0) in vec2 position;
  layout (location = 2) in vec2 texCoord;

  out vec2 fTexCoord;

  uniform vec4 uModel;
  uniform vec2 uView;

  void main()
  {
    gl_Position = vec4(position*uModel.zw + uModel.xy + uView, 0.0, 1.0);
    fTexCoord = texCoord;
  }
)";

const char* fragmentShaderSrc = R"(
  #version 330 core

  in vec2 fTexCoord;

  uniform sampler2D uTexture;

  out vec4 color;

  void main()
  {
    color = texture(uTexture, fTexCoord);
  }
)";
