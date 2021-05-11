#include "pa_demo.h"

#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <atomic>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

static std::deque<std::condition_variable> conditions;
static std::deque<std::thread> threads;
static std::deque<std::atomic_bool> triggers;
static std::atomic_bool done;
static std::mutex mutex;
static PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC my_eglSwapBuffersWithDamageKHR = nullptr;

const char *vertex_shader_source[] = GLSL_VS(
	uniform float offset;
	uniform float green;
	in vec4 a_v4Position;
	in vec4 a_v4FillColor;
	out vec4 v_v4FillColor;
	void main()
	{
		v_v4FillColor = a_v4FillColor;
		v_v4FillColor.r += offset;
		v_v4FillColor.g = green;
		gl_Position = a_v4Position;
		gl_Position.x += offset;
	}
);

const char *fragment_shader_source[] = GLSL_FS(
	in vec4 v_v4FillColor;
	out vec4 fragColor;
	void main()
	{
	        fragColor = v_v4FillColor;
	}
);

const float triangleVertices[] =
{
	0.0f,  0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
};

const float triangleColors[] =
{
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
};

const GLuint indices[] =
{
	0, 1, 2
};

static GLuint vs, fs, draw_program, loc_offset, loc_green;

static void draw(PADEMO *handle, int surface)
{
	glClear(GL_COLOR_BUFFER_BIT);
	for (int i = 0; i < handle->current_frame + 1; i++)
	{
		GLfloat offset = -0.7f + i * 0.15f;
		glUniform1f(loc_offset, offset);
		glUniform1f(loc_green, surface * 0.3f + 0.3f);
		glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_INT, indices);
	}
}

static void thread_runner(PADEMO *handle, int me)
{
	std::unique_lock<std::mutex> lk(mutex);
	while (!done)
	{
		if (triggers.at(me))
		{
			const int idx = me + 1;
			eglMakeCurrent(handle->display, handle->surface[idx], handle->surface[idx], handle->context[idx]);
			draw(handle, idx);
			EGLint rect[] = { 0, 100, handle->width, handle->height - 100 };
			my_eglSwapBuffersWithDamageKHR(handle->display, handle->surface[idx], rect, 1);
			triggers[me] = false;
		}
		bool success = false;
		do {
			success = conditions.at(me).wait_for(lk, std::chrono::milliseconds(50), [&]{ return triggers.at(me) || done; });
		} while (!success);
	}
	eglReleaseThread();
}

static int setupGraphics(PADEMO *handle, int w, int h, void *user_data)
{
	done = false;

	my_eglSwapBuffersWithDamageKHR = (PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC)eglGetProcAddress("eglSwapBuffersWithDamageKHR");
	assert(my_eglSwapBuffersWithDamageKHR);

	// setup draw program
	draw_program = glCreateProgram();
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vertex_shader_source, NULL);
	compile("vertex_shader_source", vs);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fragment_shader_source, NULL);
	compile("fragment_shader_source", fs);
	glAttachShader(draw_program, vs);
	glAttachShader(draw_program, fs);
	link_shader("draw_program", draw_program);
	glUseProgram(draw_program);
	loc_offset = glGetUniformLocation(draw_program, "offset");
	loc_green = glGetUniformLocation(draw_program, "green");

	for (unsigned i = 0; i < handle->context.size(); i++)
	{
		eglMakeCurrent(handle->display, handle->surface[i], handle->surface[i], handle->context[i]);

		// setup buffers
		GLuint iLocPosition = glGetAttribLocation(draw_program, "a_v4Position");
		GLuint iLocFillColor = glGetAttribLocation(draw_program, "a_v4FillColor");
		glEnableVertexAttribArray(iLocFillColor);
		glEnableVertexAttribArray(iLocPosition);
		glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors);
		glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices);

		// setup space
		glViewport(0, 0, handle->width, handle->height);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
		glClearDepthf(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(draw_program);
	}
	eglMakeCurrent(handle->display, handle->surface[0], handle->surface[0], handle->context[0]);

	for (int i = 0; i < 2; i++)
	{
		triggers.emplace_back(false);
		conditions.emplace_back();
		threads.emplace_back(thread_runner, handle, i);
	}

	return 0;
}

static void callback_draw(PADEMO *handle, void *user_data)
{
	// We must handle the extra surfaces (number 1 and 2) manually. Activate both extra
	// windows once at start, and one of them once on frame 5.
	for (int i = 0; i < 2; i++)
	{
		if (handle->current_frame == 0 || (handle->current_frame == 5 && i == 0))
		{
			triggers[i] = true;
			conditions[i].notify_one();
			usleep(5000);
		}
	}

	// First surface last so that pademo can do the swap on the right context
	eglMakeCurrent(handle->display, handle->surface[0], handle->surface[0], handle->context[0]);
	draw(handle, 0);

	glStateDump_ARM();

	// verify in retracer
	assert_fb(handle->width, handle->height);
}

static void test_cleanup(PADEMO *handle, void *user_data)
{
	done = true;
	for (auto &c : conditions) c.notify_one();
	for (auto &t : threads) t.join();
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(draw_program);
}

int main()
{
	return init("multithread_2", callback_draw, setupGraphics, test_cleanup, nullptr, nullptr, 3);
}
