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


"""GLES API description.
"""

import os
from .gltypes import *
import xml.etree.ElementTree as ET

# Build up set of functions from XML for validation
script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/opengl-registry/xml/gl.xml'))
root = tree.getroot()
commands = set()
for command in root.find('commands'):
    proto = command.find('proto')
    name = proto.find('name').text
    commands.add(name)
commands.add('glCreateClientSideBuffer')
commands.add('glDeleteClientSideBuffer')
commands.add('glClientSideBufferData')
commands.add('glClientSideBufferSubData')
commands.add('glCopyClientSideBuffer')
commands.add('glPatchClientSideBuffer')
commands.add('glGenGraphicBuffer_ARM')
commands.add('glGraphicBufferData_ARM')
commands.add('glDeleteGraphicBuffer_ARM')
commands.add('paMandatoryExtensions')
commands.add('glAssertBuffer_ARM')
commands.add('glStateDump_ARM')
commands.add('glLinkProgram2')

# We're stuck supporting these even though it is not a real extension and nobody should use them
commands.add('glShadingRateARM')
commands.add('glShadingRateCombinerOpsARM')
commands.add('glFramebufferShadingRateARM')

GLfixed = Alias("GLfixed", Int32)
GLclampx = Alias("GLclampx", Int32)


def GlFunction(*args, **kwargs):
    kwargs.setdefault('call', 'GL_APIENTRY')
    assert args[1] in commands, 'Function %s does not exist in the Khronos XML!' % args[1]
    return Function(*args, **kwargs)

def InGlString(charType, length, argName):
    # Helper function to describe input strings, where string length can be
    # passed as argument.
    lengthExpr = '((%s) >= 0 ? (%s) : strlen(%s))' % (length, length, argName)
    return In(String(Const(charType), lengthExpr), argName)

def OutGlString(charType, lengthPtr, argName):
    # Helper function to describe output strings, where string length can be
    # returned as a pointer.
    lengthExpr = '((%s) ? *(%s) : strlen(%s))' % (lengthPtr, lengthPtr, argName)
    return Out(String(charType, lengthExpr), argName)

GLIntAttrib = Alias("GLint", GLint)
GLAttribList = Array(Const(GLIntAttrib), "_AttribPairList_size(attrib_list, GL_NONE)")

# OpenGL ES specific functions
gles_functions = [
    ##########################################################################################################################################
    ##########################################################################################################################################
    # gles 2 apis
    GlFunction(Void, "glActiveTexture", [(GLenum, "texture")]),
    GlFunction(Void, "glAttachShader", [(GLprogram, "program"), (GLshader, "shader")]),

    GlFunction(Void, "glBindAttribLocation", [(GLprogram, "program"), (GLuint, "index"), (Const(GLstring), "name")]),
    GlFunction(Void, "glBindBuffer", [(GLenum, "target"), (GLbuffer, "buffer")]),
    GlFunction(Void, "glBindFramebuffer", [(GLenum, "target"), (GLframebuffer, "framebuffer")]),
    GlFunction(Void, "glBindRenderbuffer", [(GLenum, "target"), (GLrenderbuffer, "renderbuffer")]),
    GlFunction(Void, "glBindTexture", [(GLenum, "target"), (GLtexture, "texture")]),
    GlFunction(Void, "glBlendColor", [(GLclampf, "red"), (GLclampf, "green"), (GLclampf, "blue"), (GLclampf, "alpha")]),
    GlFunction(Void, "glBlendEquation", [(GLenum, "mode")]),
    GlFunction(Void, "glBlendEquationSeparate", [(GLenum, "modeRGB"), (GLenum, "modeAlpha")]),
    GlFunction(Void, "glBlendFunc", [(GLenum, "sfactor"), (GLenum, "dfactor")]),
    GlFunction(Void, "glBlendFuncSeparate", [(GLenum, "sfactorRGB"), (GLenum, "dfactorRGB"), (GLenum, "sfactorAlpha"), (GLenum, "dfactorAlpha")]),
    GlFunction(Void, "glBufferData", [(GLenum, "target"), (GLsizeiptr, "size"), (Blob(Const(GLvoid), "size"), "data"), (GLenum, "usage")]),
    GlFunction(Void, "glBufferSubData", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "size"), (Blob(Const(GLvoid), "size"), "data")]),

    GlFunction(GLenum, "glCheckFramebufferStatus", [(GLenum, "target")]),
    GlFunction(Void, "glClear", [(GLbitfield_attrib, "mask")]),
    GlFunction(Void, "glClearColor", [(GLclampf, "red"), (GLclampf, "green"), (GLclampf, "blue"), (GLclampf, "alpha")]),
    GlFunction(Void, "glClearDepthf", [(GLclampf, "d")]),
    GlFunction(Void, "glClearStencil", [(GLint, "s")]),
    GlFunction(Void, "glColorMask", [(GLboolean, "red"), (GLboolean, "green"), (GLboolean, "blue"), (GLboolean, "alpha")]),
    GlFunction(Void, "glCompileShader", [(GLshader, "shader")]),
    GlFunction(Void, "glCompressedTexImage2D", [(GLenum, "target"), (GLint, "level"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLint, "border"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glCompressedTexSubImage2D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glCopyTexImage2D", [(GLenum, "target"), (GLint, "level"), (GLenum, "internalformat"), (GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height"), (GLint, "border")]),
    GlFunction(Void, "glCopyTexSubImage2D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(GLprogram, "glCreateProgram", []),
    GlFunction(GLshader, "glCreateShader", [(GLenum, "type")]),
    GlFunction(Void, "glCullFace", [(GLenum, "mode")]),

    GlFunction(Void, "glDeleteBuffers", [(GLsizei, "n"), (Array(Const(GLbuffer), "n"), "buffers")]),
    GlFunction(Void, "glDeleteFramebuffers", [(GLsizei, "n"), (Array(Const(GLframebuffer), "n"), "framebuffers")]),
    GlFunction(Void, "glDeleteProgram", [(GLprogram, "program")]),
    GlFunction(Void, "glDeleteRenderbuffers", [(GLsizei, "n"), (Array(Const(GLrenderbuffer), "n"), "renderbuffers")]),
    GlFunction(Void, "glDeleteShader", [(GLshader, "shader")]),
    GlFunction(Void, "glDeleteTextures", [(GLsizei, "n"), (Array(Const(GLtexture), "n"), "textures")]),
    GlFunction(Void, "glDepthFunc", [(GLenum, "func")]),
    GlFunction(Void, "glDepthMask", [(GLboolean, "flag")]),
    GlFunction(Void, "glDepthRangef", [(GLclampf, "n"), (GLclampf, "f")]),
    GlFunction(Void, "glDetachShader", [(GLprogram, "program"), (GLshader, "shader")]),
    GlFunction(Void, "glDisable", [(GLenum, "cap")]),
    GlFunction(Void, "glDisableVertexAttribArray", [(GLattribLocation, "index")]),
    GlFunction(Void, "glDrawArrays", [(GLenum_mode, "mode"), (GLint, "first"), (GLsizei, "count")]),
    GlFunction(Void, "glDrawElements", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices")]),

    GlFunction(Void, "glEnable", [(GLenum, "cap")]),
    GlFunction(Void, "glEnableVertexAttribArray", [(GLattribLocation, "index")]),

    GlFunction(Void, "glFinish", []),
    GlFunction(Void, "glFlush", []),
    GlFunction(Void, "glFramebufferRenderbuffer", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "renderbuffertarget"), (GLrenderbuffer, "renderbuffer")]),
    GlFunction(Void, "glFramebufferTexture2D", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "textarget"), (GLtexture, "texture"), (GLint, "level")]),
    GlFunction(Void, "glFrontFace", [(GLenum, "mode")]),

    GlFunction(Void, "glGenBuffers", [(GLsizei, "n"), Out(Array(GLbuffer, "n"), "buffer")]),
    GlFunction(Void, "glGenFramebuffers", [(GLsizei, "n"), Out(Array(GLframebuffer, "n"), "framebuffers")]),
    GlFunction(Void, "glGenRenderbuffers", [(GLsizei, "n"), Out(Array(GLrenderbuffer, "n"), "renderbuffers")]),
    GlFunction(Void, "glGenTextures", [(GLsizei, "n"), Out(Array(GLtexture, "n"), "textures")]),
    GlFunction(Void, "glGenerateMipmap", [(GLenum, "target")]),
    GlFunction(Void, "glGetBooleanv", [(GLenum, "pname"), Out(Array(GLboolean, "_gl_param_size(pname)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetFloatv", [(GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetIntegerv", [(GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetActiveAttrib", [(GLprogram, "program"), (GLattribLocation, "index"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), Out(Pointer(GLint), "size"), Out(Pointer(GLenum), "type"), OutGlString(GLchar, "length", "name")], sideeffects=False),
    GlFunction(Void, "glGetActiveUniform", [(GLprogram, "program"), (GLuint, "index"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), Out(Pointer(GLint), "size"), Out(Pointer(GLenum), "type"), OutGlString(GLchar, "length", "name")], sideeffects=False),
    GlFunction(Void, "glGetAttachedShaders", [(GLprogram, "program"), (GLsizei, "maxCount"), Out(Pointer(GLsizei), "count"), Out(Array(GLuint, "(count ? *count : maxCount)"), "obj")], sideeffects=False),
    GlFunction(GLattribLocation, "glGetAttribLocation", [(GLprogram, "program"), (Const(GLstring), "name")]),
    GlFunction(Void, "glGetBufferParameteriv", [(GLenum, "target"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(GLenum_error, "glGetError", [], sideeffects=False),
    GlFunction(Void, "glGetFramebufferAttachmentParameteriv", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetProgramInfoLog", [(GLprogram, "program"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "infoLog")], sideeffects=False),
    GlFunction(Void, "glGetProgramiv", [(GLprogram, "program"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetRenderbufferParameteriv", [(GLenum, "target"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetShaderInfoLog", [(GLshader, "shader"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "infoLog")], sideeffects=False),
    GlFunction(Void, "glGetShaderPrecisionFormat", [(GLenum, "shadertype"), (GLenum, "precisiontype"), Out(Array(GLint, 2), "range"), Out(Array(GLint, 2), "precision")], sideeffects=False),
    GlFunction(Void, "glGetShaderSource", [(GLshader, "shader"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "source")], sideeffects=False),
    GlFunction(Void, "glGetShaderiv", [(GLshader, "shader"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(String(Const(GLubyte)), "glGetString", [(GLenum, "name")], sideeffects=False),
    GlFunction(Void, "glGetTexParameterfv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexParameteriv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetUniformfv", [(GLprogram, "program"), (GLuniformLocation, "location"), Out(OpaquePointer(GLfloat), "params")], sideeffects=False),
    GlFunction(Void, "glGetUniformiv", [(GLprogram, "program"), (GLuniformLocation, "location"), Out(OpaquePointer(GLint), "params")], sideeffects=False),
    GlFunction(GLuniformLocation, "glGetUniformLocation", [(GLprogram, "program"), (Const(GLstring), "name")]),
    GlFunction(Void, "glGetVertexAttribfv", [(GLattribLocation, "index"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetVertexAttribiv", [(GLattribLocation, "index"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetVertexAttribPointerv", [(GLattribLocation, "index"), (GLenum, "pname"), Out(Pointer(GLpointer), "pointer")], sideeffects=False),

    GlFunction(Void, "glHint", [(GLenum, "target"), (GLenum, "mode")]),

    GlFunction(GLboolean, "glIsBuffer", [(GLbuffer, "buffer")], sideeffects=False),
    GlFunction(GLboolean, "glIsEnabled", [(GLenum, "cap")], sideeffects=False),
    GlFunction(GLboolean, "glIsFramebuffer", [(GLframebuffer, "framebuffer")], sideeffects=False),
    GlFunction(GLboolean, "glIsProgram", [(GLprogram, "program")], sideeffects=False),
    GlFunction(GLboolean, "glIsRenderbuffer", [(GLrenderbuffer, "renderbuffer")], sideeffects=False),
    GlFunction(GLboolean, "glIsShader", [(GLshader, "shader")], sideeffects=False),
    GlFunction(GLboolean, "glIsTexture", [(GLtexture, "texture")], sideeffects=False),

    GlFunction(Void, "glLineWidth", [(GLfloat, "width")]),
    GlFunction(Void, "glLinkProgram", [(GLprogram, "program")]),

    GlFunction(Void, "glPixelStorei", [(GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glPolygonOffset", [(GLfloat, "factor"), (GLfloat, "units")]),

    GlFunction(Void, "glReadPixels", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLenum, "type"), Out(OpaquePointer(GLvoid, "_gl_image_size(format, type, width, height, 1)"), "pixels")]),
    GlFunction(Void, "glReleaseShaderCompiler", []),
    GlFunction(Void, "glRenderbufferStorage", [(GLenum, "target"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),

    GlFunction(Void, "glSampleCoverage", [(GLclampf, "value"), (GLboolean, "invert")]),
    GlFunction(Void, "glScissor", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glShaderBinary", [(GLsizei, "count"), (Array(Const(GLuint), "count"), "shaders"), (GLenum, "binaryformat"), (Blob(Const(GLvoid), "length"), "binary"), (GLsizei, "length")]),
    GlFunction(Void, "glShaderSource", [(GLshader, "shader"), (GLsizei, "count"), (Const(Array(Const(GLstring), "count")), "string"), (Array(Const(GLint), "count"), "length")]),
    GlFunction(Void, "glStencilFunc", [(GLenum, "func"), (GLint, "ref"), (GLuint, "mask")]),
    GlFunction(Void, "glStencilFuncSeparate", [(GLenum, "face"), (GLenum, "func"), (GLint, "ref"), (GLuint, "mask")]),
    GlFunction(Void, "glStencilMask", [(GLuint, "mask")]),
    GlFunction(Void, "glStencilMaskSeparate", [(GLenum, "face"), (GLuint, "mask")]),
    GlFunction(Void, "glStencilOp", [(GLenum, "fail"), (GLenum, "zfail"), (GLenum, "zpass")]),
    GlFunction(Void, "glStencilOpSeparate", [(GLenum, "face"), (GLenum, "sfail"), (GLenum, "dpfail"), (GLenum, "dppass")]),

    GlFunction(Void, "glTexImage2D", [(GLenum, "target"), (GLint, "level"), (GLenum_int, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLint, "border"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexImage2D_size(format, type, width, height)"), "pixels")]),
    GlFunction(Void, "glTexParameterf", [(GLenum, "target"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glTexParameterfv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexParameteri", [(GLenum, "target"), (GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glTexParameteriv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexSubImage2D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexSubImage2D_size(format, type, width, height)"), "pixels")]),

    GlFunction(Void, "glUniform1f", [(GLuniformLocation, "location"), (GLfloat, "v0")]),
    GlFunction(Void, "glUniform2f", [(GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1")]),
    GlFunction(Void, "glUniform3f", [(GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2")]),
    GlFunction(Void, "glUniform4f", [(GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2"), (GLfloat, "v3")]),
    GlFunction(Void, "glUniform1i", [(GLuniformLocation, "location"), (GLint, "v0")]),
    GlFunction(Void, "glUniform2i", [(GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1")]),
    GlFunction(Void, "glUniform3i", [(GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2")]),
    GlFunction(Void, "glUniform4i", [(GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2"), (GLint, "v3")]),
    GlFunction(Void, "glUniform1fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count"), "value")]),
    GlFunction(Void, "glUniform2fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*2"), "value")]),
    GlFunction(Void, "glUniform3fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*3"), "value")]),
    GlFunction(Void, "glUniform4fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*4"), "value")]),
    GlFunction(Void, "glUniform1iv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count"), "value")]),
    GlFunction(Void, "glUniform2iv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*2"), "value")]),
    GlFunction(Void, "glUniform3iv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*3"), "value")]),
    GlFunction(Void, "glUniform4iv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*4"), "value")]),
    GlFunction(Void, "glUniformMatrix2fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*2"), "value")]),
    GlFunction(Void, "glUniformMatrix3fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*3"), "value")]),
    GlFunction(Void, "glUniformMatrix4fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*4"), "value")]),
    GlFunction(Void, "glUseProgram", [(GLprogram, "program")]),

    GlFunction(Void, "glValidateProgram", [(GLprogram, "program")]),
    GlFunction(Void, "glVertexAttrib1f", [(GLattribLocation, "index"), (GLfloat, "x")]),
    GlFunction(Void, "glVertexAttrib2f", [(GLattribLocation, "index"), (GLfloat, "x"), (GLfloat, "y")]),
    GlFunction(Void, "glVertexAttrib3f", [(GLattribLocation, "index"), (GLfloat, "x"), (GLfloat, "y"), (GLfloat, "z")]),
    GlFunction(Void, "glVertexAttrib4f", [(GLattribLocation, "index"), (GLfloat, "x"), (GLfloat, "y"), (GLfloat, "z"), (GLfloat, "w")]),
    GlFunction(Void, "glVertexAttrib1fv", [(GLattribLocation, "index"), (Pointer(Const(GLfloat)), "v")]),
    GlFunction(Void, "glVertexAttrib2fv", [(GLattribLocation, "index"), (Array(Const(GLfloat), 2), "v")]),
    GlFunction(Void, "glVertexAttrib3fv", [(GLattribLocation, "index"), (Array(Const(GLfloat), 3), "v")]),
    GlFunction(Void, "glVertexAttrib4fv", [(GLattribLocation, "index"), (Array(Const(GLfloat), 4), "v")]),
    GlFunction(Void, "glVertexAttribPointer", [(GLattribLocation, "index"), (GLint, "size"), (GLenum, "type"), (GLboolean, "normalized"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),
    GlFunction(Void, "glViewport", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),



    ##########################################################################################################################################
    ##########################################################################################################################################
    # gles 1 apis
    GlFunction(Void, "glAlphaFunc", [(GLenum, "func"), (GLclampf, "ref")]),
    GlFunction(Void, "glAlphaFuncx", [(GLenum, "func"), (GLclampx, "ref")]),

    GlFunction(Void, "glClearColorx", [(GLclampx, "red"), (GLclampx, "green"), (GLclampx, "blue"), (GLclampx, "alpha")]),
    GlFunction(Void, "glClearDepthx", [(GLclampx, "depth")]),
    GlFunction(Void, "glClientActiveTexture", [(GLenum, "texture")]),
    GlFunction(Void, "glClipPlanef", [(GLenum, "plane"), (Array(Const(GLfloat), 4), "equation")]),
    GlFunction(Void, "glClipPlanex", [(GLenum, "plane"), (Array(Const(GLfixed), 4), "equation")]),
    GlFunction(Void, "glColor4f", [(GLfloat, "red"), (GLfloat, "green"), (GLfloat, "blue"), (GLfloat, "alpha")]),
    GlFunction(Void, "glColor4ub", [(GLubyte, "red"), (GLubyte, "green"), (GLubyte, "blue"), (GLubyte, "alpha")]),
    GlFunction(Void, "glColor4x", [(GLfixed, "red"), (GLfixed, "green"), (GLfixed, "blue"), (GLfixed, "alpha")]),
    GlFunction(Void, "glColorPointer", [(GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),

    GlFunction(Void, "glDepthRangex", [(GLclampx, "zNear"), (GLclampx, "zFar")]),
    GlFunction(Void, "glDisableClientState", [(GLenum, "array")]),

    GlFunction(Void, "glEnableClientState", [(GLenum, "array")]),

    GlFunction(Void, "glFogf", [(GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glFogx", [(GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glFogfv", [(GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glFogxv", [(GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glFrustumf", [(GLfloat, "left"), (GLfloat, "right"), (GLfloat, "bottom"), (GLfloat, "top"), (GLfloat, "zNear"), (GLfloat, "zFar")]),
    GlFunction(Void, "glFrustumx", [(GLfixed, "left"), (GLfixed, "right"), (GLfixed, "bottom"), (GLfixed, "top"), (GLfixed, "zNear"), (GLfixed, "zFar")]),

    GlFunction(Void, "glGetFixedv", [(GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetClipPlanef", [(GLenum, "plane"), Out(Array(GLfloat, 4), "equation")], sideeffects=False),
    GlFunction(Void, "glGetClipPlanex", [(GLenum, "plane"), Out(Array(GLfixed, 4), "equation")], sideeffects=False),
    GlFunction(Void, "glGetLightfv", [(GLenum, "light"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetLightxv", [(GLenum, "light"), (GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetMaterialfv", [(GLenum, "face"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetMaterialxv", [(GLenum, "face"), (GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetPointerv", [(GLenum, "pname"), Out(Pointer(GLpointer), "params")], sideeffects=False),
    GlFunction(Void, "glGetPointervKHR", [(GLenum, "pname"), Out(Pointer(GLpointer), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexEnvfv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexEnviv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexEnvxv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexParameterxv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),

    GlFunction(Void, "glLightf", [(GLenum, "light"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glLightfv", [(GLenum, "light"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glLightx", [(GLenum, "light"), (GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glLightxv", [(GLenum, "light"), (GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glLightModelf", [(GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glLightModelx", [(GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glLightModelfv", [(GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glLightModelxv", [(GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glLineWidthx", [(GLfixed, "width")]),
    GlFunction(Void, "glLoadIdentity", []),
    GlFunction(Void, "glLoadMatrixf", [(Array(Const(GLfloat), 16), "m")]),
    GlFunction(Void, "glLoadMatrixx", [(Array(Const(GLfixed), 16), "m")]),
    GlFunction(Void, "glLogicOp", [(GLenum, "opcode")]),

    GlFunction(Void, "glMaterialf", [(GLenum, "face"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glMaterialx", [(GLenum, "face"), (GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glMaterialfv", [(GLenum, "face"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glMaterialxv", [(GLenum, "face"), (GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glMatrixMode", [(GLenum, "mode")]),
    GlFunction(Void, "glMultMatrixf", [(Array(Const(GLfloat), 16), "m")]),
    GlFunction(Void, "glMultMatrixx", [(Array(Const(GLfixed), 16), "m")]),
    GlFunction(Void, "glMultiTexCoord4f", [(GLenum, "target"), (GLfloat, "s"), (GLfloat, "t"), (GLfloat, "r"), (GLfloat, "q")]),
    GlFunction(Void, "glMultiTexCoord4x", [(GLenum, "target"), (GLfixed, "s"), (GLfixed, "t"), (GLfixed, "r"), (GLfixed, "q")]),

    GlFunction(Void, "glNormal3f", [(GLfloat, "nx"), (GLfloat, "ny"), (GLfloat, "nz")]),
    GlFunction(Void, "glNormal3x", [(GLfixed, "nx"), (GLfixed, "ny"), (GLfixed, "nz")]),
    GlFunction(Void, "glNormalPointer", [(GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),

    GlFunction(Void, "glOrthof", [(GLfloat, "left"), (GLfloat, "right"), (GLfloat, "bottom"), (GLfloat, "top"), (GLfloat, "zNear"), (GLfloat, "zFar")]),
    GlFunction(Void, "glOrthox", [(GLfixed, "left"), (GLfixed, "right"), (GLfixed, "bottom"), (GLfixed, "top"), (GLfixed, "zNear"), (GLfixed, "zFar")]),

    GlFunction(Void, "glPointParameterf", [(GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glPointParameterx", [(GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glPointParameterfv", [(GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glPointParameterxv", [(GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glPointSize", [(GLfloat, "size")]),
    GlFunction(Void, "glPointSizex", [(GLfixed, "size")]),
    GlFunction(Void, "glPointSizePointerOES", [(GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "ptr")]),
    GlFunction(Void, "glPolygonOffsetx", [(GLfixed, "factor"), (GLfixed, "units")]),
    GlFunction(Void, "glPopMatrix", []),
    GlFunction(Void, "glPushMatrix", []),

    GlFunction(Void, "glRotatef", [(GLfloat, "angle"), (GLfloat, "x"), (GLfloat, "y"), (GLfloat, "z")]),
    GlFunction(Void, "glRotatex", [(GLfixed, "angle"), (GLfixed, "x"), (GLfixed, "y"), (GLfixed, "z")]),

    GlFunction(Void, "glSampleCoveragex", [(GLclampx, "value"), (GLboolean, "invert")]),
    GlFunction(Void, "glScalef", [(GLfloat, "x"), (GLfloat, "y"), (GLfloat, "z")]),
    GlFunction(Void, "glScalex", [(GLfixed, "x"), (GLfixed, "y"), (GLfixed, "z")]),
    GlFunction(Void, "glShadeModel", [(GLenum, "mode")]),

    GlFunction(Void, "glTexCoordPointer", [(GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),
    GlFunction(Void, "glTexEnvf", [(GLenum, "target"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glTexEnvi", [(GLenum, "target"), (GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glTexEnvx", [(GLenum, "target"), (GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glTexEnvfv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexEnviv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexEnvxv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexParameterx", [(GLenum, "target"), (GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glTexParameterxv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTranslatef", [(GLfloat, "x"), (GLfloat, "y"), (GLfloat, "z")]),
    GlFunction(Void, "glTranslatex", [(GLfixed, "x"), (GLfixed, "y"), (GLfixed, "z")]),

    GlFunction(Void, "glVertexPointer", [(GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),


    # GL_OES_blend_equation_separate
    GlFunction(Void, "glBlendEquationSeparateOES", [(GLenum, "modeRGB"), (GLenum, "modeAlpha")]),

    # GL_OES_blend_func_separate
    GlFunction(Void, "glBlendFuncSeparateOES", [(GLenum, "sfactorRGB"), (GLenum, "dfactorRGB"), (GLenum, "sfactorAlpha"), (GLenum, "dfactorAlpha")]),

    # GL_OES_blend_subtract
    GlFunction(Void, "glBlendEquationOES", [(GLenum, "mode")]),

    # GL_OES_framebuffer_object
    GlFunction(GLboolean, "glIsRenderbufferOES", [(GLrenderbuffer, "renderbuffer")], sideeffects=False),
    GlFunction(Void, "glBindRenderbufferOES", [(GLenum, "target"), (GLrenderbuffer, "renderbuffer")]),
    GlFunction(Void, "glDeleteRenderbuffersOES", [(GLsizei, "n"), (Array(Const(GLrenderbuffer), "n"), "renderbuffers")]),
    GlFunction(Void, "glGenRenderbuffersOES", [(GLsizei, "n"), Out(Array(GLrenderbuffer, "n"), "renderbuffers")]),
    GlFunction(Void, "glRenderbufferStorageOES", [(GLenum, "target"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glGetRenderbufferParameterivOES", [(GLenum, "target"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(GLboolean, "glIsFramebufferOES", [(GLframebuffer, "framebuffer")], sideeffects=False),
    GlFunction(Void, "glBindFramebufferOES", [(GLenum, "target"), (GLframebuffer, "framebuffer")]),
    GlFunction(Void, "glDeleteFramebuffersOES", [(GLsizei, "n"), (Array(Const(GLframebuffer), "n"), "framebuffers")]),
    GlFunction(Void, "glGenFramebuffersOES", [(GLsizei, "n"), Out(Array(GLframebuffer, "n"), "framebuffers")]),
    GlFunction(GLenum, "glCheckFramebufferStatusOES", [(GLenum, "target")]),
    GlFunction(Void, "glFramebufferTexture2DOES", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "textarget"), (GLtexture, "texture"), (GLint, "level")]),
    GlFunction(Void, "glFramebufferRenderbufferOES", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "renderbuffertarget"), (GLrenderbuffer, "renderbuffer")]),
    GlFunction(Void, "glGetFramebufferAttachmentParameterivOES", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGenerateMipmapOES", [(GLenum, "target")]),

    # GL_OES_matrix_palette
    GlFunction(Void, "glCurrentPaletteMatrixOES", [(GLuint, "index")]),
    GlFunction(Void, "glLoadPaletteFromModelViewMatrixOES", []),
    GlFunction(Void, "glMatrixIndexPointerOES", [(GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),
    GlFunction(Void, "glWeightPointerOES", [(GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),

    # GL_OES_point_size_array

    # GL_OES_query_matrix
    GlFunction(GLbitfield, "glQueryMatrixxOES", [(Array(GLfixed, 16), "mantissa"), (Array(GLint, 16), "exponent")]),

    # GL_OES_texture_cube_map
    GlFunction(Void, "glTexGenfOES", [(GLenum, "coord"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glTexGenfvOES", [(GLenum, "coord"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexGeniOES", [(GLenum, "coord"), (GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glTexGenivOES", [(GLenum, "coord"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexGenxOES", [(GLenum, "coord"), (GLenum, "pname"), (GLfixed, "param")]),
    GlFunction(Void, "glTexGenxvOES", [(GLenum, "coord"), (GLenum, "pname"), (Array(Const(GLfixed), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetTexGenfvOES", [(GLenum, "coord"), (GLenum, "pname"), Out(Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexGenivOES", [(GLenum, "coord"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexGenxvOES", [(GLenum, "coord"), (GLenum, "pname"), Out(Array(GLfixed, "_gl_param_size(pname)"), "params")], sideeffects=False),

    # GL_OES_mapbuffer
    GlFunction(Void, "glGetBufferPointervOES", [(GLenum, "target"), (GLenum, "pname"), Out(Pointer(GLpointer), "params")], sideeffects=False),
    GlFunction(GLpointer, "glMapBufferOES", [(GLenum, "target"), (GLbitfield, "access")]),
    GlFunction(GLboolean, "glUnmapBufferOES", [(GLenum, "target")]),

    # GL_OES_texture_3D
    GlFunction(Void, "glTexImage3DOES", [(GLenum, "target"), (GLint, "level"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLint, "border"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexImage3D_size(format, type, width, height, depth)"), "pixels")]),
    GlFunction(Void, "glTexSubImage3DOES", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexSubImage3D_size(format, type, width, height, depth)"), "pixels")]),
    GlFunction(Void, "glCopyTexSubImage3DOES", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glCompressedTexImage3DOES", [(GLenum, "target"), (GLint, "level"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLint, "border"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glCompressedTexSubImage3DOES", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLenum, "format"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glFramebufferTexture3DOES", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "textarget"), (GLtexture, "texture"), (GLint, "level"), (GLint, "zoffset")]),

    # GL_OES_get_program_binary
    GlFunction(Void, "glGetProgramBinaryOES", [(GLprogram, "program"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), Out(Pointer(GLenum), "binaryFormat"), Out(OpaqueBlob(GLvoid, "length ? *length : bufSize"), "binary")], sideeffects=False),
    GlFunction(Void, "glProgramBinaryOES", [(GLprogram, "program"), (GLenum, "binaryFormat"), (Blob(Const(GLvoid), "length"), "binary"), (GLsizei, "length")]),

    # GL_EXT_discard_framebuffer
    GlFunction(Void, "glDiscardFramebufferEXT", [(GLenum, "target"), (GLsizei, "numAttachments"), (Array(Const(GLenum), "numAttachments"), "attachments")]),

    # GL_OES_vertex_array_object
    GlFunction(Void, "glBindVertexArrayOES", [(GLarray, "array")]),
    GlFunction(Void, "glDeleteVertexArraysOES", [(GLsizei, "n"), (Array(Const(GLarray), "n"), "arrays")]),
    GlFunction(Void, "glGenVertexArraysOES", [(GLsizei, "n"), Out(Array(GLarray, "n"), "arrays")]),
    GlFunction(GLboolean, "glIsVertexArrayOES", [(GLarray, "array")], sideeffects=False),

    # GL_NV_coverage_sample
    GlFunction(Void, "glCoverageMaskNV", [(GLboolean, "mask")]),
    GlFunction(Void, "glCoverageOperationNV", [(GLenum, "operation")]),

    # GL_EXT_multisampled_render_to_texture
    GlFunction(Void, "glRenderbufferStorageMultisampleEXT", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glFramebufferTexture2DMultisampleEXT", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "textarget"), (GLtexture, "texture"), (GLint, "level"), (GLsizei, "samples")]),

    # GL_IMG_multisampled_render_to_texture
    GlFunction(Void, "glRenderbufferStorageMultisampleIMG", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glFramebufferTexture2DMultisampleIMG", [(GLenum, "target"), (GLenum, "attachment"), (GLenum, "textarget"), (GLtexture, "texture"), (GLint, "level"), (GLsizei, "samples")]),

    # GL_APPLE_framebuffer_multisample
    GlFunction(Void, "glRenderbufferStorageMultisampleAPPLE", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glResolveMultisampleFramebufferAPPLE", []),

    # GL_ANGLE_framebuffer_blit
    GlFunction(Void, "glBlitFramebufferANGLE", [(GLint, "srcX0"), (GLint, "srcY0"), (GLint, "srcX1"), (GLint, "srcY1"), (GLint, "dstX0"), (GLint, "dstY0"), (GLint, "dstX1"), (GLint, "dstY1"), (GLbitfield_attrib, "mask"), (GLenum, "filter")]),

    # GL_ANGLE_framebuffer_multisample
    GlFunction(Void, "glRenderbufferStorageMultisampleANGLE", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),

    # GL_NV_draw_buffers
    GlFunction(Void, "glDrawBuffersNV", [(GLsizei, "n"), (Array(Const(UInt), "n"), "bufs")]),

    # GL_NV_read_buffer
    GlFunction(Void, "glReadBufferNV", [(GLenum, "mode")]),

    # GL_EXT_debug_label
    GlFunction(Void, "glLabelObjectEXT", [(GLenum, "type"), (GLuint, "object"), (GLsizei, "length"), (Const(GLstring), "label")]),
    GlFunction(Void, "glGetObjectLabelEXT", [(GLenum, "type"), (GLuint, "object"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), Out(GLstring, "label")], sideeffects=False),

    # GL_EXT_debug_marker
    GlFunction(Void, "glInsertEventMarkerEXT", [(GLsizei, "length"), (String(Const(GLchar), "length ? length : strlen(marker)"), "marker")], sideeffects=True),
    GlFunction(Void, "glPushGroupMarkerEXT", [(GLsizei, "length"), (String(Const(GLchar), "length ? length : strlen(marker)"), "marker")], sideeffects=True),
    GlFunction(Void, "glPopGroupMarkerEXT", [], sideeffects=True),

    # GL_EXT_occlusion_query_boolean
    GlFunction(Void, "glGenQueriesEXT", [(GLsizei, "n"), Out(Array(GLquery, "n"), "ids")]),
    GlFunction(Void, "glDeleteQueriesEXT", [(GLsizei, "n"), (Array(Const(GLquery), "n"), "ids")]),
    GlFunction(GLboolean, "glIsQueryEXT", [(GLquery, "id")], sideeffects=False),
    GlFunction(Void, "glBeginQueryEXT", [(GLenum, "target"), (GLquery, "id")]),
    GlFunction(Void, "glEndQueryEXT", [(GLenum, "target")]),
    GlFunction(Void, "glGetQueryivEXT", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetQueryObjectuivEXT", [(GLquery, "id"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")]),

    # GL_EXT_separate_shader_objects
    GlFunction(Void, "glUseProgramStagesEXT", [(GLpipeline, "pipeline"), (GLbitfield_shader, "stages"), (GLprogram, "program")]),
    GlFunction(Void, "glActiveShaderProgramEXT", [(GLpipeline, "pipeline"), (GLprogram, "program")]),
    GlFunction(GLprogram, "glCreateShaderProgramvEXT", [(GLenum, "type"), (GLsizei, "count"), (Array(GLstringConst, "count"), "strings")]),
    GlFunction(Void, "glBindProgramPipelineEXT", [(GLpipeline, "pipeline")]),
    GlFunction(Void, "glDeleteProgramPipelinesEXT", [(GLsizei, "n"), (Array(Const(GLuint), "n"), "pipelines")]),
    GlFunction(Void, "glGenProgramPipelinesEXT", [(GLsizei, "n"), Out(Array(GLpipeline, "n"), "pipelines")]),
    GlFunction(GLboolean, "glIsProgramPipelineEXT", [(GLpipeline, "pipeline")], sideeffects=False),
    GlFunction(Void, "glProgramParameteriEXT", [(GLprogram, "program"), (GLenum, "pname"), (GLint, "value")]),
    GlFunction(Void, "glGetProgramPipelineivEXT", [(GLpipeline, "pipeline"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glProgramUniform1fEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0")]),
    GlFunction(Void, "glProgramUniform1fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count"), "value")]),
    GlFunction(Void, "glProgramUniform1iEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0")]),
    GlFunction(Void, "glProgramUniform1ivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count"), "value")]),
    GlFunction(Void, "glProgramUniform1uiEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0")]),
    GlFunction(Void, "glProgramUniform1uivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count"), "value")]),
    GlFunction(Void, "glProgramUniform2fEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1")]),
    GlFunction(Void, "glProgramUniform2fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform2iEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1")]),
    GlFunction(Void, "glProgramUniform2ivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform2uiEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1")]),
    GlFunction(Void, "glProgramUniform2uivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform3fEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2")]),
    GlFunction(Void, "glProgramUniform3fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform3iEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2")]),
    GlFunction(Void, "glProgramUniform3ivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform3uiEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2")]),
    GlFunction(Void, "glProgramUniform3uivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform4fEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2"), (GLfloat, "v3")]),
    GlFunction(Void, "glProgramUniform4fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniform4iEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2"), (GLint, "v3")]),
    GlFunction(Void, "glProgramUniform4ivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniform4uiEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2"), (GLuint, "v3")]),
    GlFunction(Void, "glProgramUniform4uivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2x3fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*3"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2x4fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*3"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3x2fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3x4fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4x2fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4x3fvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*3"), "value")]),
    GlFunction(Void, "glValidateProgramPipelineEXT", [(GLpipeline, "pipeline")]),
    GlFunction(Void, "glGetProgramPipelineInfoLogEXT", [(GLpipeline, "pipeline"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (GLstring, "infoLog")], sideeffects=False),

    # GL_OES_draw_texture
    GlFunction(Void, "glDrawTexiOES", [(GLint, "x"), (GLint, "y"), (GLint, "z"), (GLint, "width"), (GLint, "height")]),

    # GL_KHR_blend_equation_advanced
    GlFunction(Void, "glBlendBarrierKHR", []),
    GlFunction(Void, "glBlendBarrierNV", []),

    # Special functions to modify client-side memory
    GlFunction(GLuint, "glCreateClientSideBuffer", []),
    GlFunction(Void, "glDeleteClientSideBuffer", [(GLuint, "name")]),
    GlFunction(Void, "glClientSideBufferData", [(GLuint, "name"), (GLsizei, "size"), (Blob(GLubyte, "size"), "data")]),
    GlFunction(Void, "glClientSideBufferSubData", [(GLuint, "name"), (GLsizei, "offset"), (GLsizei, "size"), (Blob(GLubyte, "size"), "data")]),
    GlFunction(Void, "glCopyClientSideBuffer", [(GLenum, "target"), (GLuint, "name")]),
    GlFunction(Void, "glPatchClientSideBuffer", [(GLenum, "target"), (GLsizei, "size"), (Blob(GLubyte, "size"), "data")]),

    # Special functions to use Android GraphicBuffer or Linux DMA buffer
    GlFunction(GLuint, "glGenGraphicBuffer_ARM", [(GLuint, "width"), (GLuint, "height"), (GLint, "pixelFormat"), (GLuint, "usage")]),
    GlFunction(Void, "glGraphicBufferData_ARM", [(GLuint, "name"), (GLsizei, "size"), (Blob(GLubyte, "size"), "data")]),
    GlFunction(Void, "glDeleteGraphicBuffer_ARM", [(GLuint, "name")]),

    # Special function used to store mandatory extensions
    GlFunction(Void, "paMandatoryExtensions", [(GLsizei, "count"), (Const(Array(Const(GLstring), "count")), "stringArr")]),

    # Special function used to store MD5 checksum of buffers for integration testing
    GlFunction(Void, "glAssertBuffer_ARM", [(GLenum, "target"), (GLsizei, "offset"), (GLsizei, "size"), (Const(GLstring), "md5")]),

    # Special function to instruct a state dump at this point
    GlFunction(Void, "glStateDump_ARM", []),

    # GL_KHR_debug
    GlFunction(Void, "glDebugMessageCallbackKHR", [(GLDEBUGPROCKHR, "callback"), (OpaquePointer(Const(Void)), "userParam")], sideeffects=False),
    GlFunction(Void, "glDebugMessageControlKHR", [(GLenum, "source"), (GLenum, "type"), (GLenum, "severity"), (GLsizei, "count"), (Array(Const(GLuint), "count"), "ids"), (GLboolean, "enabled")], sideeffects=True),
    GlFunction(Void, "glDebugMessageInsertKHR", [(GLenum, "source"), (GLenum, "type"), (GLuint, "id"), (GLenum, "severity"), (GLsizei, "length"), InGlString(GLchar, "length", "buf")], sideeffects=True),
    GlFunction(Void, "glPushDebugGroupKHR", [(GLenum, "source"), (GLuint, "id"), (GLsizei, "length"), InGlString(GLchar, "length", "message")], sideeffects=True),
    GlFunction(Void, "glPopDebugGroupKHR", [], sideeffects=True),
    GlFunction(Void, "glObjectLabelKHR", [(GLenum, "identifier"), (GLuint, "name"), (GLsizei, "length"), InGlString(GLchar, "length", "label")], sideeffects=True),
    GlFunction(Void, "glGetObjectLabelKHR", [(GLenum, "identifier"), (GLuint, "name"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "label")], sideeffects=False),
    GlFunction(Void, "glObjectPtrLabelKHR", [(OpaquePointer(Const(Void)), "ptr"), (GLsizei, "length"), InGlString(GLchar, "length", "label")], sideeffects=True),
    GlFunction(GLuint, "glGetDebugMessageLogKHR", [(GLuint, "count"), (GLsizei, "bufsize"), Out(Array(GLenum, "count"), "sources"), Out(Array(GLenum, "count"), "types"), Out(Array(GLuint, "count"), "ids"), Out(Array(GLenum, "count"), "severities"), Out(Array(GLsizei, "count"), "lengths"), Out(String(GLchar, "_glGetDebugMessageLog_length(messageLog, lengths, _result)"), "messageLog")], sideeffects=False, fail=0),
    GlFunction(Void, "glDebugMessageCallback", [(GLDEBUGPROCKHR, "callback"), (OpaquePointer(Const(Void)), "userParam")], sideeffects=False),
    GlFunction(Void, "glDebugMessageControl", [(GLenum, "source"), (GLenum, "type"), (GLenum, "severity"), (GLsizei, "count"), (Array(Const(GLuint), "count"), "ids"), (GLboolean, "enabled")], sideeffects=True),
    GlFunction(Void, "glDebugMessageInsert", [(GLenum, "source"), (GLenum, "type"), (GLuint, "id"), (GLenum, "severity"), (GLsizei, "length"), InGlString(GLchar, "length", "buf")], sideeffects=True),
    GlFunction(Void, "glPushDebugGroup", [(GLenum, "source"), (GLuint, "id"), (GLsizei, "length"), InGlString(GLchar, "length", "message")], sideeffects=True),
    GlFunction(Void, "glPopDebugGroup", [], sideeffects=True),
    GlFunction(Void, "glObjectLabel", [(GLenum, "identifier"), (GLuint, "name"), (GLsizei, "length"), InGlString(GLchar, "length", "label")], sideeffects=True),
    GlFunction(Void, "glGetObjectLabel", [(GLenum, "identifier"), (GLuint, "name"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "label")], sideeffects=False),
    GlFunction(Void, "glObjectPtrLabel", [(OpaquePointer(Const(Void)), "ptr"), (GLsizei, "length"), InGlString(GLchar, "length", "label")], sideeffects=True),
    GlFunction(GLuint, "glGetDebugMessageLog", [(GLuint, "count"), (GLsizei, "bufsize"), Out(Array(GLenum, "count"), "sources"), Out(Array(GLenum, "count"), "types"), Out(Array(GLuint, "count"), "ids"), Out(Array(GLenum, "count"), "severities"), Out(Array(GLsizei, "count"), "lengths"), Out(String(GLchar, "_glGetDebugMessageLog_length(messageLog, lengths, _result)"), "messageLog")], sideeffects=False, fail=0),

    # GL_OES_sample_shading
    GlFunction(Void, "glMinSampleShadingOES", [(GLfloat, "value")]),

    # GL_OES_texture_storage_multisample_2d_array
    GlFunction(Void, "glTexStorage3DMultisampleOES", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLboolean, "fixedsamplelocations")]),

    # GL_EXT_draw_buffers_indexed
    GlFunction(Void, "glEnableiEXT", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(Void, "glDisableiEXT", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(GLboolean, "glIsEnablediEXT", [(GLenum, "target"), (GLuint, "index")], sideeffects=False),
    GlFunction(Void, "glBlendEquationiEXT", [(GLuint, "buf"), (GLenum, "mode")]),
    GlFunction(Void, "glBlendEquationSeparateiEXT", [(GLuint, "buf"), (GLenum, "modeRGB"), (GLenum, "modeAlpha")]),
    GlFunction(Void, "glBlendFunciEXT", [(GLuint, "buf"), (GLenum, "src"), (GLenum, "dst")]),
    GlFunction(Void, "glBlendFuncSeparateiEXT", [(GLuint, "buf"), (GLenum, "srcRGB"), (GLenum, "dstRGB"), (GLenum, "srcAlpha"), (GLenum, "dstAlpha")]),
    GlFunction(Void, "glColorMaskiEXT", [(GLuint, "index"), (GLboolean, "r"), (GLboolean, "g"), (GLboolean, "b"), (GLboolean, "a")]),

    # GL_OES_draw_buffers_indexed
    GlFunction(Void, "glEnableiOES", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(Void, "glDisableiOES", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(GLboolean, "glIsEnablediOES", [(GLenum, "target"), (GLuint, "index")], sideeffects=False),
    GlFunction(Void, "glBlendEquationiOES", [(GLuint, "buf"), (GLenum, "mode")]),
    GlFunction(Void, "glBlendEquationSeparateiOES", [(GLuint, "buf"), (GLenum, "modeRGB"), (GLenum, "modeAlpha")]),
    GlFunction(Void, "glBlendFunciOES", [(GLuint, "buf"), (GLenum, "src"), (GLenum, "dst")]),
    GlFunction(Void, "glBlendFuncSeparateiOES", [(GLuint, "buf"), (GLenum, "srcRGB"), (GLenum, "dstRGB"), (GLenum, "srcAlpha"), (GLenum, "dstAlpha")]),
    GlFunction(Void, "glColorMaskiOES", [(GLuint, "index"), (GLboolean, "r"), (GLboolean, "g"), (GLboolean, "b"), (GLboolean, "a")]),

    # GL_OES_viewport_array
    GlFunction(Void, "glViewportArrayvOES", [(GLuint, "first"), (GLsizei, "count"), (Array(Const(GLfloat), "count*4"), "v")]),
    GlFunction(Void, "glViewportIndexedfOES", [(GLuint, "index"), (GLfloat, "x"), (GLfloat, "y"), (GLfloat, "w"), (GLfloat, "h")]),
    GlFunction(Void, "glViewportIndexedfvOES", [(GLuint, "index"), (Array(Const(GLfloat), 4), "v")]),
    GlFunction(Void, "glScissorArrayvOES", [(GLuint, "first"), (GLsizei, "count"), (Array(Const(GLint), "count*4"), "v")]),
    GlFunction(Void, "glScissorIndexedOES", [(GLuint, "index"), (GLint, "left"), (GLint, "bottom"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glScissorIndexedvOES", [(GLuint, "index"), (Array(Const(GLint), 4), "v")]),
    GlFunction(Void, "glDepthRangeArrayfvOES", [(GLuint, "first"), (GLsizei, "count"), (Array(Const(GLfloat), "count*2"), "v")]),
    GlFunction(Void, "glDepthRangeIndexedfOES", [(GLuint, "index"), (GLfloat, "n"), (GLfloat, "f")]),
    GlFunction(Void, "glGetFloati_vOES", [(GLenum, "target"), (GLuint, "index"), Out(Array(GLfloat, "_gl_param_size(target)"), "data")], sideeffects=False),

    # GL_EXT_geometry_shader
    GlFunction(Void, "glFramebufferTextureEXT", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level")]),

    # GL_OES_geometry_shader
    GlFunction(Void, "glFramebufferTextureOES", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level")]),

    # GL_EXT_primitive_bounding_box
    GlFunction(Void, "glPrimitiveBoundingBoxEXT", [(GLfloat, "minX"), (GLfloat, "minY"), (GLfloat, "minZ"), (GLfloat, "minW"), (GLfloat, "maxX"), (GLfloat, "maxY"), (GLfloat, "maxZ"), (GLfloat, "maxW")]),

    # GL_OES_primitive_bounding_box
    GlFunction(Void, "glPrimitiveBoundingBoxOES", [(GLfloat, "minX"), (GLfloat, "minY"), (GLfloat, "minZ"), (GLfloat, "minW"), (GLfloat, "maxX"), (GLfloat, "maxY"), (GLfloat, "maxZ"), (GLfloat, "maxW")]),

    # GL_EXT_tessellation_shader
    GlFunction(Void, "glPatchParameteriEXT", [(GLenum, "pname"), (GLint, "value")]),

    # GL_EXT_texture_border_clamp
    GlFunction(Void, "glTexParameterIivEXT", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexParameterIuivEXT", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),

    GlFunction(Void, "glGetTexParameterIivEXT", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexParameterIuivEXT", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),

    GlFunction(Void, "glSamplerParameterIivEXT", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glSamplerParameterIuivEXT", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),

    GlFunction(Void, "glGetSamplerParameterIivEXT", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetSamplerParameterIuivEXT", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),

    # GL_OES_texture_border_clamp
    GlFunction(Void, "glTexParameterIivOES", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexParameterIuivOES", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),

    GlFunction(Void, "glGetTexParameterIivOES", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexParameterIuivOES", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),

    GlFunction(Void, "glSamplerParameterIivOES", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glSamplerParameterIuivOES", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),

    GlFunction(Void, "glGetSamplerParameterIivOES", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetSamplerParameterIuivOES", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),

    # GL_EXT_texture_buffer
    GlFunction(Void, "glTexBufferEXT", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer")]),
    GlFunction(Void, "glTexBufferRangeEXT", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer"), (GLintptr, "offset"), (GLsizeiptr, "size")]),

    # GL_OES_texture_buffer
    GlFunction(Void, "glTexBufferOES", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer")]),
    GlFunction(Void, "glTexBufferRangeOES", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer"), (GLintptr, "offset"), (GLsizeiptr, "size")]),

    # GL_EXT_copy_image
    GlFunction(Void, "glCopyImageSubDataEXT", [(GLuint, "srcName"), (GLenum, "srcTarget"), (GLint, "srcLevel"), (GLint, "srcX"), (GLint, "srcY"), (GLint, "srcZ"), (GLuint, "dstName"), (GLenum, "dstTarget"), (GLint, "dstLevel"), (GLint, "dstX"), (GLint, "dstY"), (GLint, "dstZ"), (GLsizei, "srcWidth"), (GLsizei, "srcHeight"), (GLsizei, "srcDepth")]),

    # GL_OES_copy_image
    GlFunction(Void, "glCopyImageSubDataOES", [(GLuint, "srcName"), (GLenum, "srcTarget"), (GLint, "srcLevel"), (GLint, "srcX"), (GLint, "srcY"), (GLint, "srcZ"), (GLuint, "dstName"), (GLenum, "dstTarget"), (GLint, "dstLevel"), (GLint, "dstX"), (GLint, "dstY"), (GLint, "dstZ"), (GLsizei, "srcWidth"), (GLsizei, "srcHeight"), (GLsizei, "srcDepth")]),

    # QCOM_alpha_test
    GlFunction(Void, "glAlphaFuncQCOM", [(GLenum, "func"), (GLclampf, "ref")]),

    # GL_EXT_disjoint_timer_query
    # (functions not already in GL_EXT_occlusion_query_boolean)
    GlFunction(Void, "glQueryCounterEXT", [(GLuint, "id"), (GLenum, "target")]),
    GlFunction(Void, "glGetQueryObjectivEXT", [(GLquery, "id"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetQueryObjecti64vEXT", [(GLquery, "id"), (GLenum, "pname"), Out(Array(GLint64, "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetQueryObjectui64vEXT", [(GLquery, "id"), (GLenum, "pname"), Out(Array(GLuint64, "_gl_param_size(pname)"), "params")]),

    # GL_AMD_performance_monitor
    GlFunction(Void, "glGetPerfMonitorGroupsAMD", [Out(Pointer(GLint), "numGroups"), (GLsizei, "groupsSize"), Out(Array(GLuint, "groupsSize"), "groups")], sideeffects=False),
    GlFunction(Void, "glGetPerfMonitorCountersAMD", [(GLuint, "group"), Out(Pointer(GLint), "numCounters"), Out(Pointer(GLint), "maxActiveCounters"), (GLsizei, "counterSize"), Out(Array(GLuint, "counterSize"), "counters")], sideeffects=False),
    GlFunction(Void, "glGetPerfMonitorGroupStringAMD", [(GLuint, "group"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "groupString")], sideeffects=False),
    GlFunction(Void, "glGetPerfMonitorCounterStringAMD", [(GLuint, "group"), (GLuint, "counter"), (GLsizei, "bufSize"), Out(Pointer(GLsizei), "length"), OutGlString(GLchar, "length", "counterString")], sideeffects=False),
    GlFunction(Void, "glGetPerfMonitorCounterInfoAMD", [(GLuint, "group"), (GLuint, "counter"), (GLenum, "pname"), Out(OpaquePointer(GLvoid, "sizeof(GLenum)"), "data")], sideeffects=False),
    GlFunction(Void, "glGenPerfMonitorsAMD", [(GLsizei, "n"), Out(Array(GLuint, "n"), "monitors")]),
    GlFunction(Void, "glDeletePerfMonitorsAMD", [(GLsizei, "n"), (Array(GLuint, "n"), "monitors")]),
    GlFunction(Void, "glSelectPerfMonitorCountersAMD", [(GLuint, "monitor"), (GLboolean, "enable"), (GLuint, "group"), (GLint, "numCounters"), (Array(GLuint, "numCounters"), "counterList")]),
    GlFunction(Void, "glBeginPerfMonitorAMD", [(GLuint, "monitor")]),
    GlFunction(Void, "glEndPerfMonitorAMD", [(GLuint, "monitor")]),
    GlFunction(Void, "glGetPerfMonitorCounterDataAMD", [(GLuint, "monitor"), (GLenum, "pname"), (GLsizei, "dataSize"), Out(Array(GLuint, "dataSize"), "data"), Out(Pointer(GLint), "bytesWritten")], sideeffects=False),

    # GL_INTEL_performance_query
    GlFunction(Void, "glBeginPerfQueryINTEL", [(GLuint, "queryHandle")], sideeffects=False),
    GlFunction(Void, "glCreatePerfQueryINTEL", [(GLuint, "queryId"), Out(Pointer(GLuint), "queryHandle")], sideeffects=False),
    GlFunction(Void, "glDeletePerfQueryINTEL", [(GLuint, "queryHandle")], sideeffects=False),
    GlFunction(Void, "glEndPerfQueryINTEL", [(GLuint, "queryHandle")], sideeffects=False),
    GlFunction(Void, "glGetFirstPerfQueryIdINTEL", [Out(Pointer(GLuint), "queryId")], sideeffects=False),
    GlFunction(Void, "glGetNextPerfQueryIdINTEL", [(GLuint, "queryId"), Out(Pointer(GLuint), "nextQueryId")], sideeffects=False),
    GlFunction(Void, "glGetPerfCounterInfoINTEL", [(GLuint, "queryId"), (GLuint, "counterId"), (GLuint, "counterNameLength"), Out(GLstring, "counterName"), (GLuint, "counterDescLength"), Out(GLstring, "counterDesc"), Out(Pointer(GLuint), "counterOffset"), Out(Pointer(GLuint), "counterDataSize"), Out(Pointer(GLuint), "counterTypeEnum"), Out(Pointer(GLuint), "counterDataTypeEnum"), Out(Pointer(GLuint64), "rawCounterMaxValue")], sideeffects=False),
    GlFunction(Void, "glGetPerfQueryDataINTEL", [(GLuint, "queryHandle"), (GLuint, "flags"), (GLsizei, "dataSize"), Out(OpaqueBlob(GLvoid, "datasize"), "data"), Out(Pointer(GLuint), "bytesWritten")], sideeffects=False),
    GlFunction(Void, "glGetPerfQueryIdByNameINTEL", [(GLstring, "queryName"), Out(Pointer(GLuint), "queryId")], sideeffects=False),
    GlFunction(Void, "glGetPerfQueryInfoINTEL", [(GLuint, "queryId"), (GLuint, "queryNameLength"), Out(GLstring, "queryName"), Out(Pointer(GLuint), "dataSize"), Out(Pointer(GLuint), "noCounters"), Out(Pointer(GLuint), "noInstances"), Out(Pointer(GLuint), "capsMask")], sideeffects=False),

    # GL_KHR_robustness
    GlFunction(GLenum, "glGetGraphicsResetStatusKHR", [], sideeffects=False),
    GlFunction(Void, "glReadnPixelsKHR", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLenum, "type"), (GLsizei, "bufSize"), Out(OpaquePointer(GLvoid, "bufSize"), "pixels")]),
    GlFunction(Void, "glGetnUniformfvKHR", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLfloat, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetnUniformivKHR", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLint, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetnUniformuivKHR", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLuint, "bufSize"), "params")], sideeffects=False),

    # GLES 3.0
    GlFunction(Void, "glActiveShaderProgram", [(GLpipeline, "pipeline"), (GLprogram, "program")]),
    GlFunction(Void, "glBeginQuery", [(GLenum, "target"), (GLquery, "id")]),
    GlFunction(Void, "glBeginTransformFeedback", [(GLenum, "primitiveMode")]),
    GlFunction(Void, "glBindBufferBase", [(GLenum, "target"), (GLuint, "index"), (GLbuffer, "buffer")]),
    GlFunction(Void, "glBindBufferRange", [(GLenum, "target"), (GLuint, "index"), (GLbuffer, "buffer"), (GLintptr, "offset"), (GLsizeiptr, "size")]),
    GlFunction(Void, "glBindImageTexture", [(GLuint, "unit"), (GLtexture, "texture"), (GLint, "level"), (GLboolean, "layered"), (GLint, "layer"), (GLenum, "access"), (GLenum, "format")]),
    GlFunction(Void, "glBindProgramPipeline", [(GLpipeline, "pipeline")]),
    GlFunction(Void, "glBindSampler", [(GLuint, "unit"), (GLsampler, "sampler")]),
    GlFunction(Void, "glBindTransformFeedback", [(GLenum, "target"), (GLfeedback, "id")]),
    GlFunction(Void, "glBindVertexArray", [(GLarray, "array")]),
    GlFunction(Void, "glBindVertexBuffer", [(GLuint, "bindingindex"), (GLbuffer, "buffer"), (GLintptr, "offset"), (GLsizei, "stride")]),
    GlFunction(Void, "glBlitFramebuffer", [(GLint, "srcX0"), (GLint, "srcY0"), (GLint, "srcX1"), (GLint, "srcY1"), (GLint, "dstX0"), (GLint, "dstY0"), (GLint, "dstX1"), (GLint, "dstY1"), (GLbitfield, "mask"), (GLenum, "filter")]),
    GlFunction(Void, "glClearBufferfi", [(GLenum, "buffer"), (GLint, "drawbuffer"), (GLfloat, "depth"), (GLint, "stencil")]),
    GlFunction(Void, "glClearBufferfv", [(GLenum, "buffer"), (GLint, "drawbuffer"), (Array(Const(GLfloat), "_glClearBuffer_size(buffer)"), "value")]),
    GlFunction(Void, "glClearBufferiv", [(GLenum, "buffer"), (GLint, "drawbuffer"), (Array(Const(GLint), "_glClearBuffer_size(buffer)"), "value")]),
    GlFunction(Void, "glClearBufferuiv", [(GLenum, "buffer"), (GLint, "drawbuffer"), (Array(Const(GLuint), "_glClearBuffer_size(buffer)"), "value")]),
    GlFunction(GLenum, "glClientWaitSync", [(GLsync, "sync"), (GLbitfield, "flags"), (GLuint64, "timeout")]),
    GlFunction(Void, "glCompressedTexImage3D", [(GLenum, "target"), (GLint, "level"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLint, "border"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glCompressedTexSubImage3D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLenum, "format"), (GLsizei, "imageSize"), (OpaquePointer(Const(GLvoid), "imageSize"), "data")]),
    GlFunction(Void, "glCopyBufferSubData", [(GLenum, "readTarget"), (GLenum, "writeTarget"), (GLintptr, "readOffset"), (GLintptr, "writeOffset"), (GLsizeiptr, "size")]),
    GlFunction(Void, "glCopyTexSubImage3D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(GLprogram, "glCreateShaderProgramv", [(GLenum, "type"), (GLsizei, "count"), (Const(Array(Const(GLstring), "count")), "strings")]),
    GlFunction(Void, "glDeleteProgramPipelines", [(GLsizei, "n"), (Array(Const(GLuint), "n"), "pipelines")]),
    GlFunction(Void, "glDeleteQueries", [(GLsizei, "n"), (Array(Const(GLuint), "n"), "ids")]),
    GlFunction(Void, "glDeleteSamplers", [(GLsizei, "count"), (Array(Const(GLuint), "count"), "samplers")]),
    GlFunction(Void, "glDeleteSync", [(GLsync, "sync")]),
    GlFunction(Void, "glDeleteTransformFeedbacks", [(GLsizei, "n"), (Array(Const(GLuint), "n"), "ids")]),
    GlFunction(Void, "glDeleteVertexArrays", [(GLsizei, "n"), (Array(Const(GLuint), "n"), "arrays")]),
    GlFunction(Void, "glDispatchCompute", [(GLuint, "num_groups_x"), (GLuint, "num_groups_y"), (GLuint, "num_groups_z")]),
    GlFunction(Void, "glDispatchComputeIndirect", [(GLintptr, "indirect")]),
    GlFunction(Void, "glDrawArraysIndirect", [(GLenum, "mode"), (GLpointerConst, "indirect")]),
    GlFunction(Void, "glDrawArraysInstanced", [(GLenum, "mode"), (GLint, "first"), (GLsizei, "count"), (GLsizei, "instancecount")]),
    GlFunction(Void, "glDrawBuffers", [(GLsizei, "n"), (Array(Const(GLenum), "n"), "bufs")]),
    GlFunction(Void, "glDrawElementsIndirect", [(GLenum, "mode"), (GLenum, "type"), (GLpointerConst, "indirect")]),
    GlFunction(Void, "glDrawElementsInstanced", [(GLenum, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount")]),
    GlFunction(Void, "glDrawRangeElements", [(GLenum, "mode"), (GLuint, "start"), (GLuint, "end"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices")]),
    GlFunction(Void, "glEndQuery", [(GLenum, "target")]),
    GlFunction(Void, "glEndTransformFeedback", []),
    GlFunction(GLsync, "glFenceSync", [(GLenum, "condition"), (GLbitfield, "flags")]),
    GlFunction(Void, "glFlushMappedBufferRange", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "length")]),
    GlFunction(Void, "glFramebufferParameteri", [(GLenum, "target"), (GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glFramebufferTextureLayer", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level"), (GLint, "layer")]),
    GlFunction(Void, "glGenProgramPipelines", [(GLsizei, "n"), Out(Array(GLpipeline, "n"), "pipelines")]),
    GlFunction(Void, "glGenQueries", [(GLsizei, "n"), Out(Array(GLquery, "n"), "ids")]),
    GlFunction(Void, "glGenSamplers", [(GLsizei, "count"), Out(Array(GLsampler, "count"), "samplers")]),
    GlFunction(Void, "glGenTransformFeedbacks", [(GLsizei, "n"), Out(Array(GLfeedback, "n"), "ids")]),
    GlFunction(Void, "glGenVertexArrays", [(GLsizei, "n"), Out(Array(GLarray, "n"), "arrays")]),
    GlFunction(Void, "glGetActiveUniformBlockName", [(GLprogram, "program"), (GLuint, "uniformBlockIndex"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (GLstring, "uniformBlockName")], sideeffects=False),
    GlFunction(Void, "glGetActiveUniformBlockiv", [(GLprogram, "program"), (GLuint, "uniformBlockIndex"), (GLenum, "pname"), (Array(GLint, "paramSizeGlGetActiveUniformBlockiv(program, uniformBlockIndex, pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetActiveUniformsiv", [(GLprogram, "program"), (GLsizei, "uniformCount"), (Array(Const(GLuint), "uniformCount"), "uniformIndices"), (GLenum, "pname"), (Array(GLint, "uniformCount"), "params")], sideeffects=False),
    GlFunction(Void, "glGetBooleani_v", [(GLenum, "target"), (GLuint, "index"), (Array(GLboolean, "_gl_param_size(target)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetBufferParameteri64v", [(GLenum, "target"), (GLenum, "pname"), (Array(GLint64, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetBufferPointerv", [(GLenum, "target"), (GLenum, "pname"), Out(Pointer(GLpointer), "params")], sideeffects=False),
    GlFunction(GLint, "glGetFragDataLocation", [(GLprogram, "program"), (Const(GLstring), "name")], sideeffects=False),
    GlFunction(Void, "glGetFramebufferParameteriv", [(GLenum, "target"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetInteger64i_v", [(GLenum, "target"), (GLuint, "index"), (Array(GLint64, "_gl_param_size(target)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetInteger64v", [(GLenum, "pname"), (Array(GLint64, "_gl_param_size(pname)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetIntegeri_v", [(GLenum, "target"), (GLuint, "index"), (Array(GLint, "_gl_param_size(target)"), "data")], sideeffects=False),
    GlFunction(Void, "glGetInternalformativ", [(GLenum, "target"), (GLenum, "internalformat"), (GLenum, "pname"), (GLsizei, "bufSize"), (Array(GLint, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetMultisamplefv", [(GLenum, "pname"), (GLuint, "index"), (Array(GLfloat, "_gl_param_size(pname)"), "val")], sideeffects=False),
    GlFunction(Void, "glGetProgramBinary", [(GLprogram, "program"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (Pointer(GLenum), "binaryFormat"), (Blob(GLvoid, "length"), "binary")], sideeffects=False),
    GlFunction(Void, "glGetProgramInterfaceiv", [(GLprogram, "program"), (GLenum, "programInterface"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetProgramPipelineInfoLog", [(GLpipeline, "pipeline"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (GLstring, "infoLog")], sideeffects=False),
    GlFunction(Void, "glGetProgramPipelineiv", [(GLpipeline, "pipeline"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(GLuint, "glGetProgramResourceIndex", [(GLprogram, "program"), (GLenum, "programInterface"), (Const(GLstring), "name")], sideeffects=False),
    GlFunction(GLint, "glGetProgramResourceLocation", [(GLprogram, "program"), (GLenum, "programInterface"), (Const(GLstring), "name")], sideeffects=False),
    GlFunction(Void, "glGetProgramResourceName", [(GLprogram, "program"), (GLenum, "programInterface"), (GLuint, "index"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (GLstring, "name")], sideeffects=False),
    GlFunction(Void, "glGetProgramResourceiv", [(GLprogram, "program"), (GLenum, "programInterface"), (GLuint, "index"), (GLsizei, "propCount"), (Array(Const(GLenum), "propCount"), "props"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (Array(GLint, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetQueryObjectuiv", [(GLquery, "id"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetQueryiv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetSamplerParameterfv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetSamplerParameteriv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(GLustring, "glGetStringi", [(GLenum, "name"), (GLuint, "index")], sideeffects=False),
    GlFunction(Void, "glGetSynciv", [(GLsync, "sync"), (GLenum, "pname"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (Array(GLint, "length"), "values")], sideeffects=False),
    GlFunction(Void, "glGetTexLevelParameterfv", [(GLenum, "target"), (GLint, "level"), (GLenum, "pname"), (Array(GLfloat, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexLevelParameteriv", [(GLenum, "target"), (GLint, "level"), (GLenum, "pname"), (Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTransformFeedbackVarying", [(GLprogram, "program"), (GLuint, "index"), (GLsizei, "bufSize"), (Pointer(GLsizei), "length"), (Pointer(GLsizei), "size"), (Pointer(GLenum), "type"), (GLstring, "name")], sideeffects=False),
    GlFunction(GLuniformBlock, "glGetUniformBlockIndex", [(GLprogram, "program"), (Const(GLstring), "uniformBlockName")]),
    GlFunction(Void, "glGetUniformIndices", [(GLprogram, "program"), (GLsizei, "uniformCount"), (Const(Array(Const(GLstring), "uniformCount")), "uniformNames"), (Array(GLuint, "uniformCount"), "uniformIndices")], sideeffects=False),
    GlFunction(Void, "glGetUniformuiv", [(GLprogram, "program"), (GLuniformLocation, "location"), Out(OpaquePointer(GLuint), "params")], sideeffects=False),
    GlFunction(Void, "glGetVertexAttribIiv", [(GLuint, "index"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetVertexAttribIuiv", [(GLuint, "index"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glInvalidateFramebuffer", [(GLenum, "target"), (GLsizei, "numAttachments"), (Array(Const(GLenum), "numAttachments"), "attachments")]),
    GlFunction(Void, "glInvalidateSubFramebuffer", [(GLenum, "target"), (GLsizei, "numAttachments"), (Array(Const(GLenum), "numAttachments"), "attachments"), (GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(GLboolean, "glIsProgramPipeline", [(GLpipeline, "pipeline")], sideeffects=False),
    GlFunction(GLboolean, "glIsQuery", [(GLquery, "id")], sideeffects=False),
    GlFunction(GLboolean, "glIsSampler", [(GLsampler, "sampler")], sideeffects=False),
    GlFunction(GLboolean, "glIsSync", [(GLsync, "sync")], sideeffects=False),
    GlFunction(GLboolean, "glIsTransformFeedback", [(GLfeedback, "id")], sideeffects=False),
    GlFunction(GLboolean, "glIsVertexArray", [(GLarray, "array")], sideeffects=False),
    GlFunction(GLpointer, "glMapBufferRange", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "length"), (GLbitfield, "access")]),
    GlFunction(Void, "glMemoryBarrier", [(GLbitfield, "barriers")]),
    GlFunction(Void, "glMemoryBarrierByRegion", [(GLbitfield, "barriers")]),
    GlFunction(Void, "glPauseTransformFeedback", []),
    GlFunction(Void, "glProgramBinary", [(GLprogram, "program"), (GLenum, "binaryFormat"), (Blob(Const(GLvoid), "length"), "binary"), (GLsizei, "length")]),
    GlFunction(Void, "glProgramParameteri", [(GLprogram, "program"), (GLenum, "pname"), (GLint, "value")]),
    GlFunction(Void, "glProgramUniform1f", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0")]),
    GlFunction(Void, "glProgramUniform1fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count"), "value")]),
    GlFunction(Void, "glProgramUniform1i", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0")]),
    GlFunction(Void, "glProgramUniform1iv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count"), "value")]),
    GlFunction(Void, "glProgramUniform1ui", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0")]),
    GlFunction(Void, "glProgramUniform1uiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count"), "value")]),
    GlFunction(Void, "glProgramUniform2f", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1")]),
    GlFunction(Void, "glProgramUniform2fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform2i", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1")]),
    GlFunction(Void, "glProgramUniform2iv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform2ui", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1")]),
    GlFunction(Void, "glProgramUniform2uiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*2"), "value")]),
    GlFunction(Void, "glProgramUniform3f", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2")]),
    GlFunction(Void, "glProgramUniform3fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform3i", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2")]),
    GlFunction(Void, "glProgramUniform3iv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform3ui", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2")]),
    GlFunction(Void, "glProgramUniform3uiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*3"), "value")]),
    GlFunction(Void, "glProgramUniform4f", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLfloat, "v0"), (GLfloat, "v1"), (GLfloat, "v2"), (GLfloat, "v3")]),
    GlFunction(Void, "glProgramUniform4fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLfloat), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniform4i", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLint, "v0"), (GLint, "v1"), (GLint, "v2"), (GLint, "v3")]),
    GlFunction(Void, "glProgramUniform4iv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLint), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniform4ui", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2"), (GLuint, "v3")]),
    GlFunction(Void, "glProgramUniform4uiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2x3fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*3"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix2x4fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*2*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*3"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3x2fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix3x4fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*3*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*4"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4x2fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*2"), "value")]),
    GlFunction(Void, "glProgramUniformMatrix4x3fv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*4*3"), "value")]),
    GlFunction(Void, "glReadBuffer", [(GLenum, "src")]),
    GlFunction(Void, "glRenderbufferStorageMultisample", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glResumeTransformFeedback", []),
    GlFunction(Void, "glSampleMaski", [(GLuint, "maskNumber"), (GLbitfield, "mask")]),
    GlFunction(Void, "glSamplerParameterf", [(GLsampler, "sampler"), (GLenum, "pname"), (GLfloat, "param")]),
    GlFunction(Void, "glSamplerParameterfv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLfloat), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glSamplerParameteri", [(GLsampler, "sampler"), (GLenum, "pname"), (GLint, "param")]),
    GlFunction(Void, "glSamplerParameteriv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexImage3D", [(GLenum, "target"), (GLint, "level"), (GLint, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLint, "border"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexImage3D_size(format, type, width, height, depth)"), "pixels")]),
    GlFunction(Void, "glTexStorage2D", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glTexStorage2DMultisample", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLboolean, "fixedsamplelocations")]),
    GlFunction(Void, "glTexStorage3D", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth")]),
    GlFunction(Void, "glTexSubImage3D", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLenum, "format"), (GLenum, "type"), (OpaquePointer(Const(GLvoid), "_glTexImage3D_size(format, type, width, height, depth)"), "pixels")]),
    GlFunction(Void, "glTransformFeedbackVaryings", [(GLprogram, "program"), (GLsizei, "count"), (Const(Array(Const(GLstring), "count")), "varyings"), (GLenum, "bufferMode")]),
    GlFunction(Void, "glUniform1ui", [(GLuniformLocation, "location"), (GLuint, "v0")]),
    GlFunction(Void, "glUniform1uiv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*1"), "value")]),
    GlFunction(Void, "glUniform2ui", [(GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1")]),
    GlFunction(Void, "glUniform2uiv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*2"), "value")]),
    GlFunction(Void, "glUniform3ui", [(GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2")]),
    GlFunction(Void, "glUniform3uiv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*3"), "value")]),
    GlFunction(Void, "glUniform4ui", [(GLuniformLocation, "location"), (GLuint, "v0"), (GLuint, "v1"), (GLuint, "v2"), (GLuint, "v3")]),
    GlFunction(Void, "glUniform4uiv", [(GLuniformLocation, "location"), (GLsizei, "count"), (Array(Const(GLuint), "count*4"), "value")]),
    GlFunction(Void, "glUniformBlockBinding", [(GLprogram, "program"), (GLuniformBlock, "uniformBlockIndex"), (GLuint, "uniformBlockBinding")]),
    GlFunction(Void, "glUniformMatrix2x3fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*6"), "value")]),
    GlFunction(Void, "glUniformMatrix2x4fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*8"), "value")]),
    GlFunction(Void, "glUniformMatrix3x2fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*6"), "value")]),
    GlFunction(Void, "glUniformMatrix3x4fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*12"), "value")]),
    GlFunction(Void, "glUniformMatrix4x2fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*8"), "value")]),
    GlFunction(Void, "glUniformMatrix4x3fv", [(GLuniformLocation, "location"), (GLsizei, "count"), (GLboolean, "transpose"), (Array(Const(GLfloat), "count*12"), "value")]),
    GlFunction(GLboolean, "glUnmapBuffer", [(GLenum, "target")]),
    GlFunction(Void, "glUseProgramStages", [(GLpipeline, "pipeline"), (GLbitfield, "stages"), (GLprogram, "program")]),
    GlFunction(Void, "glValidateProgramPipeline", [(GLpipeline, "pipeline")]),
    GlFunction(Void, "glVertexAttribBinding", [(GLuint, "attribindex"), (GLuint, "bindingindex")]),
    GlFunction(Void, "glVertexAttribDivisor", [(GLuint, "index"), (GLuint, "divisor")]),
    GlFunction(Void, "glVertexAttribFormat", [(GLuint, "attribindex"), (GLint, "size"), (GLenum, "type"), (GLboolean, "normalized"), (GLuint, "relativeoffset")]),
    GlFunction(Void, "glVertexAttribI4i", [(GLuint, "index"), (GLint, "x"), (GLint, "y"), (GLint, "z"), (GLint, "w")]),
    GlFunction(Void, "glVertexAttribI4iv", [(GLuint, "index"), (Array(Const(GLint), 4), "v")]),
    GlFunction(Void, "glVertexAttribI4ui", [(GLuint, "index"), (GLuint, "x"), (GLuint, "y"), (GLuint, "z"), (GLuint, "w")]),
    GlFunction(Void, "glVertexAttribI4uiv", [(GLuint, "index"), (Array(Const(GLuint), 4), "v")]),
    GlFunction(Void, "glVertexAttribIFormat", [(GLuint, "attribindex"), (GLint, "size"), (GLenum, "type"), (GLuint, "relativeoffset")]),
    GlFunction(Void, "glVertexAttribIPointer", [(GLuint, "index"), (GLint, "size"), (GLenum, "type"), (GLsizei, "stride"), (GLpointerConst, "pointer")]),
    GlFunction(Void, "glVertexBindingDivisor", [(GLuint, "bindingindex"), (GLuint, "divisor")]),
    GlFunction(Void, "glWaitSync", [(GLsync, "sync"), (GLbitfield, "flags"), (GLuint64, "timeout")]),

    # GLES 3.2
    GlFunction(GLenum, "glGetGraphicsResetStatus", [], sideeffects=False),
    GlFunction(Void, "glReadnPixels", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLenum, "type"), (GLsizei, "bufSize"), Out(OpaquePointer(GLvoid, "bufSize"), "pixels")]),
    GlFunction(Void, "glGetnUniformfv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLfloat, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetnUniformiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLint, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetnUniformuiv", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLuint, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glTexStorage3DMultisample", [(GLenum, "target"), (GLsizei, "samples"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLboolean, "fixedsamplelocations")]),
    GlFunction(Void, "glEnablei", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(Void, "glDisablei", [(GLenum, "target"), (GLuint, "index")]),
    GlFunction(GLboolean, "glIsEnabledi", [(GLenum, "target"), (GLuint, "index")], sideeffects=False),
    GlFunction(Void, "glBlendEquationi", [(GLuint, "buf"), (GLenum, "mode")]),
    GlFunction(Void, "glBlendEquationSeparatei", [(GLuint, "buf"), (GLenum, "modeRGB"), (GLenum, "modeAlpha")]),
    GlFunction(Void, "glBlendFunci", [(GLuint, "buf"), (GLenum, "src"), (GLenum, "dst")]),
    GlFunction(Void, "glBlendFuncSeparatei", [(GLuint, "buf"), (GLenum, "srcRGB"), (GLenum, "dstRGB"), (GLenum, "srcAlpha"), (GLenum, "dstAlpha")]),
    GlFunction(Void, "glColorMaski", [(GLuint, "index"), (GLboolean, "r"), (GLboolean, "g"), (GLboolean, "b"), (GLboolean, "a")]),
    GlFunction(Void, "glFramebufferTexture", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level")]),
    GlFunction(Void, "glPrimitiveBoundingBox", [(GLfloat, "minX"), (GLfloat, "minY"), (GLfloat, "minZ"), (GLfloat, "minW"), (GLfloat, "maxX"), (GLfloat, "maxY"), (GLfloat, "maxZ"), (GLfloat, "maxW")]),
    GlFunction(Void, "glPatchParameteri", [(GLenum, "pname"), (GLint, "value")]),
    GlFunction(Void, "glTexParameterIiv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glTexParameterIuiv", [(GLenum, "target"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetTexParameterIiv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetTexParameterIuiv", [(GLenum, "target"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glSamplerParameterIiv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glSamplerParameterIuiv", [(GLsampler, "sampler"), (GLenum, "pname"), (Array(Const(GLuint), "_gl_param_size(pname)"), "params")]),
    GlFunction(Void, "glGetSamplerParameterIiv", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glGetSamplerParameterIuiv", [(GLsampler, "sampler"), (GLenum, "pname"), Out(Array(GLuint, "_gl_param_size(pname)"), "params")], sideeffects=False),
    GlFunction(Void, "glTexBuffer", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer")]),
    GlFunction(Void, "glTexBufferRange", [(GLenum, "target"), (GLenum, "internalformat"), (GLbuffer, "buffer"), (GLintptr, "offset"), (GLsizeiptr, "size")]),
    GlFunction(Void, "glCopyImageSubData", [(GLuint, "srcName"), (GLenum, "srcTarget"), (GLint, "srcLevel"), (GLint, "srcX"), (GLint, "srcY"), (GLint, "srcZ"), (GLuint, "dstName"), (GLenum, "dstTarget"), (GLint, "dstLevel"), (GLint, "dstX"), (GLint, "dstY"), (GLint, "dstZ"), (GLsizei, "srcWidth"), (GLsizei, "srcHeight"), (GLsizei, "srcDepth")]),
    GlFunction(Void, "glDrawElementsBaseVertex", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawRangeElementsBaseVertex", [(GLenum_mode, "mode"), (GLuint, "start"), (GLuint, "end"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawElementsInstancedBaseVertex", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount"), (GLint, "basevertex")]),
    GlFunction(Void, "glBlendBarrier", []),
    GlFunction(Void, "glMinSampleShading", [(GLfloat, "value")]),

    # GL_EXT_texture_storage
    GlFunction(Void, "glTexStorage1DEXT", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width")]),
    GlFunction(Void, "glTexStorage2DEXT", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height")]),
    GlFunction(Void, "glTexStorage3DEXT", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth")]),

    # GL_OES_tessellation_shader
    GlFunction(Void, "glPatchParameteriOES", [(GLenum, "pname"), (GLint, "value")]),

    # GL_OVR_multiview
    GlFunction(Void, "glFramebufferTextureMultiviewOVR", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level"), (GLint, "baseViewIndex"), (GLsizei, "numViews")]),

    # GL_OVR_multiview_multisampled_render_to_texture
    GlFunction(Void, "glFramebufferTextureMultisampleMultiviewOVR", [(GLenum, "target"), (GLenum, "attachment"), (GLtexture, "texture"), (GLint, "level"), (GLsizei, "samples"), (GLint, "baseViewIndex"), (GLsizei, "numViews")]),

    # GL_EXT_clear_texture
    GlFunction(Void, "glClearTexImageEXT", [ (GLtexture, "texture"), (GLint, "level"), (GLenum, "format"), (GLenum, "type"), (Blob(Const(GLvoid), "_glClearBufferData_size(format, type)"), "data")]),
    GlFunction(Void, "glClearTexSubImageEXT", [ (GLtexture, "texture"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLenum, "format"), (GLenum, "type"), (Blob(Const(GLvoid), "_glClearBufferData_size(format, type)"), "data")]),

    # GL_EXT_buffer_storage
    GlFunction(Void, "glBufferStorageEXT", [ (GLenum, "target"), (GLsizeiptr, "size"), (Blob(Const(GLvoid), "size"), "data"), (GLbitfield_storage, "flags")]),

    # GL_EXT_draw_transform_feedback
    GlFunction(Void, "glDrawTransformFeedbackEXT", [(GLenum, "mode"), (GLuint, "id")]),
    GlFunction(Void, "glDrawTransformFeedbackInstancedEXT", [(GLenum, "mode"), (GLuint, "id"), (GLsizei, "instancecount")]),

    # GL_OES_draw_elements_base_vertex
    GlFunction(Void, "glDrawElementsBaseVertexOES", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawRangeElementsBaseVertexOES", [(GLenum_mode, "mode"), (GLuint, "start"), (GLuint, "end"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawElementsInstancedBaseVertexOES", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount"), (GLint, "basevertex")]),

    # GL_EXT_clip_control
    GlFunction(Void, "glClipControlEXT", [(GLenum, "origin"), (GLenum, "depth")]),

    # GL_EXT_multiview_draw_buffers
    GlFunction(Void, "glReadBufferIndexedEXT", [(GLenum, "src"), (GLint, "index")]),
    GlFunction(Void, "glDrawBuffersIndexedEXT", [(GLint, "n"), (Array(Const(GLenum), "n"), "location"), (Array(Const(GLint), "n"), "indices")]),
    GlFunction(Void, "glGetIntegeri_vEXT", [(GLenum, "target"), (GLuint, "index"), Out(Array(GLint, "_gl_param_size(target)"), "data")], sideeffects=False),

    # GL_EXT_draw_elements_base_vertex
    GlFunction(Void, "glDrawElementsBaseVertexEXT", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawRangeElementsBaseVertexEXT", [(GLenum_mode, "mode"), (GLuint, "start"), (GLuint, "end"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLint, "basevertex")]),
    GlFunction(Void, "glDrawElementsInstancedBaseVertexEXT", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount"), (GLint, "basevertex")]),

    # GL_EXT_base_instance
    GlFunction(Void, "glDrawArraysInstancedBaseInstanceEXT", [(GLenum_mode, "mode"), (GLint, "first"), (GLsizei, "count"), (GLsizei, "instancecount"), (GLuint, "baseinstance")]),
    GlFunction(Void, "glDrawElementsInstancedBaseInstanceEXT", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount"), (GLuint, "baseinstance")]),
    GlFunction(Void, "glDrawElementsInstancedBaseVertexBaseInstanceEXT", [(GLenum_mode, "mode"), (GLsizei, "count"), (GLenum, "type"), (GLpointerConst, "indices"), (GLsizei, "instancecount"), (GLint, "basevertex"), (GLuint, "baseinstance")]),

    # GL_EXT_robustness
    GlFunction(GLenum, "glGetGraphicsResetStatusEXT", [], sideeffects=False),
    GlFunction(Void, "glReadnPixelsEXT", [(GLint, "x"), (GLint, "y"), (GLsizei, "width"), (GLsizei, "height"), (GLenum, "format"), (GLenum, "type"), (GLsizei, "bufSize"), Out(OpaquePointer(GLvoid, "bufSize"), "pixels")]),
    GlFunction(Void, "glGetnUniformfvEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLfloat, "bufSize"), "params")], sideeffects=False),
    GlFunction(Void, "glGetnUniformivEXT", [(GLprogram, "program"), (GLuniformLocation, "location"), (GLsizei, "bufSize"), Out(Array(GLint, "bufSize"), "params")], sideeffects=False),

    # GL_EXT_blend_minmax
    GlFunction(Void, "glBlendEquationEXT", [(GLenum, "mode")]),

    # GL_EXT_shader_pixel_local_storage2
    GlFunction(Void, "glFramebufferPixelLocalStorageSizeEXT", [(GLframebuffer, "target"), (GLsizei, "size")]),
    GlFunction(GLsizei, "glGetFramebufferPixelLocalStorageSizeEXT", [(GLframebuffer, "target")], sideeffects=False),
    GlFunction(Void, "glClearPixelLocalStorageuiEXT", [(GLsizei, "offset"), (GLsizei, "n"), (Array(Const(GLuint), "n"), "values")]),

    # GL_EXT_draw_buffers
    GlFunction(Void, "glDrawBuffersEXT", [(GLsizei, "n"), (Array(Const(GLenum), "n"), "bufs")]),

    # GL_EXT_instanced_arrays
    GlFunction(Void, "glVertexAttribDivisorEXT", [(GLuint, "index"), (GLuint, "divisor")]),

    # GL_EXT_map_buffer_range
    GlFunction(GLpointer, "glMapBufferRangeEXT", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "length"), (GLbitfield_access, "access")]),
    GlFunction(Void, "glFlushMappedBufferRangeEXT", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "length")]),

    # GL_EXT_polygon_offset_clamp
    GlFunction(Void, "glPolygonOffsetClampEXT", [(GLfloat, "factor"), (GLfloat, "units"), (GLfloat, "clamp")]),

    # GL_EXT_raster_multisample
    GlFunction(Void, "glRasterSamplesEXT", [(GLuint, "samples"), (GLboolean, "fixedsamplelocations")]),

    # GL_EXT_sparse_texture
    GlFunction(Void, "glTexPageCommitmentEXT", [(GLenum, "target"), (GLint, "level"), (GLint, "xoffset"), (GLint, "yoffset"), (GLint, "zoffset"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLboolean, "commit")]),

    # GL_EXT_texture_view
    GlFunction(Void, "glTextureViewEXT", [(GLtexture, "texture"), (GLenum, "target"), (GLtexture, "origtexture"), (GLenum, "internalformat"), (GLuint, "minlevel"), (GLuint, "numlevels"), (GLuint, "minlayer"), (GLuint, "numlayers")]),

    # GL_OES_single_precision
    GlFunction(Void, "glClearDepthfOES", [(GLclampf, "depth")]),
    GlFunction(Void, "glClipPlanefOES", [(GLenum, "plane"), (Array(Const(GLfloat), 4), "equation")]),
    GlFunction(Void, "glDepthRangefOES", [(GLclampf, "n"), (GLclampf, "f")]),
    GlFunction(Void, "glFrustumfOES", [(GLfloat, "l"), (GLfloat, "r"), (GLfloat, "b"), (GLfloat, "t"), (GLfloat, "n"), (GLfloat, "f")]),
    GlFunction(Void, "glGetClipPlanefOES", [(GLenum, "plane"), Out(Array(GLfloat, 4), "equation")], sideeffects=False),
    GlFunction(Void, "glOrthofOES", [(GLfloat, "l"), (GLfloat, "r"), (GLfloat, "b"), (GLfloat, "t"), (GLfloat, "n"), (GLfloat, "f")]),

    # Need function to replace glLinkProgram in order to store link status during tracing together with the call itself
    GlFunction(Void, "glLinkProgram2", [(GLprogram, "program"), (GLboolean, "status")]),

    # GL_EXT_multi_draw_indirect
    # (GLES extension does not support client-side buffers)
    GlFunction(Void, "glMultiDrawArraysIndirectEXT", [(GLenum_mode, "mode"), (GLpointerConst, "indirect"), (GLsizei, "drawcount"), (GLsizei, "stride")]),
    GlFunction(Void, "glMultiDrawElementsIndirectEXT", [(GLenum_mode, "mode"), (GLenum, "type"), (GLpointerConst, "indirect"), (GLsizei, "drawcount"), (GLsizei, "stride")]),

    # GL_EXT_texture_storage_compression
    GlFunction(Void, "glTexStorageAttribs2DEXT", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLAttribList, "attrib_list")]),
    GlFunction(Void, "glTexStorageAttribs3DEXT", [(GLenum, "target"), (GLsizei, "levels"), (GLenum, "internalformat"), (GLsizei, "width"), (GLsizei, "height"), (GLsizei, "depth"), (GLAttribList, "attrib_list")]),

    # GL_EXT_external_buffer
    GlFunction(Void, "glBufferStorageExternalEXT", [(GLenum, "target"), (GLintptr, "offset"), (GLsizeiptr, "size"), (OpaquePointer(Void), "clientBuffer"), (GLbitfield, "flags")]),

    # GL_QCOM_shading_rate
    GlFunction(Void, "glShadingRateQCOM", [(GLenum, "rate")]),

    # GL_ARM_fragment_shading_rate
    GlFunction(Void, "glShadingRateARM", [(GLenum, "rate")]),
    GlFunction(Void, "glShadingRateCombinerOpsARM", [(GLenum, "combinerOp0"), (GLenum, "combinerOp1")]),
    GlFunction(Void, "glFramebufferShadingRateARM", [(GLenum, "target"), (GLenum, "attachment"), (GLuint, "texture"), (GLint, "baseLayer"), (GLsizei, "numLayers"), (GLsizei, "texelWidth"), (GLsizei, "texelHeight")]),

    # GL_EXT_fragment_shading_rate
    GlFunction(Void, "glGetFragmentShadingRatesEXT", [(GLsizei, "samples"), (GLsizei, "maxCount"), Out(Pointer(GLsizei), "count"), Out(Pointer(GLenum), "shadingRates")], sideeffects=False),
    GlFunction(Void, "glShadingRateEXT", [(GLenum, "rate")]),
    GlFunction(Void, "glShadingRateCombinerOpsEXT", [(GLenum, "combinerOp0"), (GLenum, "combinerOp1")]),
    GlFunction(Void, "glFramebufferShadingRateEXT", [(GLenum, "target"), (GLenum, "attachment"), (GLuint, "texture"), (GLint, "baseLayer"), (GLsizei, "numLayers"), (GLsizei, "texelWidth"), (GLsizei, "texelHeight")]),
]

glesapi = API('GLES')
glesapi.addFunctions(gles_functions)
