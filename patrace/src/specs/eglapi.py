##########################################################################
#
# Copyright 2011 LunarG, Inc.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
##########################################################################/

"""EGL API description."""


from stdapi import *
from gltypes import *

EGLNativeDisplayType = IntPointer("EGLNativeDisplayType")
EGLNativeWindowType = IntPointer("EGLNativeWindowType")
EGLNativePixmapType = IntPointer("EGLNativePixmapType")

EGLDisplay = IntPointer("EGLDisplay")
EGLConfig = IntPointer("EGLConfig")
EGLContext = IntPointer("EGLContext")
EGLSurface = IntPointer("EGLSurface")

EGLClientBuffer = IntPointer("EGLClientBuffer")

EGLBoolean = Enum("EGLBoolean", [
    "EGL_FALSE",
    "EGL_TRUE",
])

EGLint = Alias("EGLint", Int32)

EGLError = FakeEnum(EGLint, [
    "EGL_SUCCESS",                  # 0x3000
    "EGL_NOT_INITIALIZED",          # 0x3001
    "EGL_BAD_ACCESS",               # 0x3002
    "EGL_BAD_ALLOC",                # 0x3003
    "EGL_BAD_ATTRIBUTE",            # 0x3004
    "EGL_BAD_CONFIG",               # 0x3005
    "EGL_BAD_CONTEXT",              # 0x3006
    "EGL_BAD_CURRENT_SURFACE",      # 0x3007
    "EGL_BAD_DISPLAY",              # 0x3008
    "EGL_BAD_MATCH",                # 0x3009
    "EGL_BAD_NATIVE_PIXMAP",        # 0x300A
    "EGL_BAD_NATIVE_WINDOW",        # 0x300B
    "EGL_BAD_PARAMETER",            # 0x300C
    "EGL_BAD_SURFACE",              # 0x300D
    "EGL_CONTEXT_LOST",             # 0x300E  /* EGL 1.1 - IMG_power_management */
])

EGLName = FakeEnum(EGLint, [
    "EGL_VENDOR",                   # 0x3053
    "EGL_VERSION",                  # 0x3054
    "EGL_EXTENSIONS",               # 0x3055
    "EGL_CLIENT_APIS",              # 0x308D
])

# collection of all other enum values
EGLenum = Enum("EGLenum", [
    # config attributes
    "EGL_BUFFER_SIZE",              # 0x3020
    "EGL_ALPHA_SIZE",               # 0x3021
    "EGL_BLUE_SIZE",                # 0x3022
    "EGL_GREEN_SIZE",               # 0x3023
    "EGL_RED_SIZE",                 # 0x3024
    "EGL_DEPTH_SIZE",               # 0x3025
    "EGL_STENCIL_SIZE",             # 0x3026
    "EGL_CONFIG_CAVEAT",            # 0x3027
    "EGL_CONFIG_ID",                # 0x3028
    "EGL_LEVEL",                    # 0x3029
    "EGL_MAX_PBUFFER_HEIGHT",       # 0x302A
    "EGL_MAX_PBUFFER_PIXELS",       # 0x302B
    "EGL_MAX_PBUFFER_WIDTH",        # 0x302C
    "EGL_NATIVE_RENDERABLE",        # 0x302D
    "EGL_NATIVE_VISUAL_ID",         # 0x302E
    "EGL_NATIVE_VISUAL_TYPE",       # 0x302F
    "EGL_SAMPLES",                  # 0x3031
    "EGL_SAMPLE_BUFFERS",           # 0x3032
    "EGL_SURFACE_TYPE",             # 0x3033
    "EGL_TRANSPARENT_TYPE",         # 0x3034
    "EGL_TRANSPARENT_BLUE_VALUE",   # 0x3035
    "EGL_TRANSPARENT_GREEN_VALUE",  # 0x3036
    "EGL_TRANSPARENT_RED_VALUE",    # 0x3037
    "EGL_NONE",                     # 0x3038  /* Attrib list terminator */
    "EGL_BIND_TO_TEXTURE_RGB",      # 0x3039
    "EGL_BIND_TO_TEXTURE_RGBA",     # 0x303A
    "EGL_MIN_SWAP_INTERVAL",        # 0x303B
    "EGL_MAX_SWAP_INTERVAL",        # 0x303C
    "EGL_LUMINANCE_SIZE",           # 0x303D
    "EGL_ALPHA_MASK_SIZE",          # 0x303E
    "EGL_COLOR_BUFFER_TYPE",        # 0x303F
    "EGL_RENDERABLE_TYPE",          # 0x3040
    "EGL_MATCH_NATIVE_PIXMAP",      # 0x3041  /* Pseudo-attribute (not queryable) */
    "EGL_CONFORMANT",               # 0x3042

    # config attribute values
    "EGL_SLOW_CONFIG",              # 0x3050  /* EGL_CONFIG_CAVEAT value */
    "EGL_NON_CONFORMANT_CONFIG",    # 0x3051  /* EGL_CONFIG_CAVEAT value */
    "EGL_TRANSPARENT_RGB",          # 0x3052  /* EGL_TRANSPARENT_TYPE value */
    "EGL_RGB_BUFFER",               # 0x308E  /* EGL_COLOR_BUFFER_TYPE value */
    "EGL_LUMINANCE_BUFFER",         # 0x308F  /* EGL_COLOR_BUFFER_TYPE value */

    # surface attributes
    "EGL_HEIGHT",                   # 0x3056
    "EGL_WIDTH",                    # 0x3057
    "EGL_LARGEST_PBUFFER",          # 0x3058
    "EGL_TEXTURE_FORMAT",           # 0x3080
    "EGL_TEXTURE_TARGET",           # 0x3081
    "EGL_MIPMAP_TEXTURE",           # 0x3082
    "EGL_MIPMAP_LEVEL",             # 0x3083
    "EGL_RENDER_BUFFER",            # 0x3086
    "EGL_VG_COLORSPACE",            # 0x3087
    "EGL_VG_ALPHA_FORMAT",          # 0x3088
    "EGL_HORIZONTAL_RESOLUTION",    # 0x3090
    "EGL_VERTICAL_RESOLUTION",      # 0x3091
    "EGL_PIXEL_ASPECT_RATIO",       # 0x3092
    "EGL_SWAP_BEHAVIOR",            # 0x3093
    "EGL_MULTISAMPLE_RESOLVE",      # 0x3099

    # surface attribute values
    "EGL_NO_TEXTURE",               # 0x305C
    "EGL_TEXTURE_RGB",              # 0x305D
    "EGL_TEXTURE_RGBA",             # 0x305E
    "EGL_TEXTURE_2D",               # 0x305F
    "EGL_VG_COLORSPACE_sRGB",       # 0x3089  /* EGL_VG_COLORSPACE value */
    "EGL_VG_COLORSPACE_LINEAR",     # 0x308A  /* EGL_VG_COLORSPACE value */
    "EGL_VG_ALPHA_FORMAT_NONPRE",   # 0x308B  /* EGL_ALPHA_FORMAT value */
    "EGL_VG_ALPHA_FORMAT_PRE",      # 0x308C  /* EGL_ALPHA_FORMAT value */
    "EGL_BUFFER_PRESERVED",         # 0x3094  /* EGL_SWAP_BEHAVIOR value */
    "EGL_BUFFER_DESTROYED",         # 0x3095  /* EGL_SWAP_BEHAVIOR value */
    "EGL_MULTISAMPLE_RESOLVE_DEFAULT", # 0x309A  /* EGL_MULTISAMPLE_RESOLVE value */
    "EGL_MULTISAMPLE_RESOLVE_BOX",     # 0x309B  /* EGL_MULTISAMPLE_RESOLVE value */

    # context attributes
    "EGL_CONTEXT_CLIENT_TYPE",      # 0x3097
    "EGL_CONTEXT_CLIENT_VERSION",   # 0x3098

    # misc
    "EGL_DRAW",                     # 0x3059
    "EGL_READ",                     # 0x305A
    "EGL_CORE_NATIVE_ENGINE",       # 0x305B
    "EGL_BACK_BUFFER",              # 0x3084
    "EGL_SINGLE_BUFFER",            # 0x3085
    "EGL_OPENVG_IMAGE",             # 0x3096
    "EGL_OPENGL_ES_API",            # 0x30A0
    "EGL_OPENVG_API",               # 0x30A1
    "EGL_OPENGL_API",               # 0x30A2

    # image targets
    "EGL_NATIVE_PIXMAP_KHR",                    # 0x30B0
    "EGL_GL_TEXTURE_2D_KHR",                    # 0x30B1
    "EGL_GL_TEXTURE_3D_KHR",                    # 0x30B2
    "EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR",   # 0x30B3
    "EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR",   # 0x30B4
    "EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR",   # 0x30B5
    "EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR",   # 0x30B6
    "EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR",   # 0x30B7
    "EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR",   # 0x30B8
    "EGL_GL_RENDERBUFFER_KHR",                  # 0x30B9
    "EGL_VG_PARENT_IMAGE_KHR",                  # 0x30BA

    # image attributes
    "EGL_GL_TEXTURE_LEVEL_KHR",                 # 0x30BC
    "EGL_GL_TEXTURE_ZOFFSET_KHR",               # 0x30BD
    "EGL_IMAGE_PRESERVED_KHR",                  # 0x30D2

    # sync types
    "EGL_SYNC_FENCE_KHR",                       # 0x30F9
    "EGL_SYNC_REUSABLE_KHR",                    # 0x30FA

    # sync attributes
    "EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR",     # 0x30F0
    "EGL_SYNC_STATUS_KHR",                      # 0x30F1
    "EGL_SIGNALED_KHR",                         # 0x30F2
    "EGL_UNSIGNALED_KHR",                       # 0x30F3
    "EGL_TIMEOUT_EXPIRED_KHR",                  # 0x30F5
    "EGL_CONDITION_SATISFIED_KHR",              # 0x30F6
    "EGL_SYNC_TYPE_KHR",                        # 0x30F7
    "EGL_SYNC_CONDITION_KHR",                   # 0x30F8

    # EGL_KHR_lock_surface / EGL_KHR_lock_surface2
    "EGL_MATCH_FORMAT_KHR",                     # 0x3043
    "EGL_FORMAT_RGB_565_EXACT_KHR",             # 0x30C0  /* EGL_MATCH_FORMAT_KHR value */
    "EGL_FORMAT_RGB_565_KHR",                   # 0x30C1  /* EGL_MATCH_FORMAT_KHR value */
    "EGL_FORMAT_RGBA_8888_EXACT_KHR",           # 0x30C2  /* EGL_MATCH_FORMAT_KHR value */
    "EGL_FORMAT_RGBA_8888_KHR",                 # 0x30C3  /* EGL_MATCH_FORMAT_KHR value */
    "EGL_MAP_PRESERVE_PIXELS_KHR",              # 0x30C4
    "EGL_LOCK_USAGE_HINT_KHR",                  # 0x30C5
    "EGL_BITMAP_POINTER_KHR",                   # 0x30C6
    "EGL_BITMAP_PITCH_KHR",                     # 0x30C7
    "EGL_BITMAP_ORIGIN_KHR",                    # 0x30C8
    "EGL_BITMAP_PIXEL_RED_OFFSET_KHR",          # 0x30C9
    "EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR",        # 0x30CA
    "EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR",         # 0x30CB
    "EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR",        # 0x30CC
    "EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR",    # 0x30CD
    "EGL_LOWER_LEFT_KHR",                       # 0x30CE  /* EGL_BITMAP_ORIGIN_KHR value */
    "EGL_UPPER_LEFT_KHR",                       # 0x30CF  /* EGL_BITMAP_ORIGIN_KHR value */
    "EGL_BITMAP_PIXEL_SIZE_KHR",                # 0x3110

    # EGL_ANGLE_surface_d3d_texture_2d_share_handle
    "EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE",    # 0x3200

    # EGL_HI_clientpixmap / EGL_HI_colorformats
    "EGL_CLIENT_PIXMAP_POINTER_HI",             # 0x8F74
    "EGL_COLOR_FORMAT_HI",                      # 0x8F70
    "EGL_COLOR_RGB_HI",                         # 0x8F71
    "EGL_COLOR_RGBA_HI",                        # 0x8F72
    "EGL_COLOR_ARGB_HI",                        # 0x8F73

    # EGL_IMG_context_priority
    "EGL_CONTEXT_PRIORITY_LEVEL_IMG",           # 0x3100
    "EGL_CONTEXT_PRIORITY_HIGH_IMG",            # 0x3101
    "EGL_CONTEXT_PRIORITY_MEDIUM_IMG",          # 0x3102
    "EGL_CONTEXT_PRIORITY_LOW_IMG",             # 0x3103

    # EGL_MESA_drm_image
    "EGL_DRM_BUFFER_MESA",                      # 0x31D3
    "EGL_DRM_BUFFER_FORMAT_MESA",               # 0x31D1
    "EGL_DRM_BUFFER_USE_MESA",                  # 0x31D2
    "EGL_DRM_BUFFER_STRIDE_MESA",               # 0x31D4

    # EGL_NV_coverage_sample
    "EGL_COVERAGE_BUFFERS_NV",                  # 0x30E0
    "EGL_COVERAGE_SAMPLES_NV",                  # 0x30E1

    # EGL_NV_coverage_sample_resolve
    "EGL_COVERAGE_SAMPLE_RESOLVE_NV",           # 0x3131

    # EGL_NV_depth_nonlinear
    "EGL_DEPTH_ENCODING_NV",                    # 0x30E2
    "EGL_DEPTH_ENCODING_NONLINEAR_NV",          # 0x30E3

    # EGL_NV_post_sub_buffer
    "EGL_POST_SUB_BUFFER_SUPPORTED_NV",         # 0x30BE

    # EGL_NV_sync
    "EGL_SYNC_PRIOR_COMMANDS_COMPLETE_NV",      # 0x30E6
    "EGL_SYNC_STATUS_NV",                       # 0x30E7
    "EGL_SIGNALED_NV",                          # 0x30E8
    "EGL_UNSIGNALED_NV",                        # 0x30E9
    "EGL_ALREADY_SIGNALED_NV",                  # 0x30EA
    "EGL_TIMEOUT_EXPIRED_NV",                   # 0x30EB
    "EGL_CONDITION_SATISFIED_NV",               # 0x30EC
    "EGL_SYNC_TYPE_NV",                         # 0x30ED
    "EGL_SYNC_CONDITION_NV",                    # 0x30EE
    "EGL_SYNC_FENCE_NV",                        # 0x30EF
])

# an alias of EGLenum
EGLIntAttrib = Alias("EGLint", EGLenum) # legacy (pre 1.5) attrib definition which is an int
EGLAttrib = Alias("EGLAttrib", IntPtr) # EGL's 'EGLAttrib' type which is an intptr_t
EGLSync = IntPointer("EGLSync")
EGLImage = IntPointer("EGLImage")
EGLTime = Alias("EGLTimeKHR", UInt64)

# EGL_KHR_image_base
EGLImageKHR = Alias("EGLImageKHR", EGLImage)

# EGL_KHR_reusable_sync
EGLSyncKHR = Alias("EGLSyncKHR", EGLSync)
EGLTimeKHR = Alias("EGLTimeKHR", EGLTime)

# EGL_NV_sync
EGLSyncNV = Alias("EGLSyncNV", EGLSync)
EGLTimeNV = Alias("EGLTimeNV", EGLTime)

# EGL_HI_clientpixmap
EGLClientPixmapHI = Struct("struct EGLClientPixmapHI", [
  (OpaquePointer(Void), "pData"),
  (EGLint, "iWidth"),
  (EGLint, "iHeight"),
  (EGLint, "iStride"),
])

# EGL_NV_system_time
EGLuint64NV = Alias("EGLuint64NV", UInt64)

eglapi = API("EGL")

EGLAttribList = Array(Const(EGLIntAttrib), "_AttribPairList_size(attrib_list, EGL_NONE)")
EGLAttribList2 = Array(Const(EGLAttrib), "_AttribPairList_size(attrib_list, (intptr_t)EGL_NONE)")
EGLRectList = Array(EGLint, "(n_rects * 4)")

EGLProc = IntPointer("__eglMustCastToProperFunctionPointerType")

def GlFunction(*args, **kwargs):
    kwargs.setdefault('call', 'EGLAPIENTRY')
    return Function(*args, **kwargs)

eglapi.addFunctions([
    # EGL 1.4
    GlFunction(EGLError, "eglGetError", [], sideeffects=False),

    GlFunction(EGLDisplay, "eglGetDisplay", [(EGLNativeDisplayType, "display_id")]),
    GlFunction(EGLBoolean, "eglInitialize", [(EGLDisplay, "dpy"), Out(Pointer(EGLint), "major"), Out(Pointer(EGLint), "minor")]),
    GlFunction(EGLBoolean, "eglTerminate", [(EGLDisplay, "dpy")]),

    GlFunction(ConstCString, "eglQueryString", [(EGLDisplay, "dpy"), (EGLName, "name")], sideeffects=False),

    GlFunction(EGLBoolean, "eglGetConfigs", [(EGLDisplay, "dpy"), (Array(EGLConfig, "config_size"), "configs"), (EGLint, "config_size"), Out(Pointer(EGLint), "num_config")]),
    GlFunction(EGLBoolean, "eglChooseConfig", [(EGLDisplay, "dpy"), (EGLAttribList, "attrib_list"), (Array(EGLConfig, "config_size"), "configs"), (EGLint, "config_size"), Out(Pointer(EGLint), "num_config")]),
    GlFunction(EGLBoolean, "eglGetConfigAttrib", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLint), "value")], sideeffects=False),

    GlFunction(EGLSurface, "eglCreateWindowSurface", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLNativeWindowType, "win"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePbufferSurface", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePixmapSurface", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLNativePixmapType, "pixmap"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroySurface", [(EGLDisplay, "dpy"), (EGLSurface, "surface")]),
    GlFunction(EGLBoolean, "eglQuerySurface", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLint), "value")], sideeffects=False),

    GlFunction(EGLBoolean, "eglBindAPI", [(EGLenum, "api")]),
    GlFunction(EGLenum, "eglQueryAPI", [], sideeffects=False),

    GlFunction(EGLBoolean, "eglWaitClient", []),

    GlFunction(EGLBoolean, "eglReleaseThread", []),

    GlFunction(EGLSurface, "eglCreatePbufferFromClientBuffer", [(EGLDisplay, "dpy"), (EGLenum, "buftype"), (EGLClientBuffer, "buffer"), (EGLConfig, "config"), (EGLAttribList, "attrib_list")]),

    GlFunction(EGLBoolean, "eglSurfaceAttrib", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLIntAttrib, "attribute"), (EGLint, "value")]),
    GlFunction(EGLBoolean, "eglBindTexImage", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLIntAttrib, "buffer")]),
    GlFunction(EGLBoolean, "eglReleaseTexImage", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLIntAttrib, "buffer")]),

    GlFunction(EGLBoolean, "eglSwapInterval", [(EGLDisplay, "dpy"), (EGLint, "interval")]),

    GlFunction(EGLContext, "eglCreateContext", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLContext, "share_context"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroyContext", [(EGLDisplay, "dpy"), (EGLContext, "ctx")]),
    GlFunction(EGLBoolean, "eglMakeCurrent", [(EGLDisplay, "dpy"), (EGLSurface, "draw"), (EGLSurface, "read"), (EGLContext, "ctx")]),

    GlFunction(EGLContext, "eglGetCurrentContext", [], sideeffects=False),
    GlFunction(EGLSurface, "eglGetCurrentSurface", [(EGLIntAttrib, "readdraw")], sideeffects=False),
    GlFunction(EGLDisplay, "eglGetCurrentDisplay", [], sideeffects=False),

    GlFunction(EGLBoolean, "eglQueryContext", [(EGLDisplay, "dpy"), (EGLContext, "ctx"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLint), "value")], sideeffects=False),

    GlFunction(EGLBoolean, "eglWaitGL", []),
    GlFunction(EGLBoolean, "eglWaitNative", [(EGLIntAttrib, "engine")]),
    GlFunction(EGLBoolean, "eglSwapBuffers", [(EGLDisplay, "dpy"), (EGLSurface, "surface")]),
    GlFunction(EGLBoolean, "eglCopyBuffers", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLNativePixmapType, "target")]),

    GlFunction(EGLProc, "eglGetProcAddress", [(Const(CString), "procname")]),

    # EGL_VERSION_1_5
    GlFunction(EGLSync, "eglCreateSync", [(EGLDisplay, "dpy"), (EGLenum, "type"), (EGLAttribList2, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroySync", [(EGLDisplay, "dpy"), (EGLSync, "sync")]),
    GlFunction(EGLint, "eglClientWaitSync", [(EGLDisplay, "dpy"), (EGLSync, "sync"), (EGLint, "flags"), (EGLTime, "timeout")]),
    GlFunction(EGLBoolean, "eglGetSyncAttrib", [(EGLDisplay, "dpy"), (EGLSync, "sync"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLAttrib), "value")], sideeffects=False),
    GlFunction(EGLImage, "eglCreateImage", [(EGLDisplay, "dpy"), (EGLContext, "ctx"), (EGLenum, "target"), (EGLClientBuffer, "buffer"), (EGLAttribList2, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroyImage", [(EGLDisplay, "dpy"), (EGLImage, "image")]),
    GlFunction(EGLDisplay, "eglGetPlatformDisplay", [(EGLenum, "platform"), (OpaquePointer(Void), "native_display"), (EGLAttribList2, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePlatformWindowSurface", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (OpaquePointer(Void), "native_window"), (EGLAttribList2, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePlatformPixmapSurface", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (OpaquePointer(Void), "native_pixmap"), (EGLAttribList2, "attrib_list")]),
    GlFunction(EGLBoolean, "eglWaitSync", [(EGLDisplay, "dpy"), (EGLSync, "sync"), (EGLIntAttrib, "flags")]),

    # EGL_ANGLE_query_surface_pointer
    GlFunction(EGLBoolean, "eglQuerySurfacePointerANGLE", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLIntAttrib, "attribute"), Out(Pointer(OpaquePointer(Void)), "value")], sideeffects=False),

    # EGL_CHROMIUM_get_sync_values
    GlFunction(EGLBoolean, "eglGetSyncValuesCHROMIUM", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), Out(Pointer(Int64), "ust"), Out(Pointer(Int64), "msc"), Out(Pointer(Int64), "sbc")], sideeffects=False),

    # EGL_EXT_platform_base
    GlFunction(EGLDisplay, "eglGetPlatformDisplayEXT", [(EGLenum, "platform"), (OpaquePointer(Void), "native_display"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePlatformWindowSurfaceEXT", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (OpaquePointer(Void), "native_window"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLSurface, "eglCreatePlatformPixmapSurfaceEXT", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (OpaquePointer(Void), "native_pixmap"), (EGLAttribList, "attrib_list")]),

    # EGL_EXT_swap_buffers_with_damage
    GlFunction(EGLBoolean, "eglSwapBuffersWithDamageEXT", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLRectList, "rects"), (EGLint, "n_rects")]),

    # EGL_HI_clientpixmap
    GlFunction(EGLSurface, "eglCreatePixmapSurfaceHI", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (Pointer(EGLClientPixmapHI), "pixmap")]),

    # EGL_KHR_fence_sync
    GlFunction(EGLSyncKHR, "eglCreateSyncKHR", [(EGLDisplay, "dpy"), (EGLenum, "type"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroySyncKHR", [(EGLDisplay, "dpy"), (EGLSyncKHR, "sync")]),
    GlFunction(EGLint, "eglClientWaitSyncKHR", [(EGLDisplay, "dpy"), (EGLSyncKHR, "sync"), (EGLint, "flags"), (EGLTimeKHR, "timeout")]),
    GlFunction(EGLBoolean, "eglGetSyncAttribKHR", [(EGLDisplay, "dpy"), (EGLSyncKHR, "sync"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLint), "value")], sideeffects=False),

    # EGL_KHR_image
    GlFunction(EGLImageKHR, "eglCreateImageKHR", [(EGLDisplay, "dpy"), (EGLContext, "ctx"), (EGLenum, "target"), (EGLClientBuffer, "buffer"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroyImageKHR", [(EGLDisplay, "dpy"), (EGLImageKHR, "image")]),

    # EGL_KHR_lock_surface
    GlFunction(EGLBoolean, "eglLockSurfaceKHR", [(EGLDisplay, "display"), (EGLSurface, "surface"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglUnlockSurfaceKHR", [(EGLDisplay, "display"), (EGLSurface, "surface")]),

    # EGL_KHR_reusable_sync
    GlFunction(EGLBoolean, "eglSignalSyncKHR", [(EGLDisplay, "dpy"), (EGLSyncKHR, "sync"), (EGLenum, "mode")]),

    # EGL_KHR_swap_buffers_with_damage
    GlFunction(EGLBoolean, "eglSwapBuffersWithDamageKHR", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLRectList, "rects"), (EGLint, "n_rects")]),

    # EGL_KHR_wait_sync
    GlFunction(EGLint, "eglWaitSyncKHR", [(EGLDisplay, "dpy"), (EGLSyncKHR, "sync"), (EGLint, "flags")]),

    # EGL_NV_sync
    GlFunction(EGLSyncNV, "eglCreateFenceSyncNV", [(EGLDisplay, "dpy"), (EGLenum, "condition"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglDestroySyncNV", [(EGLSyncNV, "sync")]),
    GlFunction(EGLBoolean, "eglFenceNV", [(EGLSyncNV, "sync")]),
    GlFunction(EGLint, "eglClientWaitSyncNV", [(EGLSyncNV, "sync"), (EGLint, "flags"), (EGLTimeNV, "timeout")]),
    GlFunction(EGLBoolean, "eglSignalSyncNV", [(EGLSyncNV, "sync"), (EGLenum, "mode")]),
    GlFunction(EGLBoolean, "eglGetSyncAttribNV", [(EGLSyncNV, "sync"), (EGLIntAttrib, "attribute"), Out(Pointer(EGLint), "value")], sideeffects=False),

    # EGL_MESA_drm_image
    GlFunction(EGLImageKHR, "eglCreateDRMImageMESA", [(EGLDisplay, "dpy"), (EGLAttribList, "attrib_list")]),
    GlFunction(EGLBoolean, "eglExportDRMImageMESA", [(EGLDisplay, "dpy"), (EGLImageKHR, "image"), Out(Pointer(EGLint), "name"), Out(Pointer(EGLint), "handle"), Out(Pointer(EGLint), "stride")]),

    # EGL_NV_post_sub_buffer
    GlFunction(EGLBoolean, "eglPostSubBufferNV", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLint, "x"), (EGLint, "y"), (EGLint, "width"), (EGLint, "height")]),

    # EGL_NV_system_time
    GlFunction(EGLuint64NV, "eglGetSystemTimeFrequencyNV", [], sideeffects=False),
    GlFunction(EGLuint64NV, "eglGetSystemTimeNV", [], sideeffects=False),

    # GL_OES_EGL_image
    GlFunction(Void, "glEGLImageTargetTexture2DOES", [(GLenum, "target"), (EGLImageKHR, "image")]),
    GlFunction(Void, "glEGLImageTargetRenderbufferStorageOES", [(GLenum, "target"), (EGLImageKHR, "image")]),

    # Fake call to store extra window information
    GlFunction(EGLSurface, "eglCreateWindowSurface2", [(EGLDisplay, "dpy"), (EGLConfig, "config"), (EGLNativeWindowType, "win"), (EGLAttribList, "attrib_list"), (EGLint, "x"), (EGLint, "y"), (EGLint, "width"), (EGLint, "height")]),

    # EGL_KHR_partial_update
    GlFunction(EGLBoolean, "eglSetDamageRegionKHR", [(EGLDisplay, "dpy"), (EGLSurface, "surface"), (EGLRectList, "rects"), (EGLint, "n_rects")]),
])
