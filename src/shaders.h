#include "sclog4c/sclog4c.h"
#include "tinydir.h"
#include "hashmap.h"
#include "slre.h"
#include <stdio.h>

/**********
 * Utility
 **********/

// http://www.cse.yorku.ca/~oz/hash.html
static uint64_t djb2(const char *str, size_t length) {
	unsigned long hash = 5381;
	char c;
	for (int i = 0; i < length; ++i) {
		c = *(str + i);
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

/*******************
 * Eventually, this will also support
 * offline compilation using the same API.
 *******************/
/***************************
 * Type Definitions
 * One per shader type
 ***************************/
#define \
    shader_type(T) \
    typedef GLuint shaggy_##T;

shader_type(vertex_shader);
shader_type(tess_control_shader);
shader_type(tess_evaluation_shader);
shader_type(geometry_shader);
shader_type(fragment_shader);

/****************************
 * Shader Creation Functions
 * One per shader type
 ****************************/

#define \
    shader_create(Type, Enum) \
    shaggy_##Type shaggy_create_##Type (void) { \
        return glCreateShader(Enum); \
    }

shader_create(vertex_shader, GL_VERTEX_SHADER);

shader_create(tess_control_shader, GL_TESS_CONTROL_SHADER);

shader_create(tess_evaluation_shader, GL_TESS_EVALUATION_SHADER);

shader_create(geometry_shader, GL_GEOMETRY_SHADER);

shader_create(fragment_shader, GL_FRAGMENT_SHADER);

/*************************
 * Platform-specific Code
 *************************/

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/***********************************************************************************
 * Load shader source into the shader via a file.
 * @param shader OpenGL Shader you wish to assign source.
 * @param file File to obtain source for said shader.
 * @todo Only works with POSIX for now since we virtually map the file using mmap.
 ***********************************************************************************/
void shaggy_source_shader_from_file(GLuint shader, const char *file) {
	struct stat source_stat;
	int source_fd;
	int error;
	size_t length;
	char *source;

	source_fd = open(file, O_RDONLY);

	if (source_fd == -1) {
		logm(ERROR, strerror(errno));
		return;
	}

	error = fstat(source, &source_stat);
	if (error == -1) {
		logm(ERROR, strerror(errno));
		return;
	}

	if (!S_ISREG(source_stat.st_mode)) {
		logm(ERROR, "Shader file isn't a regular file!");
		goto fail;
	}

	source = mmap(0, source_stat.st_size, PROT_READ, MAP_PRIVATE, source_fd, 0);
	if (source == MAP_FAILED) {
		logm(ERROR, strerror(errno));
		goto fail;
	}

	glShaderSource(shader, 1, (const GLchar * const*)&source, &source_stat.st_size);

	munmap(source, source_stat.st_size);

fail:
	close(source_fd);
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

void shaggy_source_shader_from_file(GLuint shader, const char *file) {
	HANDLE source;
	DWORD source_size = 0;
	HANDLE source_map;
	LPVOID source_map_view;

	source = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if (source == INVALID_HANDLE_VALUE) {
		_print_windows_error();
		return;
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

	UnmapViewOfFile(source_map_view);

	fail_map_view:
	CloseHandle(source_map);

	fail_map:
	CloseHandle(source);
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
struct shader_hm_entry {
	uint64_t hash;
	GLint shader;
};

DEFINE_HASHMAP(shader_hm, struct shader_hm_entry);
#define SHADER_CMP(left, right) (left->hash == right->hash)
#define SHADER_HASH(entry) entry->hash
DECLARE_HASHMAP(shader_hm, SHADER_CMP, SHADER_HASH, free, realloc);

struct shaggy_manager {
	shader_hm vertex_shader_map;
	shader_hm fragment_shader_map;
};

static inline
struct shaggy_manager *shaggy_create_shader_manager(void) {
	struct shaggy_manager *manager = calloc(1, sizeof(struct shaggy_manager));

	shader_hmNew(&manager->vertex_shader_map);
	shader_hmNew(&manager->fragment_shader_map);
}

static inline
void shaggy_destroy_shader_manager(struct shaggy_manager *manager) {
	free(manager);
}

static inline
void shaggy_manage_shader_file(struct shaggy_manager *manager, const char *pathname) {
	int bytes_scanned;
	int name_length;
	int error;
	struct slre_cap caps[2];
	tinydir_file file;
	const char *regexp = "(^[a-zA-Z0-9\\.]*)\\.(vert|frag)\\.glsl";

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
	bytes_scanned = slre_match(regexp, file.name, name_length, caps, 2, SLRE_IGNORE_CASE);

	if (bytes_scanned < 0 || bytes_scanned != name_length) {
		logm(WARNING, "File %s didn't match a valid shaggy shader file name.", file.name);
		return;
	}

	logm(INFO, "Filename %s - Captures: %.*s %.*s",
		 file.name,
		 caps[0].len, caps[0].ptr,
		 caps[1].len, caps[1].ptr);

	/*******************************
	 * Compile the shader and stuff
	 *******************************/
	if (strncmp("frag", caps[1].ptr, caps[1].len)) {
		shader = shaggy_create_fragment_shader();
	} else if (strncmp("vert", caps[1].ptr, caps[1].len)) {
		shader = shaggy_create_vertex_shader();
	} else {
		logm(ERROR, "Unknown shader type. This should never happen!");
		return;
	}

	shaggy_source_shader_from_file(shader, file.path);
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
	struct shader_hm_entry entry = {djb2(caps[0].ptr, caps[0].len), shader};
	struct shader_hm_entry *pEntry = &entry;

	HashMapPutResult result = shader_hmPut(&manager->fragment_shader_map, &pEntry, HMDR_FIND);
	if (result == HMPR_FOUND) {
		logm(WARNING, "Shader %s already exists!", file.name);
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

#define fetch_shader_func(T) \
static inline                                                                                        \
shaggy_##T##_shader shaggy_manage_fetch_##T##_shader(struct shaggy_manager *manager, const char *shader_name) { \
    struct shader_hm_entry entry = { djb2(shader_name, strlen(shader_name)), 0 };                    \
    struct shader_hm_entry *pEntry = &entry;                                                         \
    bool found = false;                                                                              \
                                                                                                     \
    found = shader_hmFind(&manager->T##_shader_map, &pEntry);                                        \
                                                                                                     \
    if (found == false) {                                                                            \
        logm(WARNING, "Failed to find shader %s.", shader_name);                                     \
    }                                                                                                \
}

fetch_shader_func(fragment)

fetch_shader_func(vertex)