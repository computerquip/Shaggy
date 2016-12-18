#include "sclog4c/sclog4c.h"
#include "khash.h"
#include "tinydir.h"
#include "slre.h"
#include <stdio.h>

/**********
 * Utility
 **********/

/*******************
 * Eventually, this will also support
 * offline compilation using the same API.
 *******************/
/***************************
 * Type Definitions
 * One per shader type
 ***************************/
enum shaggy_shader_type {
	SHAGGY_VERTEX_SHADER,
	SHAGGY_FRAGMENT_SHADER,
	SHAGGY_TESS_CONTROL_SHADER,
	SHAGGY_TESS_EVALUATION_SHADER,
	SHAGGY_GEOMETRY_SHADER
};

#define \
    shader_type(T)         \
    typedef struct {       \
        GLuint shader;     \
    } shaggy_##T##_shader; \

shader_type(vertex);
shader_type(tess_control);
shader_type(tess_evaluation);
shader_type(geometry);
shader_type(fragment);

/****************************
 * Shader Creation Functions
 * One per shader type
 ****************************/

#define \
    shader_create(Type, Enum) \
    shaggy_##Type##_shader shaggy_create_##Type##_shader (void) { \
        return (shaggy_##Type##_shader){ glCreateShader(Enum) }; \
    }

shader_create(vertex, GL_VERTEX_SHADER);

shader_create(tess_control, GL_TESS_CONTROL_SHADER);

shader_create(tess_evaluation, GL_TESS_EVALUATION_SHADER);

shader_create(geometry, GL_GEOMETRY_SHADER);

shader_create(fragment, GL_FRAGMENT_SHADER);

/*************************
 * Platform-specific Code
 *************************/

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

/***********************************************************************************
 * Load shader source into the shader via a file.
 * @param shader OpenGL Shader you wish to assign source.
 * @param file File to obtain source for said shader.
 * @todo Only works with POSIX for now since we virtually map the file using mmap.
 ***********************************************************************************/
bool shaggy_source_shader_from_file(GLuint shader, const char *file) {
	struct stat source_stat;
	int source_fd;
	int error;
	size_t length;
	char *source;
	bool result = false;

	source_fd = open(file, O_RDONLY);
	if (source_fd == -1) {
		logm(ERROR, "Failed to open() file: %s", strerror(errno));
		return result;
	}

	error = fstat(source_fd, &source_stat);
	if (error == -1) {
		logm(ERROR, "fstat() failed: %s", strerror(errno));
		return result;
	}

	if (!S_ISREG(source_stat.st_mode)) {
		logm(ERROR, "Shader file isn't a regular file!");
		goto fail;
	}

	if (source_stat.st_size == 0) {
		logm(WARNING, "File %s is of size 0!", file);
		goto fail;
	}

	source = mmap(0, source_stat.st_size, PROT_READ, MAP_PRIVATE, source_fd, 0);
	if (source == MAP_FAILED) {
		logm(ERROR, "mmap() failed: %s", strerror(errno));
		goto fail;
	}

	glShaderSource(shader, 1, (const GLchar *const *) &source, (const GLint *) &source_stat.st_size);

	munmap(source, source_stat.st_size);

	result = true;

fail:
	close(source_fd);
	return result;
}

#elif _WIN32

/* Man I hate Windows... this is insane. */
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

static inline
void _print_windows_error() {
	LPVOID lpMsgBuf;

	/* This is probably up there in most atrocious things I've seen. */
	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL);

	logm(ERROR, "%s", lpMsgBuf);
	LocalFree(lpMsgBuf);
}

bool shaggy_source_shader_from_file(GLuint shader, const char *file) {
	HANDLE source;
	DWORD source_size = 0;
	HANDLE source_map;
	LPVOID source_map_view;
	bool result = false;

	source = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (source == INVALID_HANDLE_VALUE) {
		_print_windows_error();
		return result;
	}

	source_size = GetFileSize(source, 0);

	source_map = CreateFileMapping(
			source, NULL,
			PAGE_READONLY,
			0, source_size,
			NULL);

	if (source_map == NULL) {
		_print_windows_error();
		goto fail_map;
	}

	source_map_view = MapViewOfFile(
			source_map,
			FILE_MAP_READ,
			0, 0,
			source_size);

	if (source_map_view == NULL) {
		_print_windows_error();
		goto fail_map_view;
	}

	logm(WARNING, "%s", source_map_view);
	glShaderSource(shader, 1, (const GLchar *const *) &source_map_view, (const GLint *) &source_size);
	result = true;

	UnmapViewOfFile(source_map_view);

	fail_map_view:
	CloseHandle(source_map);

	fail_map:
	CloseHandle(source);
	return result;
}

#endif

static inline
void shaggy_compile_shader(GLuint shader) {
	glCompileShader(shader);
}

static inline
GLint shaggy_check_shader_compile_status(GLuint shader) {
	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	return compiled;
}

static inline
GLint shaggy_get_shader_info_log_length(GLuint shader) {
	GLint length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

	return length;
}

static inline
GLsizei shaggy_get_shader_info_log(GLuint shader, GLsizei buf_size, GLchar *buf) {
	GLsizei info_size;
	glGetShaderInfoLog(shader, buf_size, &info_size, buf);

	return info_size;
}

static inline
void shaggy_delete_shader(GLuint shader) {
	glDeleteShader(shader);
}

/*********************
 * Program Functions
 *********************/

typedef GLuint shaggy_program;

shaggy_program shaggy_create_program(void) {
	return glCreateProgram();
}

static inline
void shaggy_attach_shader(shaggy_program program, GLuint shader) {
	glAttachShader(program, shader);
}

static inline
void shaggy_link_program(shaggy_program program) {
	glLinkProgram(program);
}

static inline
GLint shaggy_check_program_link_status(shaggy_program program) {
	GLint status = GL_FALSE;

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	return status;
}

static inline
GLint shaggy_get_program_info_log_length(shaggy_program program) {
	GLint length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	return length;
}

static inline
GLint shaggy_get_program_info_log(shaggy_program program, GLsizei buf_size, GLchar *buf) {
	GLint length = 0;
	glGetProgramInfoLog(program, buf_size, &length, buf);

	return length;
}

static inline
void shaggy_detach_shader(shaggy_program program, GLuint shader) {
	glDetachShader(program, shader);
}

static inline
void shaggy_delete_program(shaggy_program program) {
	glDeleteProgram(program);
}

/**************************************************************************
 * Managed functions that handle shader loading, compiling, and cleanup
 * You give a shader, it will immediately try to load and compile the file.
 * It will stay in memory until you explicitly remove the shader or kill the managed object.
 * Programs must still be explicitly defined. Perhaps a spec input later.
 **************************************************************************/

KHASH_MAP_INIT_STR(shader_map, GLint)

struct shaggy_manager {
	khash_t(shader_map) *vertex_shader_map;
	khash_t(shader_map) *fragment_shader_map;
};

static inline
struct shaggy_manager *shaggy_create_shader_manager(void) {
	struct shaggy_manager *manager = malloc(sizeof(struct shaggy_manager));

	manager->vertex_shader_map = kh_init(shader_map);
	manager->fragment_shader_map = kh_init(shader_map);

	return manager;
}

static inline
void shaggy_destroy_shader_manager(struct shaggy_manager *manager) {
	kh_destroy(shader_map, manager->vertex_shader_map);
	kh_destroy(shader_map, manager->fragment_shader_map);
	free(manager);
}

#define shaggy_manage_add_shader(manager, name, shader) _Generic((shader),         \
        shaggy_vertex_shader: shaggy_manage_add_vertex_shader,                     \
        shaggy_fragment_shader: shaggy_manage_add_fragment_shader                  \
    ) (manager, name, shader)


#define shader_hash_impl(T)                                                                             \
static inline                                                                                           \
void shaggy_manage_add_##T##_shader(                                                                    \
struct shaggy_manager *manager, const char *name, shaggy_##T##_shader shader) {                         \
    int ret;                                                                                            \
    khiter_t iter;                                                                                      \
    khash_t(shader_map) *map = manager->T##_shader_map;                                                 \
                                                                                                        \
    iter = kh_put(shader_map, map, name, &ret);                                                         \
    if (ret == -1) {                                                                                    \
        logm(ERROR, "Failed to create key %s in shader hash table!", name);                             \
    }                                                                                                   \
                                                                                                        \
    logm(INFO, "Added %s to the " #T " shader hash table!", name);                                      \
    kh_val(map, iter) = shader.shader;                                                                  \
                                                                                                        \
}                                                                                                       \
                                                                                                        \
static inline                                                                                           \
shaggy_##T##_shader                                                                                     \
shaggy_manage_fetch_##T##_shader(struct shaggy_manager *manager, const char *shader_name) {             \
    khiter_t iter;                                                                                      \
    khash_t(shader_map) *map = manager->T##_shader_map;                                                 \
                                                                                                        \
    iter = kh_get(shader_map, map, shader_name);                                                        \
    if (iter == kh_end(map)) {                                                                          \
        logm(WARNING, "Failed to find key %s in shader hash table!", shader_name);                      \
        return (shaggy_##T##_shader) { 0 };                                                             \
    }                                                                                                   \
                                                                                                        \
    return (shaggy_##T##_shader) { kh_value(map, iter) };                                               \
}

shader_hash_impl(fragment)

shader_hash_impl(vertex)

static inline
void shaggy_manage_shader_file(struct shaggy_manager *manager, const char *pathname) {
	int bytes_scanned;
	int name_length;
	int error;
	struct slre_cap caps[2];
	tinydir_file file;
	const char *filename_exp = "(^[a-zA-Z0-9\\.]*)\\.(vert|frag)\\.glsl";

	GLint shader = 0; /* Resulting shader */

	error = tinydir_file_open(&file, pathname);
	if (error < 0) {
		logm(WARNING, "Failed to create tinydir object for %s", pathname);
		return;
	}

	if (!file.is_reg) {
		logm(WARNING, "%s is not a regular file!", pathname);
		return;
	}

	name_length = strlen(file.name);
	bytes_scanned = slre_match(filename_exp, file.name, name_length, caps, 2, SLRE_IGNORE_CASE);

	if (bytes_scanned < 0 || bytes_scanned != name_length) {
		logm(WARNING, "File %s didn't match a valid shaggy shader file name.", file.name);
		return;
	}

	/*******************************
	 * Compile the shader and stuff
	 *******************************/
	bytes_scanned = slre_match("frag", caps[1].ptr, caps[1].len, 0, 0, SLRE_IGNORE_CASE);
	if (bytes_scanned == caps[1].len) {
		shader = shaggy_create_fragment_shader().shader;
		goto regex_done;
	}

	bytes_scanned = slre_match("vert", caps[1].ptr, caps[1].len, 0, 0, SLRE_IGNORE_CASE);
	if (bytes_scanned == caps[1].len) {
		shader = shaggy_create_vertex_shader().shader;
		goto regex_done;
	}

	regex_done:

	if (!shaggy_source_shader_from_file(shader, file.path)) {
		return;
	}

	shaggy_compile_shader(shader);
	{
		GLint status = shaggy_check_shader_compile_status(shader);
		if (status == GL_FALSE) {
			GLsizei buf_size = shaggy_get_shader_info_log_length(shader);
			GLchar buf[buf_size];

			shaggy_get_shader_info_log(shader, buf_size, buf);
			logm(WARNING, "Failed to compile %s: %.*s",
				 file.name, buf_size, buf);
			return;
		}
	}
	/**********************************
	 * Add shader to hashmap so we can
	 * query by shader file name.
	 **********************************/
	bytes_scanned = slre_match("vert", caps[1].ptr, caps[1].len, 0, 0, SLRE_IGNORE_CASE);
	if (bytes_scanned == caps[1].len) {
		shaggy_manage_add_shader(manager, strndup(caps[0].ptr, caps[0].len), (shaggy_fragment_shader) {shader});
		return;
	}

	bytes_scanned = slre_match("frag", caps[1].ptr, caps[1].len, 0, 0, SLRE_IGNORE_CASE);
	if (bytes_scanned == caps[1].len) {
		shaggy_manage_add_shader(manager, strndup(caps[0].ptr, caps[0].len), (shaggy_vertex_shader) {shader});
		return;
	}
}

static inline
void shaggy_manage_shader_dir(struct shaggy_manager *manager, const char *dir_path) {
	tinydir_dir dir;
	tinydir_open(&dir, dir_path);

	while (dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		shaggy_manage_shader_file(manager, file.path);

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

static inline
shaggy_program shaggy_manage_build_program(struct shaggy_manager *manager, const char *shaders[5]) {
	GLint status;
	shaggy_program program = shaggy_create_program();

	GLuint shader = shaggy_manage_fetch_vertex_shader(manager, shaders[SHAGGY_VERTEX_SHADER]).shader;
	if (!shader) {
		logm(WARNING, "Failed to fetch vertex shader %s", shaders[SHAGGY_VERTEX_SHADER]);
	} else {
		shaggy_attach_shader(program, shader);
	}

	shader = shaggy_manage_fetch_fragment_shader(manager, shaders[SHAGGY_FRAGMENT_SHADER]).shader;
	if (!shader) {
		logm(WARNING, "Failed to fetch vertex shader %s", shaders[SHAGGY_FRAGMENT_SHADER]);
	} else {
		shaggy_attach_shader(program, shader);
	}

	/*********************************************
	 * TODO Various other steps can be taken here
	 * Will implement as they come along
	 *********************************************/

	shaggy_link_program(program);

	status = shaggy_check_program_link_status(program);
	if (status == GL_FALSE) {
		GLsizei buf_size = shaggy_get_program_info_log_length(program);
		GLchar buf[buf_size];

		shaggy_get_program_info_log(program, buf_size, buf);
		logm(WARNING, "Failed to link program: %.*s", buf_size, buf);
		shaggy_delete_program(program);
		program = 0;
	}

	return program;
}