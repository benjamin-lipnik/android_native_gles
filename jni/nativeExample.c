#include <jni.h>
#include <errno.h>
#include <math.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

struct engine {
	struct android_app* app;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;

	int32_t touchX;
	int32_t touchY;
};

/**
 * Initialize an EGL context for the current display.
 * TODO tidy this up, currently it's mostly Google example code
 */

#define LOG(...) LOGI("beni: " __VA_ARGS__)




const char * vertex_shader_source = 
	"#version 300 es\n"
	"precision mediump float;\n"
	"layout (location = 0) in vec2 aPos;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = vec4(aPos, 0.0f, 1.0f);\n"
	"}";

const char * fragment_shader_source = 
	"#version 300 es\n"
	"precision mediump float;\n"
	"out vec4 color;\n"
	"void main() {\n"
	"   color = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
	"}";

uint32_t vao;
uint32_t vbo;
uint32_t shader;

int make_shader() {
	//Creating and compiles shaders from shader_sources.h
	//creatin a vertex shader    
	unsigned int vertex_shader_id;  
	vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_id, 1, &vertex_shader_source, NULL); 
	glCompileShader(vertex_shader_id); //compiling my vertex shader

	//COMPILATION INFO
	int shader_compilation_status;
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &shader_compilation_status); //compilation success status
	if(!shader_compilation_status) { //check for bad compilation
	    char compilation_info[512] = {0};
	    glGetShaderInfoLog(vertex_shader_id, 512, NULL, compilation_info);
	    LOG("Vertex shader compilation failed: ->%s<-\n", compilation_info);
	    return -1;
	}
	

	//creatin a fregment shader 
	unsigned int fragment_shader_id;
	fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_id, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader_id);

	//COMPILATION INFO
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &shader_compilation_status); //compilation success status
	if(!shader_compilation_status) { //check for bad compilation
	    char compilation_info[512] = {0};
	    glGetShaderInfoLog(fragment_shader_id, 512, NULL, compilation_info);
	    LOG("Fragment shader compilation failed: ->%s<-\n", compilation_info);
	    return -1;
	}

	//Linking vertex and fragment shader into a shader program
	unsigned int shader_program_id = glCreateProgram();
	glAttachShader(shader_program_id, vertex_shader_id);
	glAttachShader(shader_program_id, fragment_shader_id);
	glLinkProgram(shader_program_id);

	//LINKING INFO
	int shader_linking_status;
	glGetProgramiv(shader_program_id, GL_LINK_STATUS, &shader_linking_status); //linking success status
	if(!shader_linking_status) { //Chech for bad linking
	    char linking_info[512] = {0};
	    glGetProgramInfoLog(shader_program_id, 512, NULL, linking_info);    
	    LOG("Shader linking failed: ->%s<-\n", linking_info);            
	    return -2;    
	}

	//Freeing used shader resources
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
	
	shader = shader_program_id;
	LOG("SHADER GUT!\n");	
	return 0;
}


void beni_setup() {
	LOG("GL version: %s!\n", glGetString(GL_VERSION));
	make_shader();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	float tri[] = {
		-1,-1,
		-1,1,
		 1,1
	};
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
	glEnableVertexAttribArray(0);	
}

void beni_loop() {
	glClearColor(0,1,1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shader);	
	glDrawArrays(GL_TRIANGLES, 0, 3);

}




int init_display(struct engine* engine) {

	// Setup OpenGL ES 3
	// http://stackoverflow.com/questions/11478957/how-do-i-create-an-opengl-es-2-context-in-a-native-activity

	const EGLint attribs[] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, //important
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_NONE
	};

	EGLint attribList[] =
	{
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
	};

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);

	context = eglCreateContext(display, config, NULL, attribList);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	// Grab the width and height of the surface
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;

	// Initialize GL state.
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	//glEnable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, w, h);

	beni_setup();

	return 0;
}

/**
 * Just the current frame in the display.
 */
void draw_frame(struct engine* engine) {
	// No display.
	if (engine->display == NULL) {
		return;
	}

	//glClearColor(1,0,0, 1);
	//glClear(GL_COLOR_BUFFER_BIT);
	beni_loop();	
	eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
void terminate_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
int32_t handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->touchX = AMotionEvent_getX(event, 0);
		engine->touchY = AMotionEvent_getY(event, 0);
		LOGI("x %d\ty %d\n",engine->touchX,engine->touchY);
		return 1;
	}
	return 0;
}

/**
 * Process the next main command.
 */
void handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (engine->app->window != NULL) {
			init_display(engine);
			draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		terminate_display(engine);
		break;
	case APP_CMD_LOST_FOCUS:
		draw_frame(engine);
		break;
	}
}

/**
 * Main entry point, handles events
 */
void android_main(struct android_app* state) {
	app_dummy();

	struct engine engine;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = handle_cmd;
	state->onInputEvent = handle_input;
	engine.app = state;

	//LOG("beni start.\n");
	LOGI("beni_start.\n");
	//LOGI("beni: GL_VERSION: ->%s<-\n", glGetString(GL_VERSION));

	// Read all pending events.
	while (1) {
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident=ALooper_pollAll(0, NULL, &events,(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				terminate_display(&engine);
				return;
			}
		}

		//LOG("beni: GL_VERSION: ->%s<-\n", glGetString(GL_VERSION));
		
		// Draw the current frame
		draw_frame(&engine);
	}
}
