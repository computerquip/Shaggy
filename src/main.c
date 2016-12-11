#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "linmath.h"

typedef struct shaggy_ctx {
	SDL_Window *window;
	bool running;
} shaggy_ctx;

void handle_window_event(shaggy_ctx *ctx, SDL_Event *event) {
	switch (event->window.event) {
	case SDL_WINDOWEVENT_SHOWN:
		SDL_Log("Window %d shown", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_HIDDEN:
		SDL_Log("Window %d hidden", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_EXPOSED:
		SDL_Log("Window %d exposed", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_MOVED:
		SDL_Log("Window %d moved to %d,%d",
				event->window.windowID, event->window.data1,
				event->window.data2);
		break;
	case SDL_WINDOWEVENT_RESIZED:
		SDL_Log("Window %d resized to %dx%d",
				event->window.windowID, event->window.data1,
				event->window.data2);
		break;
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		SDL_Log("Window %d size changed to %dx%d",
				event->window.windowID, event->window.data1,
				event->window.data2);
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		SDL_Log("Window %d minimized", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_MAXIMIZED:
		SDL_Log("Window %d maximized", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_RESTORED:
		SDL_Log("Window %d restored", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_ENTER:
		SDL_Log("Mouse entered window %d",
				event->window.windowID);
		break;
	case SDL_WINDOWEVENT_LEAVE:
		SDL_Log("Mouse left window %d", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		SDL_Log("Window %d gained keyboard focus",
				event->window.windowID);
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		SDL_Log("Window %d lost keyboard focus",
				event->window.windowID);
		break;
	case SDL_WINDOWEVENT_CLOSE:
		SDL_Log("Window %d closed", event->window.windowID);
		ctx->running = false;
		break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
	case SDL_WINDOWEVENT_TAKE_FOCUS:
		SDL_Log("Window %d is offered a focus", event->window.windowID);
		break;
	case SDL_WINDOWEVENT_HIT_TEST:
		SDL_Log("Window %d has a special hit test", event->window.windowID);
		break;
#endif
	default:
		SDL_Log("Window %d got unknown event %d",
				event->window.windowID, event->window.event);
		break;
	}
}

int main() {
	shaggy_ctx ctx = {0, true};
	SDL_Event event;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	ctx.window = SDL_CreateWindow(
			"Shaggy",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			800, 600,
			SDL_WINDOW_OPENGL
	);

	glewInit();

	while (ctx.running) {
		mat4x4 Projection = {};
		mat4x4 View = {};
		mat4x4 Model = {};
		mat4x4 ViewModel = {};
		mat4x4 ProjectionViewModel = {};

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

		mat4x4_mul(ViewModel, View, Model);
		mat4x4_mul(ProjectionViewModel, Projection, ViewModel);

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_WINDOWEVENT:
				handle_window_event(&ctx, &event);
				break;
			default:
				break;
			}
		}

		SDL_GL_SwapWindow(ctx.window);
	}

	SDL_DestroyWindow(ctx.window);
	SDL_Quit();

	return 0;
}