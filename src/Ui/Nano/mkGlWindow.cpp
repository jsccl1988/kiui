//  Copyright (c) 2015 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

//#define NANOVG_GL_USE_UNIFORMBUFFER 1
#ifdef NANOVG_GLEW
#include <Ui/Nano/nanovg/glew.h>
#elif defined(KIUI_EMSCRIPTEN)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <GLFW/glfw3.h>

#include <Ui/mkUiConfig.h>
#include <Ui/Nano/mkGlWindow.h>

#include <Ui/mkUiWindow.h>

#include <iostream>

#define CAP_FRAMERATE
#ifdef CAP_FRAMERATE
#include <thread>
#endif

void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

namespace mk
{
	MouseButton convertGlfwButton(int button)
	{
		switch(button)
		{
		case GLFW_MOUSE_BUTTON_LEFT:
			return LEFT_BUTTON;
		case GLFW_MOUSE_BUTTON_RIGHT:
			return RIGHT_BUTTON;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			return MIDDLE_BUTTON;
		default:
			return LEFT_BUTTON;
		}
	}

	GlWindow::GlWindow(size_t width, size_t height, string title, string ressourcePath)
		: RenderWindow(width, height, title, 0)
		, mRessourcePath(ressourcePath)
		, mUiWindow()
		, mGlWindow(nullptr)
	{}

	GlWindow::~GlWindow()
	{
		glfwTerminate();
	}

	void GlWindow::initContext()
	{
		if(!glfwInit()) {
			printf("Failed to init GLFW.");
			return;
		}

		glfwSetErrorCallback(errorcb);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		mGlWindow = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), NULL, NULL);

		if(!mGlWindow) {
			glfwTerminate();
			return;
		}

		glfwSetWindowUserPointer(mGlWindow, this);
		glfwSetKeyCallback(mGlWindow, [](GLFWwindow* w, int key, int scancode, int action, int mods) { static_cast<GlWindow*>(glfwGetWindowUserPointer(w))->injectKey(key, scancode, action, mods); });
		glfwSetMouseButtonCallback(mGlWindow, [](GLFWwindow* w, int button, int action, int mods) { static_cast<GlWindow*>(glfwGetWindowUserPointer(w))->injectMouseButton(button, action, mods); });
		glfwSetCursorPosCallback(mGlWindow, [](GLFWwindow* w, double x, double y) { static_cast<GlWindow*>(glfwGetWindowUserPointer(w))->injectMouseMove(x, y); });

		glfwMakeContextCurrent(mGlWindow);
		glfwSetInputMode(mGlWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

#ifdef NANOVG_GLEW
		if(glewInit() != GLEW_OK) {
			printf("Could not init glew.\n");
			return;
		}
#endif

		int winWidth, winHeight;

		glfwGetWindowSize(mGlWindow, &winWidth, &winHeight);
		glfwGetFramebufferSize(mGlWindow, &mFbWidth, &mFbHeight);

		// Calculate pixel ration for hi-dpi devices.
		float pxRatio = (float)mFbWidth / (float)winWidth;

		mNanoWindow = make_unique<NanoWindow>(mWidth, mHeight, pxRatio, mRessourcePath);
		mUiWindow = make_unique<UiWindow>();
		mUiWindow->setup(this, mNanoWindow.get(), nullptr);
		this->initInput(mUiWindow.get(), 0);

		glfwSwapInterval(0);
		glfwSetTime(0);
	}

	void GlWindow::initInput(InputDispatcher* input, size_t windowHnd)
	{
		UNUSED(windowHnd);
		mInput = input;
	}

	bool GlWindow::renderFrame()
	{
		static size_t frames = 0;
		static double prevtime;

		double time = glfwGetTime();
		if(time - prevtime >= 4.f)
		{
			printf("fps %f\n", (frames / (time - prevtime)));
			prevtime = time;
			frames = 0;
		}

		// Update and render
		glViewport(0, 0, mFbWidth, mFbHeight);

		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		bool pursue = mUiWindow->nextFrame();
		mNanoWindow->nextFrame();

#ifdef CAP_FRAMERATE
		double delta = 16.66666667 - ((glfwGetTime() - time) * 1000.f);

		if(delta > 0.f)
			std::this_thread::sleep_for(std::chrono::milliseconds(int(delta)));
#endif

		glfwSwapBuffers(mGlWindow);
		glfwPollEvents();

		++frames;

		return pursue;
	}

	void GlWindow::updateSize()
	{
		int width, height;
		glfwGetWindowSize(mGlWindow, &width, &height);
		mUiWindow->resize(mWidth, mHeight);
	}

	void GlWindow::resize(size_t width, size_t height)
	{
		UNUSED(width); UNUSED(height);
	}

	void GlWindow::injectMouseMove(double x, double y)
	{
		float xDif = float(x) - mMouseX;
		float yDif = float(y) - mMouseY;

		mMouseX = float(x);
		mMouseY = float(y);

		float clampedX = std::max(0.f, std::min(float(mWidth), mMouseX));
		float clampedY = std::max(0.f, std::min(float(mHeight), mMouseY));

		mUiWindow->dispatchMouseMoved(clampedX, clampedY, xDif, yDif);
	}

	void GlWindow::injectMouseButton(int button, int action, int mods)
	{
		float clampedX = std::max(0.f, std::min(float(mWidth), mMouseX));
		float clampedY = std::max(0.f, std::min(float(mHeight), mMouseY));

		UNUSED(mods);
		if(action == GLFW_PRESS)
			mInput->dispatchMousePressed(clampedX, clampedY, convertGlfwButton(button));
		else if(action == GLFW_RELEASE)
			mInput->dispatchMouseReleased(clampedX, clampedY, convertGlfwButton(button));
	}

	void GlWindow::injectKey(int key, int scancode, int action, int mods)
	{
		UNUSED(key); UNUSED(mods);
		if(action == GLFW_PRESS)
			mInput->dispatchKeyPressed((KeyCode) scancode, ' ');
		else if(action == GLFW_RELEASE)
			mInput->dispatchKeyReleased((KeyCode)scancode, ' ');
	}

	void GlWindow::injectChar(int codepoint, int mods)
	{
		UNUSED(codepoint); UNUSED(mods);
		//if(action == GLFW_PRESS)
		//	mInput->keyPressed();
	}
}