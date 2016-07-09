const char* vertexShaderSrc = R"(
  #version 330 core

  layout (location = 0) in vec4 transform;
  layout (location = 1) in vec4 texCoord;
  layout (location = 2) in vec2 model;

  out vec2 fTexCoord;

  uniform vec2 uView;

  void main()
  {
    gl_Position = vec4(transform.xy + model*transform.zw/2, 0.0, 1.0);
    // fTexCoord = texCoord.xy + transform.xy*texCoord.zw;
    fTexCoord = texCoord.xy + ((model+1)/2)*texCoord.zw;
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
    // color = texture(uTexture);
  }

)";
