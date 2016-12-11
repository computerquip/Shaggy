#include <stdio.h>

/*******************
 * Eventually, this will also support
 * offline compilation using the same API.
 *******************/

enum SHAGGY_LOG_SEVERITY {
	SHAGGY_LOG_SUCCESS,
	SHAGGY_LOG_WARNING,
	SHAGGY_LOG_ERROR,
};

#ifndef SHAGGY_USE_CUSTOM_LOGGER

void shaggy_log(enum SHAGGY_LOG_SEVERITY severity, const char *message) {
	const char *severity_map[] = {
			"SUCCESS",
			"WARNING",
			"ERROR"
	};

	printf("Shaggy - %s: %s\n", severity_map[severity], message);
}
#endif

/***************************
 * Type Definitions
 * One per shader type
 ***************************/
#define \
    shader_type(T) \
    typedef T GLuint;

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
    shaggy_##Type shaggy_create_##Type##(void) { \
        return { glCreateShader(Enum) }; \
    }

shader_create(vertex_shader, GL_VERTEX_SHADER);

shader_create(tess_control_shader, GL_TESS_CONTROL_SHADER);

shader_create(tess_evaluation_shader, GL_TESS_EVALUATION_SHADER);

shader_create(geometry_shader, GL_GEOMETRY_SHADER);

shader_create(fragment_shader, GL_FRAGMENT_SHADER);


#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/**********************
 * Load shader source into the shader via a file.
 * @param shader OpenGL Shader you wish to assign source.
 * @param file File to obtain source for said shader.
 * @todo Only works with POSIX for now since we virtually map the file using mmap.
 */
void shaggy_source_shader_from_file(GLuint shader, const char *file) {
	struct stat source_stat;
	int source_fd;
	int error;
	size_t length;
	char *source;

	source_fd = open(file, O_RDONLY);

	if (source_fd == -1) {
		shaggy_log(SHAGGY_LOG_ERROR, strerror(errno));
		return;
	}

	error = fstat(source, &source_stat);
	if (error == -1) {
		shaggy_log(SHAGGY_LOG_ERROR, strerror(errno));
		return;
	}

	if (!S_ISREG(source_stat.st_mode)) {
		shaggy_log(SHAGGY_LOG_ERROR, "Shader file isn't a regular file!");
		return;
	}

	source = mmap(0, source_stat.st_size, PROT_READ, MAP_PRIVATE, source_fd, 0);
	if (source == MAP_FAILED) {
		shaggy_log(SHAGGY_LOG_ERROR, strerror(errno));
		return;
	}

	glShaderSource(shader, 1, &source, &source_stat.st_size);

	munmap(source, source_stat.st_size);

	close(source_fd);
}

#endif

static inline
void shaggy_compile_shader(GLuint shader) {
	glCompileShader(shader);
}

static inline
GLint shaggy_get_shader_compile_status(GLuint shader) {
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

typedef shaggy_program GLuint;

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
GLint shaggy_get_program_link_status(shaggy_program program) {
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
void shaggy_detach_shader(shaggy_program program, shaggy_shader shader) {
	glDetachShader(program, shader);
}

static inline
void shaggy_delete_program(shaggy_program program) {
	glDeleteProgram(program);
}