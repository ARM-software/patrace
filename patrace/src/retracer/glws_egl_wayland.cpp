#include "retracer/glws_egl_wayland.hpp"
#include "retracer/retracer.hpp"

#include "dispatch/eglproc_auto.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <linux/input.h>

namespace retracer
{

static void shsurf_handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    struct wl_display *display = (struct wl_display *)data;

    wl_shell_surface_pong(shell_surface, serial);
    wl_display_flush(display);
}

static void shsurf_handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width,
	int32_t height)
{
}

static void shsurf_handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shellListener = {
	shsurf_handle_ping,
	shsurf_handle_configure,
	shsurf_handle_popup_done
};

class WaylandWindow : public NativeWindow
{
    public:
        WaylandWindow(int width, int height, const std::string& title, struct wl_display *display,
                struct wl_compositor *compositor, struct wl_shell *shell, struct wl_output *output,
                int fs_width, int fs_height)
            : NativeWindow(width, height, title)
                , mDisplay(display), mOutput(output), mFSWidth(fs_width), mFSHeight(fs_height)
        {
            mSurface = wl_compositor_create_surface(compositor);
            if (!mSurface) {
                DBG_LOG("Failed to initialize wayland surface\n");
                return;
            }

            mShellSurface = wl_shell_get_shell_surface(shell, mSurface);
            if (!mShellSurface) {
                DBG_LOG("Failed to initialize wayland shell surface\n");
                return;
            }

            wl_shell_surface_add_listener(mShellSurface, &shellListener, mDisplay);
            /* TODO we set the shell surface listener, so it responds to compositor
             * pings. However the retracer doesn't regularly dispatch events
             * from the Wayland default queue, so some compositors might think
             * the application is unresponsive. */

            mHandle = (EGLNativeWindowType)wl_egl_window_create(mSurface, width, height);
            if (!mHandle) {
                DBG_LOG("Failed to create wayland window\n");
                return;
            }

            mVisible = false;
        }

        ~WaylandWindow()
        {
            wl_egl_window_destroy((wl_egl_window*)mHandle);
            wl_shell_surface_destroy(mShellSurface);
            wl_surface_destroy(mSurface);
        }

        virtual void show()
        {
            NativeWindow::show();

            eglWaitClient();

            /* TODO: I *think* its the wl_shell_surface_set_* function that
             * that triggers it to be displayed and not just the creation
             * of the shell surface, but I might be wrong */
            if (getWidth() == mFSWidth && getHeight() == mFSHeight) {
                /* We assume that if the desired size is the size of the
                 * framebuffer then we want it fullscreen */
                wl_shell_surface_set_fullscreen(mShellSurface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                            0, mOutput);
            } else{
                wl_shell_surface_set_toplevel(mShellSurface);
            }
            wl_display_roundtrip(mDisplay);
            mVisible = true;
        }

        bool resize(int w, int h)
        {
            if (NativeWindow::resize(w, h))
            {
                /* NB: The resize won't actually happen yet, it'll
                 * only affect the next frame that EGL renders */
                wl_egl_window_resize((wl_egl_window*)mHandle, w, h, 0, 0);
                if (mVisible) {
                    /* We are already showing the window, so may need to
                     * transition to fullscreen or vice-versa */
                    if (getWidth() == mFSWidth && getHeight() == mFSHeight) {
                        /* We assume that if the desired size is the size of the
                         * framebuffer then we want it fullscreen */
                        wl_shell_surface_set_fullscreen(mShellSurface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                                    0, mOutput);
                    } else{
                        wl_shell_surface_set_toplevel(mShellSurface);
                    }
                    wl_display_roundtrip(mDisplay);
                }
                return true;
            }

            return false;
        }

    private:
        struct wl_display* mDisplay;
        struct wl_output* mOutput;
        int mFSWidth;
        int mFSHeight;
        struct wl_surface* mSurface;
        struct wl_shell_surface* mShellSurface;
        bool mVisible;
};


GlwsEglWayland::GlwsEglWayland()
    : GlwsEgl()
{
}

GlwsEglWayland::~GlwsEglWayland()
{
}

void GlwsEglWayland::processStepEvent()
{
    while (1)
    {
        /* Check the list first - key presses may have been added by a
         * processStepEvent() call on another window */
        struct key_press *key_press, *next;
        wl_list_for_each_reverse_safe(key_press, next, &mKeysPressed, link) {
            uint32_t key = key_press->key;

            /* always remove key presses - we just ignore non F? keys */
            wl_list_remove(&key_press->link);
            free(key_press);

            if (key >= KEY_F1 && key <= KEY_F4) {
                const unsigned int forwardNum = static_cast<unsigned int>(std::pow(10, key - KEY_F1));
                if (!gRetracer.RetraceForward(forwardNum, 0))
                    return;
            } else if (key >= KEY_F5 && key <= KEY_F8) {
                const unsigned int drawNum = static_cast<unsigned int>(std::pow(10, key - KEY_F5));
                if (!gRetracer.RetraceForward(0, drawNum))
                    return;
            }
        }

        int events = wl_display_dispatch_queue((struct wl_display *)mEglNativeDisplay, mKBQueue);
        if (events < 0) {
            DBG_LOG("An error occured while dispatching the keyboard Wayland queue\n");
            return;
        }
    }
}

/* type of data ptr passed to registry_handler and registry_remove_handler.
 * Could be done by making these class functions instead I suppose */
struct registry_data {
    struct wl_compositor** compositor;
    struct wl_shell**      shell;
    struct wl_output**     output;
    struct wl_seat**       seat;
};

static void registry_handler(void *data, struct wl_registry *wl_registry, uint32_t name,
        const char *interface, uint32_t version)
{
    /* This code stores only the first wl_output and wl_seat advertised */
    struct registry_data *ptrs_out = (struct registry_data *)data;

    if (!strcmp(interface, "wl_compositor")) {
        uint32_t bind_ver = std::min(version, 3u);
        *(ptrs_out->compositor) = (wl_compositor*)wl_registry_bind(wl_registry, name, &wl_compositor_interface, bind_ver);
    } else if (!strcmp(interface, "wl_shell")) {
        uint32_t bind_ver = std::min(version, 1u);
        *(ptrs_out->shell) = (wl_shell*)wl_registry_bind(wl_registry, name, &wl_shell_interface, bind_ver);
    } else if (!strcmp(interface, "wl_output") && !(*ptrs_out->output)) {
        uint32_t bind_ver = std::min(version, 2u);
        *(ptrs_out->output) = (wl_output*)wl_registry_bind(wl_registry, name, &wl_output_interface, bind_ver);
    } else if (!strcmp(interface, "wl_seat") && !(*ptrs_out->seat)) {
        uint32_t bind_ver = std::min(version, 1u);
        *(ptrs_out->seat) = (wl_seat*)wl_registry_bind(wl_registry, name, &wl_seat_interface, bind_ver);
    }
}

static void registry_remove_handler(void *data, struct wl_registry *wl_registry, uint32_t name)
{
}

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
    struct keyboard_state *kbstate = (struct keyboard_state *)data;
    /* respond to releasing a key */
    if (state != WL_KEYBOARD_KEY_STATE_RELEASED) {
        return;
    }

    struct key_press *key_press = new struct key_press();
    if(!key_press)
    {
        DBG_LOG("Failed to allocate\n");
    }

    key_press->key = key;
    wl_list_insert(&kbstate->keysPressed, &key_press->link);
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
}

static void
display_handle_geometry(void *data, struct wl_output *wl_output, int x, int y,
        int physical_width, int physical_height, int subpixel, const char *make,
        const char *model, int transform)
{
    struct output_size_properties *props = (struct output_size_properties *)data;

    if (wl_output == props->output) {
        props->transform = (wl_output_transform)transform;
    }
}

static void
display_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
        int width, int height, int refresh)
{
    struct output_size_properties *props = (struct output_size_properties *)data;

    if (wl_output == props->output && (flags & WL_OUTPUT_MODE_CURRENT)) {
        props->width = width;
        props->height = height;
    }
}

static void
display_handle_done(void *data, struct wl_output *wl_output)
{
}

static void
display_handle_scale(void *data, struct wl_output *wl_output, int factor)
{
    struct output_size_properties *props = (struct output_size_properties *)data;

    if (wl_output == props->output) {
        props->scale = factor;
    }
}

EGLNativeDisplayType GlwsEglWayland::getNativeDisplay()
{
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        gRetracer.reportAndAbort("Unable to open Wayland display\n");
        return 0;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        gRetracer.reportAndAbort("Unable to get Wayland display registry\n");
        wl_display_disconnect(display);
        return 0;
    }

    static struct wl_registry_listener listener = {
        registry_handler,
        registry_remove_handler
    };

    static struct registry_data interface_ptrs = {
        &mCompositor,
        &mShell,
        &mOutputProps.output,
        &mSeat
    };

    int res = wl_registry_add_listener(registry, &listener, &interface_ptrs);
    if (0 > res) {
        gRetracer.reportAndAbort("Unable to add registry listener\n");
        wl_registry_destroy(registry);
        wl_display_disconnect(display);
        return 0;
    }

    res = wl_display_roundtrip(display);

    if (0 > res) {
        gRetracer.reportAndAbort("Unable to perform display roundtrip to get interfaces\n");
        wl_registry_destroy(registry);
        wl_display_disconnect(display);
        return 0;
    }

    wl_registry_destroy(registry);

    if (!mCompositor || !mOutputProps.output || !mShell ) {
        gRetracer.reportAndAbort("Failed to obtain the shell, compositor and/or output interface\n");
        if (mCompositor)
            wl_compositor_destroy(mCompositor);
        if (mOutputProps.output)
            wl_output_destroy(mOutputProps.output);
        if (mShell)
            wl_shell_destroy(mShell);
        if (mSeat)
            wl_seat_destroy(mSeat);

        wl_display_disconnect(display);
        return 0;
    }

    static const struct wl_output_listener output_listener = {
        display_handle_geometry,
        display_handle_mode,
        display_handle_done,
        display_handle_scale
    };

    mOutputProps.width = 0;
    mOutputProps.height = 0;
    mOutputProps.scale = 1;
    mOutputProps.transform = WL_OUTPUT_TRANSFORM_NORMAL;

    res = wl_output_add_listener(mOutputProps.output, &output_listener, &mOutputProps);
    if (res) {
        gRetracer.reportAndAbort("Unable to add output listener\n");
        wl_compositor_destroy(mCompositor);
        wl_output_destroy(mOutputProps.output);
        wl_shell_destroy(mShell);
        if (mSeat) {
            wl_seat_destroy(mSeat);
        }
        wl_display_disconnect(display);
    }

    res = wl_display_roundtrip(display);
    if (0 > res) {
        gRetracer.reportAndAbort("Unable to roundtrip to the compositor after adding the output listener\n");
        wl_compositor_destroy(mCompositor);
        wl_output_destroy(mOutputProps.output);
        wl_shell_destroy(mShell);
        if (mSeat) {
            wl_seat_destroy(mSeat);
        }
        wl_display_disconnect(display);
    }

    mKBQueue = wl_display_create_queue(display);
    if (!mKBQueue) {
        gRetracer.reportAndAbort("Unable to create a queue for keyboard events\n");
        wl_compositor_destroy(mCompositor);
        wl_output_destroy(mOutputProps.output);
        wl_shell_destroy(mShell);
        wl_seat_destroy(mSeat);
        wl_display_disconnect(display);
        return 0;
    }

    wl_list_init(&mKeyboardState.keysPressed);

    static const struct wl_keyboard_listener kb_listener = {
        keyboard_handle_keymap,
        keyboard_handle_enter,
        keyboard_handle_leave,
        keyboard_handle_key,
        keyboard_handle_modifiers,
    };

    if (mSeat) {
        mKeyboard = wl_seat_get_keyboard(mSeat);
        if (!mKeyboard) {
            gRetracer.reportAndAbort("Unable to get keyboard from seat\n");
            wl_event_queue_destroy(mKBQueue);
            wl_compositor_destroy(mCompositor);
            wl_output_destroy(mOutputProps.output);
            wl_shell_destroy(mShell);
            wl_seat_destroy(mSeat);
            wl_display_disconnect(display);
            return 0;
        }

        wl_proxy_set_queue((struct wl_proxy *)mKeyboard, mKBQueue);

        res = wl_keyboard_add_listener(mKeyboard, &kb_listener, &mKeyboardState);
        if (res) {
            gRetracer.reportAndAbort("Unable to set keyboard listener\n");
            releaseNativeDisplay((EGLNativeDisplayType)display);
            return 0;
        }
    }

    return (EGLNativeDisplayType)display;
}

void GlwsEglWayland::releaseNativeDisplay(EGLNativeDisplayType display)
{
    if (mSeat) {
        wl_keyboard_destroy(mKeyboard);
        wl_seat_destroy(mSeat);
    }
    wl_event_queue_destroy(mKBQueue);
    struct key_press *key_press, *next;
    wl_list_for_each_reverse_safe(key_press, next, &mKeysPressed, link) {
        wl_list_remove(&key_press->link);
        delete key_press;
    }
    wl_compositor_destroy(mCompositor);
    wl_output_destroy(mOutputProps.output);
    wl_shell_destroy(mShell);

    wl_display_disconnect((struct wl_display *)mEglNativeDisplay);
    mEglNativeDisplay = 0;
}

Drawable* GlwsEglWayland::CreateDrawable(int width, int height, int win, EGLint const* attribList)
{
    Drawable* handler = NULL;
    NativeWindowMutex.lock();
    WinNameToNativeWindowMap_t::iterator it = gWinNameToNativeWindowMap.find(win);

    NativeWindow* window = 0;
    if (it != gWinNameToNativeWindowMap.end())  // if found
    {
        window = it->second;
        if (window->getWidth() != width ||
            window->getHeight() != height)
        {
            window->resize(width, height);
        }
    }
    else
    {
        std::stringstream title;
        title << win << " - eglretrace";

        mOutputWidth = mOutputProps.width * mOutputProps.scale;
        mOutputHeight = mOutputProps.height * mOutputProps.scale;

        int t = mOutputWidth;
        switch (mOutputProps.transform) {
        case WL_OUTPUT_TRANSFORM_90:
        case WL_OUTPUT_TRANSFORM_270:
        case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        case WL_OUTPUT_TRANSFORM_FLIPPED_270:
            mOutputWidth = mOutputHeight;
            mOutputHeight = t;
        default:
            break;
        }

        // TODO: Delete
        window = new WaylandWindow(width, height, title.str(), (struct wl_display *)mEglNativeDisplay,
                                   mCompositor, mShell, mOutput, mOutputWidth, mOutputHeight);
        gWinNameToNativeWindowMap[win] = window;
    }

    handler = new EglDrawable(width, height, mEglDisplay, mEglConfig, window, attribList);
    NativeWindowMutex.unlock();
    return handler;
}

GLWS& GLWS::instance()
{
    static GlwsEglWayland g;
    return g;
}
} // namespace retracer
