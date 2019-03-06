#if !defined(GLWS_EGL_WAYLAND_HPP)
#define GLWS_EGL_WAYLAND_HPP

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <weston-egl-ext.h>
#include <wayland-server.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <wayland-server-protocol.h> // for enum wl_output_transform
// in case the above include does not include it
#ifndef WL_OUTPUT_TRANSFORM_ENUM
#define WL_OUTPUT_TRANSFORM_ENUM
enum wl_output_transform {
	WL_OUTPUT_TRANSFORM_NORMAL = 0,
	WL_OUTPUT_TRANSFORM_90 = 1,
	WL_OUTPUT_TRANSFORM_180 = 2,
	WL_OUTPUT_TRANSFORM_270 = 3,
	WL_OUTPUT_TRANSFORM_FLIPPED = 4,
	WL_OUTPUT_TRANSFORM_FLIPPED_90 = 5,
	WL_OUTPUT_TRANSFORM_FLIPPED_180 = 6,
	WL_OUTPUT_TRANSFORM_FLIPPED_270 = 7,
};
#endif /* WL_OUTPUT_TRANSFORM_ENUM */

#include "retracer/glws_egl.hpp"

namespace retracer {

struct key_press {
    uint32_t key;
    struct wl_list link;
};

struct keyboard_state {
    struct wl_list     keysPressed;
    /* TODO Originally the KB focus was tracked here as well, hence the struct.
     * As processStepEvent() is now global instead of per-drawable we don't
     * need to do this anymore, though really it make sense to only listen to
     * key presses when they affect the right window... */
};

struct output_size_properties {
    struct wl_output *output;
    int width;
    int height;
    int scale;
    enum wl_output_transform transform;
};

class GlwsEglWayland : public GlwsEgl
{
public:
    GlwsEglWayland();
    ~GlwsEglWayland();
    virtual Drawable* CreateDrawable(int width, int height, int win);
    virtual void processStepEvent();

    EGLNativeDisplayType getNativeDisplay();
    void releaseNativeDisplay(EGLNativeDisplayType display);

protected:
    struct wl_compositor*     mCompositor;
    struct wl_shell*          mShell;
    struct wl_output*         mOutput;
    struct wl_seat*           mSeat;
    struct wl_keyboard*       mKeyboard;
    struct wl_event_queue*    mKBQueue;
    struct wl_list            mKeysPressed;
    struct keyboard_state     mKeyboardState;
    struct output_size_properties
                              mOutputProps;
    struct wl_surface*        mKBFocus;
    int                       mOutputWidth;
    int                       mOutputHeight;
};

}

#endif // !defined(GLWS_EGL_WAYLAND_HPP)
