##########################################################################
#
# Copyright 2011 Jose Fonseca
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


'''Describe GL types.'''


import platform

from specs.stdapi import *

# GLboolean is typedefed to be *unsigned char* in the khronos header
# We don't want to use Enum to represent it, otherwise it will
# occupies *4* bytes in the trace file.
#
#GLboolean = Enum("GLboolean", [
#    "GL_TRUE",
#    "GL_FALSE",
#])
GLboolean = Alias("GLboolean", UChar)

GLvoid = Alias("GLvoid", Void)
GLbyte = Alias("GLbyte", SChar)
GLshort = Alias("GLshort", Short)
GLint = Alias("GLint", Int)
GLint64 = Alias("GLint64", Int64)
GLubyte = Alias("GLubyte", UChar)
GLushort = Alias("GLushort", UShort)
GLuint = Alias("GLuint", UInt)
GLuint64 = Alias("GLuint64", UInt64)
GLsizei = Alias("GLsizei", Int)
GLintptr = Alias("GLintptr", Int)
GLsizeiptr = Alias("GLsizeiptr", Int)
GLfloat = Alias("GLfloat", Float)
GLclampf = Alias("GLclampf", Float)
GLdouble = Alias("GLdouble", Double)
GLclampd = Alias("GLclampd", Double)
GLchar = Alias("GLchar", SChar)
GLustring = String(Const(GLubyte))
GLsyncstruct = Alias("GLsync", UInt64, True)

GLcharARB = Alias("GLcharARB", SChar)
GLintptrARB = Alias("GLintptrARB", Int)
GLsizeiptrARB = Alias("GLsizeiptrARB", Int)
GLhandleARB = Handle("handleARB", Alias("GLhandleARB", UInt))
GLhalfARB = Alias("GLhalfARB", UShort)
GLhalfNV = Alias("GLhalfNV", UShort)
GLint64EXT = Alias("GLint64EXT", Int64)
GLuint64EXT = Alias("GLuint64EXT", UInt64)
GLDEBUGPROC = Opaque("GLDEBUGPROC")
GLDEBUGPROCARB = Opaque("GLDEBUGPROCARB")
GLDEBUGPROCAMD = Opaque("GLDEBUGPROCAMD")
GLDEBUGPROCKHR = Opaque("GLDEBUGPROCKHR")

GLstring = String(GLchar)
GLstringConst = String(Const(GLchar))
GLstringARB = String(GLcharARB)
GLstringConstARB = String(Const(GLcharARB))

GLpointer = OpaquePointer(GLvoid)
GLpointerConst = OpaquePointer(Const(GLvoid))

GLlist = Handle("list", GLuint)
GLtexture = Handle("texture", GLuint)
GLbuffer = Handle("buffer", GLuint)
GLquery = Handle("query", GLuint)
GLfenceNV = Handle("fenceNV", GLuint)
GLprogram = Handle("program", GLuint)
GLshader = Handle("shader", GLuint)

GLattribLocation = Alias("GLuint", UInt)
# Share the same mapping table for uniform locations of both core and
# GL_ARB_shader_objects programs.  For a combination of reasons:
#
# - all OpenGL implementations appear to alias the names for both kind of
#   programs;
#
# - most applications will use only one kind of shader programs;
#
# - some applications actually mix glUniformXxx calls with
#   GL_ARB_shader_objects programs and glUniformXxxARB calls with core
#   programs, and therefore, rely on a joint implementation.
#
# We use GLhandleARB as program key, since it is wider (void *) on MacOSX.
#
GLuniformLocation = Handle("uniformLocation", GLint, key=('program', GLprogram))
GLuniformLocationARB = Handle("uniformlocation", GLint, key=('programObj', GLhandleARB))

GLuniformBlock = Handle("uniformBlockIndex", GLuint, key=('program', GLprogram))

GLprogramARB = Handle("programARB", GLuint)
GLframebuffer = Handle("framebuffer", GLuint)
GLrenderbuffer = Handle("renderbuffer", GLuint)
GLfragmentShaderATI = Handle("fragmentShaderATI", GLuint)
GLarray = Handle("array", GLuint)
GLregion = Handle("region", GLuint)
GLpipeline = Handle("pipeline", GLuint)
GLsampler = Handle("sampler", GLuint)
GLfeedback = Handle("feedback", GLuint)

GLsync = Handle("sync", GLsyncstruct)

#GLenum = Enum("GLenum", [
    # Parameters are added later from glparams.py's parameter table
#])
#GLenum = Alias("GLenum", UInt)
GLenum = Enum("GLenum", [])

# Some functions take GLenum disguised as GLint, and need special treatment so
# that symbolic names are traced correctly.
GLenum_int = Alias("GLint", GLenum)

GLenum_mode = FakeEnum(GLenum, [
    "GL_POINTS",                         # 0x0000
    "GL_LINES",                          # 0x0001
    "GL_LINE_LOOP",                      # 0x0002
    "GL_LINE_STRIP",                     # 0x0003
    "GL_TRIANGLES",                      # 0x0004
    "GL_TRIANGLE_STRIP",                 # 0x0005
    "GL_TRIANGLE_FAN",                   # 0x0006
    "GL_QUADS",                          # 0x0007
    "GL_QUAD_STRIP",                     # 0x0008
    "GL_POLYGON",                        # 0x0009
    "GL_LINES_ADJACENCY",                # 0x000A
    "GL_LINE_STRIP_ADJACENCY",           # 0x000B
    "GL_TRIANGLES_ADJACENCY",            # 0x000C
    "GL_TRIANGLE_STRIP_ADJACENCY",       # 0x000D
    "GL_PATCHES",                        # 0x000E
])

GLenum_error = FakeEnum(GLenum, [
    "GL_NO_ERROR",                       # 0x0
    "GL_INVALID_ENUM",                   # 0x0500
    "GL_INVALID_VALUE",                  # 0x0501
    "GL_INVALID_OPERATION",              # 0x0502
    "GL_STACK_OVERFLOW",                 # 0x0503
    "GL_STACK_UNDERFLOW",                # 0x0504
    "GL_OUT_OF_MEMORY",                  # 0x0505
    "GL_INVALID_FRAMEBUFFER_OPERATION",  # 0x0506
    "GL_TABLE_TOO_LARGE",                # 0x8031
])

GLbitfield = Alias("GLbitfield", UInt)

GLbitfield_attrib = Flags(GLbitfield, [
    "GL_ALL_ATTRIB_BITS",     # 0x000FFFFF
    "GL_CURRENT_BIT",         # 0x00000001
    "GL_POINT_BIT",           # 0x00000002
    "GL_LINE_BIT",            # 0x00000004
    "GL_POLYGON_BIT",         # 0x00000008
    "GL_POLYGON_STIPPLE_BIT", # 0x00000010
    "GL_PIXEL_MODE_BIT",      # 0x00000020
    "GL_LIGHTING_BIT",        # 0x00000040
    "GL_FOG_BIT",             # 0x00000080
    "GL_DEPTH_BUFFER_BIT",    # 0x00000100
    "GL_ACCUM_BUFFER_BIT",    # 0x00000200
    "GL_STENCIL_BUFFER_BIT",  # 0x00000400
    "GL_VIEWPORT_BIT",        # 0x00000800
    "GL_TRANSFORM_BIT",       # 0x00001000
    "GL_ENABLE_BIT",          # 0x00002000
    "GL_COLOR_BUFFER_BIT",    # 0x00004000
    "GL_HINT_BIT",            # 0x00008000
    "GL_EVAL_BIT",            # 0x00010000
    "GL_LIST_BIT",            # 0x00020000
    "GL_TEXTURE_BIT",         # 0x00040000
    "GL_SCISSOR_BIT",         # 0x00080000
    "GL_MULTISAMPLE_BIT",     # 0x20000000
])

GLbitfield_client_attrib = Flags(GLbitfield, [
    "GL_CLIENT_ALL_ATTRIB_BITS",  # 0xFFFFFFFF
    "GL_CLIENT_PIXEL_STORE_BIT",  # 0x00000001
    "GL_CLIENT_VERTEX_ARRAY_BIT", # 0x00000002
])

GLbitfield_shader = Flags(GLbitfield, [
    "GL_ALL_SHADER_BITS",                        # 0xFFFFFFFF
    "GL_VERTEX_SHADER_BIT",                      # 0x00000001
    "GL_FRAGMENT_SHADER_BIT",                    # 0x00000002
    "GL_GEOMETRY_SHADER_BIT",                    # 0x00000004
    "GL_TESS_CONTROL_SHADER_BIT",                # 0x00000008
    "GL_TESS_EVALUATION_SHADER_BIT",             # 0x00000010
    "GL_COMPUTE_SHADER_BIT",                     # 0x00000020
])

GLbitfield_access = Flags(GLbitfield, [
    "GL_MAP_READ_BIT",                # 0x0001
    "GL_MAP_WRITE_BIT",               # 0x0002
    "GL_MAP_INVALIDATE_RANGE_BIT",    # 0x0004
    "GL_MAP_INVALIDATE_BUFFER_BIT",   # 0x0008
    "GL_MAP_FLUSH_EXPLICIT_BIT",      # 0x0010
    "GL_MAP_UNSYNCHRONIZED_BIT",      # 0x0020
    "GL_MAP_PERSISTENT_BIT",          # 0x0040
    "GL_MAP_COHERENT_BIT",            # 0x0080
])

GLbitfield_storage = Flags(GLbitfield, [
    "GL_MAP_READ_BIT",                # 0x0001 (existing)
    "GL_MAP_WRITE_BIT",               # 0x0002 (existing)
    "GL_MAP_PERSISTENT_BIT",          # 0x0040
    "GL_MAP_COHERENT_BIT",            # 0x0080
    "GL_DYNAMIC_STORAGE_BIT",         # 0x0100
    "GL_CLIENT_STORAGE_BIT",          # 0x0200
])

GLbitfield_sync_flush = Flags(GLbitfield, [
    "GL_SYNC_FLUSH_COMMANDS_BIT",				# 0x00000001
])

GLbitfield_barrier = Flags(GLbitfield, [
    "GL_ALL_BARRIER_BITS",                      # 0xFFFFFFFF
    "GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT",       # 0x00000001
    "GL_ELEMENT_ARRAY_BARRIER_BIT",             # 0x00000002
    "GL_UNIFORM_BARRIER_BIT",                   # 0x00000004
    "GL_TEXTURE_FETCH_BARRIER_BIT",             # 0x00000008
    "GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV",   # 0x00000010
    "GL_SHADER_IMAGE_ACCESS_BARRIER_BIT",       # 0x00000020
    "GL_COMMAND_BARRIER_BIT",                   # 0x00000040
    "GL_PIXEL_BUFFER_BARRIER_BIT",              # 0x00000080
    "GL_TEXTURE_UPDATE_BARRIER_BIT",            # 0x00000100
    "GL_BUFFER_UPDATE_BARRIER_BIT",             # 0x00000200
    "GL_FRAMEBUFFER_BARRIER_BIT",               # 0x00000400
    "GL_TRANSFORM_FEEDBACK_BARRIER_BIT",        # 0x00000800
    "GL_ATOMIC_COUNTER_BARRIER_BIT",            # 0x00001000
    "GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT",      # 0x00004000
    "GL_QUERY_BUFFER_BARRIER_BIT",              # 0x00008000
])

