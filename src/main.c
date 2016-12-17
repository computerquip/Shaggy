#include "sclog4c/sclog4c.h"

#include <stdio.h>
#include <stdbool.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include "linmath.h"
#include "shaders.h"

typedef struct shaggy_ctx {
	SDL_Window *window;
	SDL_GLContext gl_ctx;
	bool running;
} shaggy_ctx;

void handle_window_event(shaggy_ctx *ctx, SDL_Event *event) {
	switch (event->window.event) {
		case SDL_WINDOWEVENT_SHOWN:
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			break;
		case SDL_WINDOWEVENT_MOVED:
			break;
		case SDL_WINDOWEVENT_RESIZED:
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			break;
		case SDL_WINDOWEVENT_RESTORED:
			break;
		case SDL_WINDOWEVENT_ENTER:
			break;
		case SDL_WINDOWEVENT_LEAVE:
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			break;
		case SDL_WINDOWEVENT_CLOSE:
			ctx->running = false;
			break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
		case SDL_WINDOWEVENT_TAKE_FOCUS:
			break;
		case SDL_WINDOWEVENT_HIT_TEST:
			break;
#endif
		default:
			logm(INFO, "Window %d got unknown event %d",
				 event->window.windowID, event->window.event);
			break;
	}
}

int main(int argc, char *argv[]) {
	shaggy_ctx ctx = {};
	SDL_Event event;
	struct shaggy_manager *shader_manager;

	sclog4c_level = INFO;

	ctx.running = true;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	/*************************************
	 * Set GL Context Attributes
	 * Must be set before Window Creation
	 *************************************/
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	/*******************************
	 * Create OpenGL-capable Window
	 *******************************/
	ctx.window = SDL_CreateWindow(
			"Shaggy",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			800, 600,
			SDL_WINDOW_OPENGL
	);

	if (ctx.window == NULL) {
		logm(FATAL, "Failed to create window: %s\n", SDL_GetError());
		return 0;
	}

	/****************************************
	 * Create Context Associated with Window
	 ****************************************/
	ctx.gl_ctx = SDL_GL_CreateContext(ctx.window);

	if (ctx.gl_ctx == NULL) {
		logm(FATAL, "Failed to create GL context: %s\n", SDL_GetError());
		return 0;
	}

	/**************************************
	 * Initialize OpenGL Function Pointers
	 * We use GLEW, may use GLAD later
	 **************************************/
	{
		GLenum error = glewInit();
		if (error != GLEW_OK) {
			logm(FATAL, "Failed to initialize GLEW: %s\n",
				 glewGetErrorString(error));

			return 0;
		}
	}

	/*************************
	 * Create Shader Programs
	 *************************/
	/*************************
	 * Use Manager Instead
	 *************************/
#if 0
	shaggy_program program;

	{
		shaggy_vertex_shader vert = shaggy_create_vertex_shader();
		shaggy_fragment_shader frag = shaggy_create_fragment_shader();

		shaggy_source_shader_from_file(vert, "../shaders/basic.vertex.glsl");
		shaggy_compile_shader(vert);
		if (!shaggy_check_shader_compile_status(vert)) {
			GLint log_length = shaggy_get_shader_info_log_length(vert);
			char buf[log_length];

			logm(FATAL, "Vertex shader failed to compile: %s\n",
				shaggy_get_shader_info_log(vert, log_length, buf));
		}

		shaggy_source_shader_from_file(frag, "../shaders/basic.frag.glsl");
		shaggy_compile_shader(frag);
		if (!shaggy_check_shader_compile_status(frag)) {
			GLint log_length = shaggy_get_shader_info_log_length(frag);
			GLchar buf[log_length];

			logm(FATAL, "Vertex shader failed to compile: %s\n",
					shaggy_get_shader_info_log(frag, log_length, buf));
		}

		program = shaggy_create_program();
		shaggy_attach_shader(program, vert);
		shaggy_attach_shader(program, frag);
		shaggy_link_program(program);
		if (!shaggy_check_program_link_status(program)) {
			GLint log_length = shaggy_get_program_info_log_length(program);
			GLchar buf[log_length];

			shaggy_get_program_info_log(program, log_length, buf);
			logm(FATAL, "Program failed to link: %s\n",
					shaggy_get_shader_info_log(frag, log_length, buf));
		}
	}
#else
	shader_manager = shaggy_create_shader_manager();
	shaggy_manage_shader_dir(shader_manager, "../shaders");
	shaggy_fragment_shader temp_shader = shaggy_manage_fetch_fragment_shader(shader_manager, "basic");
	shaggy_fragment_shader temp_shader2 = shaggy_manage_fetch_fragment_shader(shader_manager, "basic2");
#endif

	/**************************
	 * Main Logic Loop
	 **************************/
	while (ctx.running) {
		mat4x4 Projection = {};
		mat4x4 View = {};
		mat4x4 Model = {};
		mat4x4 ModelView = {};

		GLuint uniform_Matrices;

		/***********************
		 * Build World Matrices
		 ***********************/
		mat4x4_perspective(
				Projection,
				1.7f,
				16 / 9,
				0.1f,
				100.0f
		);

		mat4x4_look_at(
				View,
				(vec3) {1.0f, 0.0f, 0.0f},
				(vec3) {0.0f, 0.0f, 0.0f},
				(vec3) {0.0f, 0.0f, 1.0f}
		);

		mat4x4_identity(Model);

		mat4x4_mul(ModelView, View, Model);

		/*******************************
		 * Build Uniform Buffer Objects
		 * Pass Matrices In Said Objects
		 ********************************/
		glGenBuffers(1, &uniform_Matrices);
		glBindBuffer(GL_UNIFORM_BUFFER, uniform_Matrices);
		glNamedBufferData(uniform_Matrices, sizeof(mat4x4) * 2, NULL, GL_DYNAMIC_DRAW);
		glNamedBufferSubData(uniform_Matrices, 0, sizeof(mat4x4), Projection);
		glNamedBufferSubData(uniform_Matrices, sizeof(mat4x4), sizeof(mat4x4), ModelView);

		/***********************
		 * Input Handling Logic
		 ***********************/
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_WINDOWEVENT:
					handle_window_event(&ctx, &event);
					break;
				default:
					break;
			}
		}

		/********************
		 * Swap Framebuffers
		 ********************/
		SDL_GL_SwapWindow(ctx.window);
	}

	/**********
	 * Cleanup
	 **********/
	SDL_DestroyWindow(ctx.window);
	SDL_Quit();

	return 0;
}