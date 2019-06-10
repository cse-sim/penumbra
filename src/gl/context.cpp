/* Copyright (c) 2017 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#include <iostream>

// Penumbra
#include <gl/context.h>
#include <error.h>

namespace Pumbra {

const char* Context::vertexShaderSource =
R"src(
  #version 120
  uniform mat4 MVP;
  uniform vec3 vCol;
  attribute vec3 vPos;
  varying vec3 color;
  void main()
  {
    gl_Position = MVP * vec4(vPos, 1.0);
    color = vCol;
  }
)src";

const char* Context::fragmentShaderSource =
R"src(
  #version 120
  varying vec3 color;
  void main()
  {
    gl_FragColor = vec4(color, 1.0);
  }
)src";

static void glErrorCallback(int, const char* description)
{
  showMessage(MSG_INFO, description);
}

Context::Context(unsigned size) :
  size(size),
  modelSet(false),
  isWireFrame(false),
  isCameraMode(false),
  viewScale(1.f),
  cameraRotAngleX(0.f),
  cameraRotAngleY(0.f),
  lbutton_down(true),
  isRenderMode(false)
{

  glfwSetErrorCallback(glErrorCallback);

  if (!glfwInit()) {
    showMessage(MSG_ERR, "Unable to initialize GLFW.");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  window = glfwCreateWindow(1, 1, "Penumbra", NULL, NULL);
  glfwMakeContextCurrent(window);
  if (!window) {
    showMessage(MSG_ERR, "Unable to create OpenGL context. OpenGL 2.1 is required to perform shading calculations.");
  }

  // OpenGL extension loader
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    showMessage(MSG_ERR, "Failed to load OpenGL extensions.");
  }

  if (!glfwExtensionSupported("GL_ARB_vertex_array_object") && !glfwExtensionSupported("GL_APPLE_vertex_array_object")) {
    showMessage(MSG_ERR, "Your version of OpenGL does not support vertex array objects.");
  }
  if (!glfwExtensionSupported("GL_EXT_framebuffer_object")) {
    showMessage(MSG_ERR, "Your version of OpenGL does not support framebuffer objects.");
  }

  //std::string glVersion = (char*)glGetString(GL_VERSION);
  //showMessage(MSG_INFO, "OpenGL version = " + glVersion);
  GLint max_view_size[2];
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &max_view_size[0]);
  GLuint max_res = std::min(GL_MAX_RENDERBUFFER_SIZE_EXT, max_view_size[0]);
  if (size >= max_res) {
      showMessage(MSG_WARN, "The selected resolution is larger than the maximum allowable by your hardware. The size will be reset to be equal to the maximum allowable.");  // TODO: Format
      this->size = max_res;
  }

  glViewport(0, 0, this->size, this->size);

    //Input callbacks for orbit mode
  glfwSetWindowUserPointer(window, this);
#define glfwWPtr(w)  static_cast<Context*>(glfwGetWindowUserPointer(w))

  auto key_callback = [](GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/)
  {
	  if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		  glfwWPtr(w)->toggleWireFrame();
	  }

	  if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		  glfwWPtr(w)->toggleCameraMode();
	  }
  };

  auto scroll_callback = [](GLFWwindow* w, double /*xOffset*/, double yOffset) {

	  glfwWPtr(w)->viewScale += 0.1f*yOffset;

	  if (glfwWPtr(w)->isCameraMode) {
		  glfwWPtr(w)->setCameraMVP();
	  }
  };

  auto mouse_callback = [](GLFWwindow* w, int button, int action, int /*mods*/) {
  // Set a booleon to tell if the left button is down. And at the actual press, initialize the
  // curosor position.  Note that you only come in here the moments the button is pressed and
  // released, not between.

	  if (button == GLFW_MOUSE_BUTTON_LEFT) {
		  if (GLFW_PRESS == action) {
			  glfwWPtr(w)->lbutton_down = true;
			  glfwGetCursorPos(w, &glfwWPtr(w)->prevPosX, &glfwWPtr(w)->prevPosY);
		  }
		  else if (GLFW_RELEASE == action) {
			  glfwWPtr(w)->lbutton_down = false;
		  }
	  }
  };

  auto cursor_Pos_callback = [](GLFWwindow* w, double xPos, double yPos) {
  // Constantly monitored cursor position.

	  if (glfwWPtr(w)->lbutton_down && glfwWPtr(w)->isCameraMode) {

	    static const double rotationSpeed = 1./300.;

		  glfwWPtr(w)->cameraRotAngleX = -(yPos - glfwWPtr(w)->prevPosY )*rotationSpeed;	//Y motion should produce x rotation
		  glfwWPtr(w)->cameraRotAngleY = (xPos - glfwWPtr(w)->prevPosX)*rotationSpeed;	//X motion should produce -y rotation

		  glfwWPtr(w)->prevPosX = xPos;
		  glfwWPtr(w)->prevPosY = yPos;

		  glfwWPtr(w)->calcCameraView();
		  glfwWPtr(w)->setCameraMVP();
	  }
  };

  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetMouseButtonCallback(window, mouse_callback);
  glfwSetCursorPosCallback(window, cursor_Pos_callback);

  glfwSwapInterval(1);

  glEnable(GL_DEPTH_TEST);

  // Shader program
  GLProgram program(vertexShaderSource, fragmentShaderSource);

  glBindAttribLocation(program.getInt(), 0, "vPos");
  mvpLocation = glGetUniformLocation(program.getInt(), "MVP");
  vColLocation = glGetUniformLocation(program.getInt(), "vCol");

  glGenFramebuffersEXT(1, &fbo);
  glGenRenderbuffersEXT(1, &rbo);

  initOffScreenMode();

}

Context::~Context(){
  glDeleteFramebuffersEXT(1, &fbo);
  glDeleteRenderbuffersEXT(1, &rbo);
  glfwDestroyWindow(window);
}
void Context::toggleWireFrame() {
	isWireFrame = !isWireFrame;
	if (isWireFrame) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}
void Context::toggleCameraMode() {
	isCameraMode = !isCameraMode;
	lbutton_down = false; //There are things, like killing the last window, that may have left this true.
	if (isCameraMode) {
	    viewScale = 1.0;
		mat4x4_dup(cameraView, view);
		setCameraMVP();
	}
	else {
		setMVP();
	}
}

void Context::clearModel(){
  model.clearModel();
  modelSet = false;
}

void Context::setModel(const std::vector<float>& vertices) {

  // set model vertices
  model.setVertices(vertices);

  float bLeft = MAX_FLOAT, bBottom = MAX_FLOAT, bFront = MAX_FLOAT;
  float bRight = -MAX_FLOAT, bTop = -MAX_FLOAT, bBack = -MAX_FLOAT;


  // calculate bounding box
  for(int i = 0; i < (int)vertices.size(); i += model.vertexSize) {
    float x = vertices[i];
    float y = vertices[i+1];
    float z = vertices[i+2];
    bLeft = std::min(x, bLeft);
    bRight = std::max(x, bRight);
    bFront = std::min(y, bFront);
    bBack = std::max(y, bBack);
    bBottom = std::min(z, bBottom);
    bTop = std::max(z, bTop);
  }

  float tempBox[8][4] =
  {
    { bLeft, bFront, bBottom, 0.0 },
    { bLeft, bFront, bTop, 0.0 },
    { bLeft, bBack, bBottom, 0.0 },
    { bLeft, bBack, bTop, 0.0 },
    { bRight, bFront, bBottom, 0.0 },
    { bRight, bFront, bTop, 0.0 },
    { bRight, bBack, bBottom, 0.0 },
    { bRight, bBack, bTop, 0.0 }
  };

  for (std::size_t i = 0; i < 8; i++) {
    for (std::size_t j = 0; j < 4; j++) {
      modelBox[i][j] = tempBox[i][j];
    }
  }

  modelSet = true;
}

void Context::setScene(SurfaceBuffer surfaceBuffer, mat4x4 sunView) {

  if (!modelSet) {
    showMessage(MSG_ERR, "Model has not been set. Cannot set OpenGL scene.");
  }

  mat4x4_dup(view, sunView);

  // calculate clipping planes in rendered coorinates
   left=MAX_FLOAT;
   right=-MAX_FLOAT;
   bottom = MAX_FLOAT;
   top = -MAX_FLOAT;
   near_ = -MAX_FLOAT;
   far_ = MAX_FLOAT;

  for (int i = surfaceBuffer.begin*model.vertexSize; i < surfaceBuffer.begin*model.vertexSize + surfaceBuffer.count*model.vertexSize; i += model.vertexSize) {
    vec4 point = {
      model.vertexArray[i],
      model.vertexArray[i + 1],
      model.vertexArray[i + 2],
      0 };
    vec4 trans;
    mat4x4_mul_vec4(trans, view, point);
    left = std::min(trans[0], left);
    right = std::max(trans[0], right);
    bottom = std::min(trans[1], bottom);
    top = std::max(trans[1], top);
    //near_ = min(trans[2], near_);
    far_ = std::min(trans[2], far_);
  }

  // Use model box to determine near clipping plane
  for (std::size_t i = 0; i < 8; i++) {
    vec4 trans;
    mat4x4_mul_vec4(trans, view, modelBox[i]);
    near_ = std::max(trans[2], near_);
  }

  // account for camera position
  near_ -= 1.f;
  far_ -= 1.001f;	//For some reason, -1. is too tight when sun is perpendicular to the surface.

  // Grow horizontal extents of view by one pixel on each side
  float deltaX = (right - left)/size;
  left -= deltaX;
  right += deltaX;

  // Grow vertical extents of view by one pixel on each side
  float deltaY = (top - bottom)/size;
  bottom -= deltaY;
  top += deltaY;

  // calculate pixel area (A[i]*cos(theta) for each pixel of the surface)
  // multiplies by the number of pixels to get projected sunlit surface fraction

  pixelArea = (right - left)*(top - bottom) / (size*size);

  mat4x4_ortho(projection, left, right, bottom, top, -near_, -far_);
  mat4x4_mul(mvp, projection, view);

  setMVP();
}

void Context::calcCameraView() {
// Do something here to give the appearance of rotating the scene.
  mat4x4 tempMat;	//Transpose changes the affects of consecutive rotations from local to global space.
  mat4x4_transpose(tempMat, cameraView);
  mat4x4_rotate_X(cameraView, tempMat, cameraRotAngleX);
  mat4x4_rotate_Y(tempMat, cameraView, cameraRotAngleY);
  mat4x4_transpose(cameraView, tempMat);	//Transpose back.
}

void Context::setMVP()
{
  glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, (const GLfloat*)mvp);
}

void Context::setCameraMVP()
{
	float deltaW, deltaH;
	float cRight = right;
	float cLeft = left;
	float cTop = top;
	float cBottom = bottom;
	float cNear = near_;
	float cFar = far_;

	deltaW = (cRight - cLeft) / 2.;
	deltaH = (cTop - cBottom) / 2.;

	if (deltaW > deltaH) {
		cTop += (deltaW-deltaH);
		cBottom -= (deltaW - deltaH);
	}
	else {
		cLeft -= (deltaH - deltaW);
		cRight += (deltaW - deltaH);
	}

	cNear = 100.; // Set near and far to something now and make it tighter later.
	cFar = -100.;	// To tighten, look at the sphere of possible rotations.

	mat4x4 cProjection;
	mat4x4 cMVP;

	mat4x4_ortho(cProjection, viewScale*cLeft, viewScale*cRight, viewScale*cBottom, viewScale*cTop, -cNear, -cFar);
	mat4x4_mul(cMVP, cProjection, cameraView);
	glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, (const GLfloat*)cMVP);
}

void Context::drawModel() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDepthFunc(GL_LESS);
  model.drawAll();
  glDepthFunc(GL_EQUAL);
}

void Context::showRendering(SurfaceBuffer surfaceBuffer)
{
    glfwSetWindowSize(window, size, size);
    glfwShowWindow(window);

    if (!isRenderMode) {  // if not currently render mode
        initRenderMode();
    }

    while (!glfwWindowShouldClose(window))
    {
        glUniform3f(vColLocation, 0.5f, 0.5f, 0.5f);
        drawModel();
        glUniform3f(vColLocation, 1.f, 1.f, 1.f);
        model.drawSurface(surfaceBuffer);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwSetWindowShouldClose(window, 0);
    glfwHideWindow(window);
}

float Context::calculatePSSF(SurfaceBuffer surfaceBuffer) {

  if (isRenderMode) {  // if currently render mode, switch to off screen mode
    initOffScreenMode();
    isRenderMode = false;
  }

  glGenQueries(1, &query);
  drawModel();
  glBeginQuery(GL_SAMPLES_PASSED, query);
  model.drawSurface(surfaceBuffer);
  glEndQuery(GL_SAMPLES_PASSED);

  // wait until the result is available
  GLint ready(0);
  while (!ready) {
    glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &ready);
  }

  // retrieve result
  GLint pixelCount;
  glGetQueryObjectiv(query, GL_QUERY_RESULT, &pixelCount);

  glDeleteQueries(1, &query);

  return pixelCount*pixelArea;
}

void Context::initOffScreenMode() {
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, size, size);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbo);

  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
    std::string reason;
    switch (status) {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT: {
      reason = "Incomplete attachment.";
    }
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: {
      reason = "Incomplete or missing attachment.";
    }
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: {
      reason = "Incomplete formats.";
    }
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT: {
      reason = "Incomplete draw buffer.";
    }
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT: {
      reason = "Incomplete read buffer.";
    }
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT: {
      reason = "Framebuffers are not supported.";
    }
      break;
    default: {
      reason = "Reason unknown.";
    }
    }
    showMessage(MSG_ERR, "Unable to create framebuffer. " + reason);

  }

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glUniform3f(vColLocation, 0.5f, 0.5f, 0.5f); // TODO: Change shader to ignore color in this case
}

void Context::initRenderMode() {
    // set to default framebuffer and renderbuffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

}