from __future__ import print_function
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import specs.stdapi as stdapi
import specs.gles12api as gles12api
import specs.eglapi as eglapi
from dispatch.dispatch import function_pointer_type, function_pointer_value

# global variables
gMaxId = 0
gIdToFunc = {}
gIdToLength = {}
api = stdapi.API()

type_to_printf = {
    'EGLAttrib *': '%p',
    'EGLClientBuffer': '%p',
    'EGLConfig': '%p',
    'EGLConfig *': '%p',
    'EGLContext': '%p',
    'EGLDisplay': '%p',
    'EGLImageKHR': '%p',
    'EGLImage': '%p',
    'EGLNativeDisplayType': '%p',
    'EGLNativePixmapType': '%lx',
    'EGLNativeWindowType': '%lx',
    'EGLSurface': '%p',
    'EGLSync': '%p',
    'EGLSyncKHR': '%p',
    'EGLSyncNV': '%p',
    'EGLTimeKHR': '%lx',
    'EGLTimeNV': '%lx',
    'EGLenum': '%x',
    'EGLint': '%x',
    'EGLint *': '%p',
    'GLbitfield': '%x',
    'GLboolean': '%x',
    'GLboolean *': '%p',
    'GLchar *': '%p',
    'GLclampf': '%f',
    'GLclampx': '%x',
    'GLenum': '%x',
    'GLenum *': '%p',
    'GLfixed': '%x',
    'GLfixed *': '%p',
    'GLfloat': '%f',
    'GLfloat *': '%p',
    'GLint': '%x',
    'GLint *': '%p',
    'GLint64 *': '%p',
    'GLintptr': '%lx',
    'GLsizei': '%x',
    'GLsizei *': '%p',
    'GLsizeiptr': '%lx',
    'GLsync': '%p',
    'GLubyte': '%x',
    'GLuint': '%x',
    'GLuint *': '%p',
    'GLuint64': '%lx',
    'GLuint64 *': '%p',
    'GLvoid *': '%p',
    'GLvoid * *': '%p',
    'GLDEBUGPROCKHR': '%p',
    'const EGLAttrib *': '%p',
    'const EGLint *': '%p',
    'const GLchar *': '%p',
    'const GLchar * const *': '%p',
    'const GLenum *': '%p',
    'const GLfixed *': '%p',
    'const GLfloat *': '%p',
    'const GLint *': '%p',
    'const GLuint *': '%p',
    'const GLvoid *': '%p',
    'const char *': '%p',
    'const void *': '%p',
    'const unsigned int *': '%p',
    'int64_t *': '%p',
    'struct EGLClientPixmapHI *': '%p',
    'void *': '%p',
    'void * *': '%p',
}

# Special inject function entry points
internal_functions = [
    'glGetUniformLocation',
    'glBindAttribLocation',
    'glGetUniformBlockIndex',
    'glGenTextures',
    'glDeleteTextures',
    'glBindBuffer',
]

array_pointer_function_names = set((
    "glVertexPointer",
    "glNormalPointer",
    "glColorPointer",
    "glIndexPointer",
    "glTexCoordPointer",
    "glEdgeFlagPointer",
    "glFogCoordPointer",
    "glSecondaryColorPointer",

    "glInterleavedArrays",

    "glVertexPointerEXT",
    "glNormalPointerEXT",
    "glColorPointerEXT",
    "glIndexPointerEXT",
    "glTexCoordPointerEXT",
    "glEdgeFlagPointerEXT",
    "glFogCoordPointerEXT",
    "glSecondaryColorPointerEXT",

    "glVertexAttribPointer",
    "glVertexAttribPointerARB",
    "glVertexAttribPointerNV",
    "glVertexAttribIPointer",
    "glVertexAttribIPointerEXT",
    "glVertexAttribLPointer",
    "glVertexAttribLPointerEXT",
))

# We do not want the application to call these, we want control over them
ignore_functions = [
    'glDebugMessageCallback',
    'glDebugMessageControl'
]

# Will be prepended with 'after_'
post_functions = [
    'glLinkProgram',
    'glUnmapBuffer',
    'eglDestroyImageKHR',
    'eglDestroyImage',
]

arrays_es1 = [("Vertex", "VERTEX"),
    ("Normal",  "NORMAL"),
    ("Color", "COLOR"),
    ("TexCoord", "TEXTURE_COORD")]

class SerializeVisitor(stdapi.Visitor):
    def visitVoid(self, void, name, func):
        pass
    def visitLiteral(self, literal, name, func):
        print('    dest = WriteFixed<%s>(dest, %s); // literal' % (literal, name))
    def visitString(self, string, name, func):
        if func.name == 'glAssertBuffer_ARM': # replace dummy argument with md5sum
            print('    const char *ptr = (const char *)_glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);')
            print('    MD5Digest md5_calc(ptr, size);')
            print('    dest = WriteString(dest, md5_calc.text().c_str()); // string')
            print('    _glUnmapBuffer(target);')
        else:
            print('    dest = WriteString(dest, (const char*)%s); // string' % (name))
    def visitConst(self, const, name, func):
        self.visit(const.type, name, func)
    def visitStruct(self, struct, name, func):
        print('    #error')
    def visitArray(self, array, name, func):
        eleSerialType = stdapi.getSerializationType(array.type)
        if stdapi.isString(array.type):
            print('    dest = WriteStringArray(dest, %s, %s); // string array' % (array.length, name))
        elif func.name == "glGetSynciv":
            print('    if (%s) {' % array.length)
            print('        dest = Write1DArray<%s>(dest, *%s, (%s*)%s); // array' % (eleSerialType, array.length, eleSerialType, name))
            print('    } else {')
            print('        dest = Write1DArray<%s>(dest, 0, (%s*)%s); // array size is 0' % (eleSerialType, eleSerialType, name))
            print('    }')
        else:
            print('    dest = Write1DArray<%s>(dest, %s, (%s*)%s); // array' % (eleSerialType, array.length, eleSerialType, name))
    def visitBlob(self, blob, name, func):
        if func.name == 'glGetProgramBinary':
            print('    if (%s) {' % blob.size)
            print('        dest = Write1DArray<char>(dest, (unsigned int)*%s, (const char*)%s); // blob' % (blob.size, name))
            print('    } else {')
            print('        dest = Write1DArray<char>(dest, 0, (const char*)%s); // blob size is 0' % (name))
            print('    }')
        else:
            print('    dest = Write1DArray<char>(dest, (unsigned int)%s, (const char*)%s); // blob' % (blob.size, name))
    def visitEnum(self, enum, name, func):
        print('    dest = WriteFixed<int>(dest, %s); // enum' % (name))
    def visitBitmask(self, bitmask, name, func):
        self.visit(bitmask.type, name, func)
    def visitPointer(self, pointer, name, func):
        print('    if (%s) {' % name)
        print('        dest = WriteFixed<unsigned int>(dest, 1); // valid pointer')
        print('        dest = WriteFixed<%s>(dest, *%s); // pointer' % (stdapi.getSerializationType(pointer.type), name))
        print('    } else {')
        print('        dest = WriteFixed<unsigned int>(dest, 0); // NULL pointer')
        print('    }')
    def visitIntPointer(self, pointer, name, func):
        print('    dest = WriteFixed<int>(dest, (intptr_t)%s); // int pointer' % name)
    def visitObjPointer(self, pointer, name, func):
        print('    #error')
    def visitLinearPointer(self, pointer, name, func):
        print('    dest = WriteFixed<unsigned int>(dest, (uintptr_t)%s); // linear pointer' % name)
    def visitReference(self, reference, name, func):
        print('    #error')
    def visitHandle(self, handle, name, func):
        # Currently only the GLsync has casting needs:
        # GLsync -> unsigned long long
        if handle.type.needs_casting:
            name = '({type}){name}'.format(type=handle.type.type, name=name)

        self.visit(handle.type, name, func)
        # TODO: LZ:
    def visitAlias(self, alias, name, func):
        self.visit(alias.type, name, func)
    def visitOpaque(self, opaque, name, func):
        if func.name in stdapi.draw_function_names and name == 'indices':
            print('    if (!_element_array_buffer) {')
            print('#if ENABLE_CLIENT_SIDE_BUFFER')
            print('        dest = WriteFixed<unsigned int>(dest, ClientSideBufferObjectReferenceType); // IS *MEMORY REFERENCE*')
            print('        dest = WriteFixed<unsigned int>(dest, clientSideBufferObjName);')
            print('        dest = WriteFixed<unsigned int>(dest, 0);')
            print('#else')
            print('        dest = Write1DArray<char>(dest, (unsigned int)(count*_gl_type_size(type)), (const char*)indices);')
            print('#endif')
            print('    } else {')
            print("        dest = WriteFixed<unsigned int>(dest, BufferObjectReferenceType); // ISN'T *BLOB*")
            print('        dest = WriteFixed<unsigned int>(dest, (uintptr_t)%s); // opaque -> ptr '% (name))
            print('    }')
        elif func.name in stdapi.texture_function_names:
            print('    if (isUsingPBO)')
            print('    {')
            print('        GLint _unpack_buffer = 0;')
            print('        _glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &_unpack_buffer);')
            print('        if (!_unpack_buffer)')
            print('        {')
            print('            dest = WriteFixed<unsigned int>(dest, BlobType);')
            print('            dest = Write1DArray<char>(dest, (unsigned int)%s, (const char*)%s);' % (opaque.size, name))
            print('        }')
            print('        else')
            print('        {')
            print('            dest = WriteFixed<unsigned int>(dest, BufferObjectReferenceType); // Is not blob')
            print('            dest = WriteFixed<unsigned int>(dest, (uintptr_t)%s); // opaque -> ptr ' % (name))
            print('        }')
            print('    }')
            print('    else')
            print('    {')
            print('        dest = WriteFixed<unsigned int>(dest, BlobType);')
            print('        dest = Write1DArray<char>(dest, (unsigned int)%s, (const char*)%s);' % (opaque.size, name))
            print('    }')
        elif func.name == "glReadPixels" or func.name == 'glReadnPixels' or func.name == 'glReadnPixelsEXT' or func.name == 'glReadnPixelsKHR':
            print('    if (isUsingPBO)')
            print('    {')
            print('        GLint _pack_buffer = 0;')
            print('        _glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &_pack_buffer);')
            print('        if (!_pack_buffer)')
            print('        {')
            print('            dest = WriteFixed<unsigned int>(dest, NoopType);')
            print('        }')
            print('        else')
            print('        {')
            print('            dest = WriteFixed<unsigned int>(dest, BufferObjectReferenceType); // Is not blob')
            print('            dest = WriteFixed<unsigned int>(dest, (uintptr_t)%s); // opaque -> ptr ' % (name))
            print('        }')
            print('    }')
            print('    else')
            print('    {')
            print('       dest = WriteFixed<unsigned int>(dest, NoopType);')
            print('    }')
        else:
            print("    dest = WriteFixed<unsigned int>(dest, BufferObjectReferenceType); // IS Simple Memory Offset")
            print('    dest = WriteFixed<unsigned int>(dest, (uintptr_t)%s); // opaque -> ptr' % (name))
    def visitInterface(self, interface, name, func):
        print('    #error')
    def visitPolymorphic(self, polymorphic, name, func):
        print('    #error')

class TypeGetter(stdapi.Visitor):
    '''Determine which glGet*v function that matches the specified type.'''

    def __init__(self, prefix = 'glGet', long_suffix = True, ext_suffix = ''):
        self.prefix = prefix
        self.long_suffix = long_suffix
        self.ext_suffix = ext_suffix

    def visitConst(self, const):
        return self.visit(const.type)

    def visitAlias(self, alias):
        if alias.expr == 'GLboolean':
            if self.long_suffix:
                suffix = 'Booleanv'
                arg_type = alias.expr
            else:
                suffix = 'iv'
                arg_type = 'GLint'
        elif alias.expr == 'GLdouble':
            if self.long_suffix:
                suffix = 'Doublev'
                arg_type = alias.expr
            else:
                suffix = 'dv'
                arg_type = alias.expr
        elif alias.expr == 'GLfloat':
            if self.long_suffix:
                suffix = 'Floatv'
                arg_type = alias.expr
            else:
                suffix = 'fv'
                arg_type = alias.expr
        elif alias.expr in ('GLint', 'GLuint', 'GLsizei'):
            if self.long_suffix:
                suffix = 'Integerv'
                arg_type = 'GLint'
            else:
                suffix = 'iv'
                arg_type = 'GLint'
        else:
            print(alias.expr)
            assert False
        function_name = self.prefix + suffix + self.ext_suffix
        return function_name, arg_type

    def visitEnum(self, enum):
        return self.visit(gles12api.GLint)

    def visitBitmask(self, bitmask):
        return self.visit(gles12api.GLint)

    def visitOpaque(self, pointer):
        return self.prefix + 'Pointerv' + self.ext_suffix, 'GLvoid *'


class UserArrays:
    def __init__(self):
        self.idt = ''

    def prolog(self, uppercase_name):
        if uppercase_name == 'TEXTURE_COORD':
            print('%sGLint client_active_texture = 0;' % self.idt)
            print('%s_glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &client_active_texture);' % self.idt)
            print('%sGLint max_texture_coords = 0;' % self.idt)
            print('%s_glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_texture_coords);' % self.idt)
            print('%sfor (GLint unit = 0; unit < max_texture_coords; ++unit) {' % self.idt)
            print('%s    GLint texture = GL_TEXTURE0 + unit;' % self.idt)
            print('%s    _glClientActiveTexture(texture);' % self.idt)
            self.idt += '    '

    def cleanup(self, uppercase_name):
        if uppercase_name == 'TEXTURE_COORD':
            print('%s_glClientActiveTexture(client_active_texture);' % self.idt)

    def epilog(self, uppercase_name):
        if uppercase_name == 'TEXTURE_COORD':
            print('        }')
            self.idt = self.idt[0:-4]
        self.cleanup(uppercase_name)

class Tracer:
    def traceFunctionInject(self, func):
        print(func.prototype('inject_' + func.name) + '{')
        print('    unsigned char tid = GetThreadId();')
        if func.type is not stdapi.Void:
            print('    %s _result;' % func.type)
        self.traceFunctionBody_pre(func)
        self.traceFunctionBody(func, True)
        self.traceFunctionBody_after(func)
        if func.type is not stdapi.Void:
            print('    return _result;')
        print('}')
        print()

    def traceFunction(self, func):
        print(func.prototype('patrace_' + func.name) + '{')

        print('    unsigned char tid = GetThreadId();')
        print('    UpdateTimesEGLConfigUsed(tid);')

        if func.type is not stdapi.Void:
            print('    %s _result;' % func.type)

        self.traceFunctionBody_pre(func)
        self.traceFunctionBody(func)
        self.traceFunctionBody_after(func)

        if func.type is not stdapi.Void:
            print('    return _result;')
        print('}')
        print()

        if func.name == 'glGetAttribLocation':
            print('extern "C" PUBLIC GLint GLES_CALLCONVENTION glGetAttribLocation(GLuint program, const GLchar * name);')
        else:
            print('extern "C" PUBLIC ' + func.prototype() + ';')

        if func.name == 'glGetAttribLocation':
            print('GLint GLES_CALLCONVENTION glGetAttribLocation(GLuint program, const GLchar * name) {')
        else:
            print(func.prototype() + ' {')

        params = ', '.join([str(arg.name) for arg in func.args])

        if func.type is not stdapi.Void:
            do_ret = "return "
        else:
            do_ret = ""
        dispatch = "patrace_"+func.name
        print('    %s%s(%s);' % (do_ret, dispatch, params))
        print('}')
        print()

    def traceFunctionBody(self, func, mark_as_injected = False):
        print('    // traceFunctionBody')
        global gIdToLength

        invoke_func = func

        print('    if (gTraceThread.at(tid).mCallDepth)')
        print('    {')
        print('        //DBG_LOG("WARNING: Recursive glCall. Depth: %d\\n", gTraceThread.at(tid).mCallDepth);')
        self.invokeFunction(invoke_func, indent=' ' * 8)
        if func.type is stdapi.Void:
            print('        return;')
        else:
            print('        return _result;')
        print('    }')

        self.invokeFunction(invoke_func)

        if func.name == 'eglCreateWindowSurface':
            print('    EGLint x = 0, y = 0, width = 0, height = 0;')
            print('    if (_eglQuerySurface(dpy, _result, EGL_WIDTH, (EGLint*) &width) == EGL_FALSE)')
            print('        DBG_LOG("Failed to query surface width\\n");')
            print('    if (_eglQuerySurface(dpy, _result, EGL_HEIGHT, (EGLint*) &height) == EGL_FALSE)')
            print('        DBG_LOG("Failed to query surface height\\n");')

        if func.name in ['eglCreateImage', 'eglCreateImageKHR']:
            print('    // Create a texture based on the the newly created EGLImage.')
            print('    // The texture created, will be used as an EGLImage when retracing.')
            print('    GLuint textureId = pre_%s(_result, target, buffer, attrib_list);' % func.name)
            print()
            print('    // Use texture created in pre_eglCreateImageKHR')
            print('    ctx = _eglGetCurrentContext();')
            print('    target = EGL_GL_TEXTURE_2D_KHR;')
            print('    intptr_t textureIdAsPtr = static_cast<intptr_t>(textureId);')
            print('    buffer = reinterpret_cast<EGLClientBuffer>(textureIdAsPtr);')
            print()

        if func.name == 'glEGLImageTargetTexture2DOES':
            print('    // The data of EGLImage need to be refreshed')
            print('    // We need to create a new EGLImage to replace it when retracing')
            print('    EGLint *attrib_list;')
            print('    GLuint textureId = pre_glEGLImageTargetTexture2DOES(image, attrib_list);')
            print()
            print('    // Use texture created in pre_eglCreateImageKHR')
            print('    intptr_t textureIdAsPtr = static_cast<intptr_t>(textureId);')
            print('    EGLClientBuffer buffer = reinterpret_cast<EGLClientBuffer>(textureIdAsPtr);')
            print()

        if func.name == 'glGetInternalformativ':
            print('    if (tracerParams.Support2xMSAA && pname == GL_NUM_SAMPLE_COUNTS) // check if we need to increase this value to include MSAA 2')
            print('    {')
            print('        printf("glGetInternalformativ GL_NUM_SAMPLE_COUNTS count=%d\\n", *params);')
            print('        std::vector<GLint> vals(*params);')
            print('        _glGetInternalformativ(target, internalformat, GL_SAMPLES, *params, vals.data());')
            print('        bool found = false;')
            print('        for (int i = 0; i < *params; i++) if (vals.at(i) == 2) found = true;')
            print('        printf("glGetInternalformativ GL_NUM_SAMPLE_COUNTS inc cap\\n");')
            print('        if (!found) *params = *params + 1; // increase requested capacity by one')
            print('    }')
            print('    else if (tracerParams.Support2xMSAA && pname == GL_SAMPLES && bufSize > 1)')
            print('    {')
            print('        params[bufSize - 1] = 2; // the list is always sorted in descending order, and 2 is always the minimum possible')
            print('    }')
        print('    // save parameters')
        print('    gTraceOut->callMutex.lock();')
        print('    char* dest = gTraceOut->writebuf;')
        if func.name == 'glEGLImageTargetTexture2DOES':
            print()
            print('    // Firstly, insert an eglDestroyImageKHR')
            print('    EGLDisplay dpy = _eglGetCurrentDisplay();')
            print('    BCall *pCall0 = (BCall*)dest;')
            print('    pCall0->funcId = eglDestroyImageKHR_id;')
            print('    pCall0->tid = tid; pCall0->reserved = 0; pCall0->source = 1;')
            print('    dest += sizeof(*pCall0);')
            print('    dest = WriteFixed<int>(dest, (intptr_t)dpy); // int pointer')
            print('    dest = WriteFixed<int>(dest, (intptr_t)image); // int pointer')
            print('    dest = WriteFixed<int>(dest, EGL_TRUE); // enum')
            print('    gTraceOut->callNo++;')
            print()
            print('    // Secondly, save an eglCreateImageKHR')
            print('    char *starting_point2 = dest;')
            print('    EGLContext ctx = _eglGetCurrentContext();')
            print('    BCall_vlen *pCall1 = (BCall_vlen*)dest;')
            print('    pCall1->funcId = eglCreateImageKHR_id;')
            print('    pCall1->tid = tid; pCall1->reserved = 0; pCall1->source = 1;')
            print('    dest += sizeof(*pCall1);')
            print('    dest = WriteFixed<int>(dest, (intptr_t)dpy); // int pointer')
            print('    dest = WriteFixed<int>(dest, (intptr_t)ctx); // int pointer')
            print('    dest = WriteFixed<int>(dest, EGL_GL_TEXTURE_2D_KHR); // enum')
            print('    dest = WriteFixed<int>(dest, (intptr_t)buffer); // int pointer')
            print('    dest = Write1DArray<unsigned int>(dest, _AttribPairList_size(attrib_list, EGL_NONE), (unsigned int*)attrib_list); // array')
            print('    dest = WriteFixed<int>(dest, (intptr_t)image); // int pointer')
            print('    pCall1->toNext = dest-starting_point2;')
            print('    gTraceOut->callNo++;')
            print()
            print('    // finally, save glEGLImageTargetTexture2DOES')

        if gIdToLength[func.id] == '0':
            print('    BCall_vlen *pCall = (BCall_vlen*)dest;')
        else:
            print('    BCall *pCall = (BCall*)dest;')
        if func.name in ['eglCreateWindowSurface', 'glLinkProgram']: # replacement functions
            print('    pCall->funcId = %s2_id;' % (func.name))
        else:
            print('    pCall->funcId = %s_id;' % (func.name))
        print('#ifdef DEBUG')
        print('    if (pCall->funcId > common::ApiInfo::MaxSigId)')
        print('    {')
        print('        DBG_LOG("Fatal error: Trying to write bad func ID for %s!\\n");' % func.name)
        print('        abort();')
        print('    }')
        print('#endif')
        print('    pCall->tid = tid;')
        print('    pCall->source = %s;' % ('0' if not mark_as_injected else '1'))
        print('    pCall->reserved = 0;')
        print('    dest += sizeof(*pCall);')
        print()

        for arg in func.args:
            SerializeVisitor().visit(arg.type, arg.name, func)
        if func.name == 'eglCreateWindowSurface':
            SerializeVisitor().visit(stdapi.Int, 'x', func)
            SerializeVisitor().visit(stdapi.Int, 'y', func)
            SerializeVisitor().visit(stdapi.Int, 'width', func)
            SerializeVisitor().visit(stdapi.Int, 'height', func)
        if func.name == 'glLinkProgram':
            SerializeVisitor().visit(stdapi.UChar, 'link_status', func)
        if func.type is not stdapi.Void:
            SerializeVisitor().visit(func.type, '_result', func)
        if func.name.startswith('gl') and func.name != 'glGetError':
            print('    pCall->errNo = GetCallErrorNo("%s", tid);' % func.name)
        if gIdToLength[func.id] == '0':
            print('    pCall->toNext = dest-gTraceOut->writebuf;')
            print('#ifdef DEBUG')
            print('    if (pCall->toNext == 0)')
            print('    {')
            print('        DBG_LOG("Zero-length variable call detected for %s in call %%d\\n", (int)gTraceOut->callNo);' % func.name)
            print('        abort();')
            print('    }')
            print('#endif')

        print('    gTraceOut->WriteBuf(dest);')
        print('    gTraceOut->callNo++;')
        print('    gTraceOut->callMutex.unlock();')

    def invokeFunction(self, func, prefix='_', suffix='', indent='    '):
        if func.name in ignore_functions:
            print('%s// deliberately not calling the function while tracing' % indent)
            return
        print('%s// invokeFunction' % indent)
        if func.name == 'glCreateShaderProgramv' or func.name == 'glCreateShaderProgramvEXT':
            print('%s++gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s_result = replace_glCreateShaderProgramv(tid, type, count, strings);' % indent)
            print('%s--gTraceThread.at(tid).mCallDepth;' % indent)
            return
        if func.name == 'glStateDump_ARM':
            print('%sgTraceOut->getStateLogger().logState(tid);' % indent)
            return
        if func.name in ['glClientSideBufferData',
                         'glClientSideBufferSubData',
                         'glCreateClientSideBuffer',
                         'glDeleteClientSideBuffer',
                         'glCopyClientSideBuffer',
                         'glPatchClientSideBuffer',
                         'glGenGraphicBuffer_ARM',
                         'glGraphicBufferData_ARM',
                         'glDeleteGraphicBuffer_ARM',
                         'glLinkProgram',
                         'paMandatoryExtensions',
                         'glAssertBuffer_ARM',
                         ]:
            return

        if func.name == 'glGetError':
            print('    if (tracerParams.EnableErrorCheck) {')
            print('        _result = GetCurTraceContext(tid)->lastGlError;')
            print('        GetCurTraceContext(tid)->lastGlError = GL_NO_ERROR;')
            print('    }')
            print('    else {')
            print('        _result = _glGetError();')
            print('    }')
            return

        if func.type is stdapi.Void:
            result = ''
        else:
            result = '_result = '
        dispatch = prefix+func.name+suffix
        params = ', '.join([str(arg.name) for arg in func.args])

        if func.name == 'glMapBufferRange':
            params = 'target, offset, length, readAccess'

        printf_types = ', '.join([
            type_to_printf.get(str(arg.type), '%d')
            for arg in func.args
        ])

        appendParams = ''
        if params:
            appendParams = ', ' + params

        print('%s// DBG_LOG("Invoking: %s(%s)\\n"%s);' % (indent, dispatch, printf_types, appendParams))

        if func.name == 'glGetStringi':
            print('%sif (name == GL_EXTENSIONS && tracerParams.FilterSupportedExtension)' % indent)
            print('%s{' % indent)
            print('%s    if (index >= tracerParams.SupportedExtensions.size())' % indent)
            print('%s    {' % indent)
            print('%s        DBG_LOG("Invalid index %%u to extension list! Max is %%u.\\n", index, (unsigned)tracerParams.SupportedExtensions.size());' % indent)
            print('%s        return (GLubyte*)"";' % indent)
            print('%s    }' % indent)
            print('%s    _result = (const GLubyte*)tracerParams.SupportedExtensions[index].c_str();' % indent)
            print('%s}' % indent)
            print('%selse' % indent)
            print('%s{' % indent)
            print('%s    ++gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s    %s%s(%s);' % (indent, result, dispatch, params))
            print('%s    --gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s    const char *str = reinterpret_cast<const char*>(_result);' % indent)
            print('%s    if (name == GL_EXTENSIONS && _result)' % indent)
            print('%s    {' % indent)
            print('%s        if ((tracerParams.ErrorOutOnBinaryShaders && (strcmp(str, "GL_ARM_mali_shader_binary") == 0 || strcmp(str, "GL_ARM_mali_program_binary") == 0 || strcmp(str, "GL_OES_get_program_binary") == 0))' % indent)
            print('%s            || (tracerParams.DisableBufferStorage && strcmp(str, "GL_EXT_buffer_storage") == 0))' % indent)
            print('%s        {' % indent)
            print('%s            gTraceOut->callMutex.lock();' % indent)
            print('%s            static std::string s;' % indent)
            print('%s            s = str;' % indent)
            print("%s            std::replace(s.begin(), s.end(), '_', 'X'); // make it not match anything anymore" % indent)
            print('%s            _result = reinterpret_cast<const GLubyte*>(s.c_str());' % indent)
            print('%s            gTraceOut->callMutex.unlock();' % indent)
            print('%s        }' % indent)
            print('%s    }' % indent)
            print('%s}' % indent)
            return

        if func.name == 'eglSwapBuffersWithDamageKHR':
            print('%s++gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s%s%s(%s);' % (indent, result, dispatch, params))
            print('%sif (!_result)' % indent)
            print('%s{' % indent)
            print('%s    // the eglSwapBuffersWithDamageKHR may not be supported so fall back eglSwapBuffers' % indent)
            print('%s    _result = _eglSwapBuffers(dpy, surface);' % indent)
            print('%s}' % indent)
            print('%s--gTraceThread.at(tid).mCallDepth;' % indent)
            return

        if func.name in ['glMapBuffer', 'glMapBufferOES']:
            print('%sGLint length;' % indent)
            print()
            print('%s++gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s%s%s(%s);' % (indent, result, dispatch, params))
            print('%s_glGetBufferParameteriv(target, GL_BUFFER_SIZE, &length);' % indent)
            print('%s_glUnmapBuffer(target);' % indent)
            print('%s// Overwrite the glMapBufferOES with glMapBufferRange for reading the buffer contents' %indent)
            print('%s_result = _glMapBufferRange(target, 0, length, (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));' %indent)
            print('%sif (!_result)' %indent)
            print('%s{' %indent)
            print('%s    DBG_LOG("the glMapBufferRange() is not supported, fall back to %s()\\n");' % (indent, func.name))
            print('%s    %s%s(%s);' % (indent, result, dispatch, params))
            print('%s}' %indent)
            print('%s--gTraceThread.at(tid).mCallDepth;' % indent)
            print('%sGetCurTraceContext(tid)->isFullMapping = true;' % indent)
            return

        # Force glProgramBinary to fail.  Per ARB_get_program_binary this should signal the app that it needs to recompile.
        if func.name in ('glProgramBinary', 'glProgramBinaryOES'):
            print('%sif (tracerParams.ErrorOutOnBinaryShaders)' % indent)
            print('%s{' % indent)
            print('%s    binaryFormat = 0xDEADDEAD;' % indent)
            print('%s    _glProgramBinary(program, binaryFormat, &binaryFormat, sizeof(binaryFormat));' % indent)
            print('%s    _glGetError(); // clear error code because we get an Android 7 crash otherwise' % indent)
            print('%s    DBG_LOG("Deliberately failing a %s call\\n");' % (indent, func.name))
            print('%s}' % indent)
            print('%selse // run normally (with binary shaders - yay)' % indent)
            print('%s{' % indent)
            print('%s    ++gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s    %s%s(%s);' % (indent, result, dispatch, params))
            print('%s    --gTraceThread.at(tid).mCallDepth;' % indent)
            print('%s    DBG_LOG("Calling %s -- your trace will contain binary shaders!\\n");' % (indent, func.name))
            print('%s}' % indent)
            return

        print()
        print('%s++gTraceThread.at(tid).mCallDepth;' % indent)
        print('%s%s%s(%s);' % (indent, result, dispatch, params))
        print('%s--gTraceThread.at(tid).mCallDepth;' % indent)
        print()

        if func.name == 'glGetString':
            print('%sif (name == GL_EXTENSIONS && tracerParams.FilterSupportedExtension)' % indent)
            print('%s{' % indent)
            print('%s    string devExt = reinterpret_cast<const char*>(_result);' % indent)
            print('%s    vector<string>& cfgExts = tracerParams.SupportedExtensions;' % indent)
            print('%s    vector<string> notSupportedExts;' % indent)
            #print r'%s    DBG_LOG("The device reports these extensions:\n");' % indent
            #print r'%s    size_t pos = 0;' % indent
            #print r'%s    while (pos < devExt.size())' % indent
            #print r'%s    {' % indent
            #print r'%s        DBG_LOG("%%s\n",  devExt.substr(pos, 70).c_str());' % indent
            #print r'%s        pos += 70;' % indent
            #print r'%s    }' % indent
            print()
            print('%s    for (unsigned int i = 0; i < cfgExts.size(); ++i)' % indent)
            print('%s    {' % indent)
            print('%s        if (devExt.find(cfgExts[i]) == string::npos)' % indent)
            print('%s            notSupportedExts.push_back(cfgExts[i]);' % indent)
            print('%s    }' % indent)
            print()
            print('%s    if (notSupportedExts.size() > 0)' % indent)
            print('%s    {' % indent)
            print('%s        DBG_LOG("This device doesn\'t support the following extensions listed in /system/lib/egl/tracerparams.cfg.");' % indent)
            print('%s        for (unsigned int i = 0; i < notSupportedExts.size(); ++i)' % indent)
            print(r'%s            DBG_LOG("    \'%%s\'\n", notSupportedExts[i].c_str());' % indent)
            print('%s        DBG_LOG("Aborting...");' % indent)
            print('%s        os::abort();' % indent)
            print('%s    }' % indent)
            print('%s    _result = reinterpret_cast<const GLubyte*>(tracerParams.SupportedExtensionsString.c_str());' % indent)
            #print r'%s      DBG_LOG("Fake extensions: \'%%s\'\n", (const char*)_result);' % indent
            print('%s}' % indent)
            print('%sif (name == GL_EXTENSIONS && tracerParams.ErrorOutOnBinaryShaders) // remove binary shader support by default' % indent)
            print('%s{' % indent)
            print('%s    gTraceOut->callMutex.lock();' % indent)
            print('%s    vector<string> target;' % indent)
            print('%s    target.push_back("GL_ARM_mali_shader_binary");' % indent)
            print('%s    target.push_back("GL_ARM_mali_program_binary");' % indent)
            print('%s    target.push_back("GL_OES_get_program_binary");' % indent)
            print('%s    tracerParams._tmp_extensions = removeString(_result, target);' % indent)
            print('%s    _result = reinterpret_cast<const GLubyte*>(tracerParams._tmp_extensions.c_str());' % indent)
            print('%s    gTraceOut->callMutex.unlock();' % indent)
            print('%s}' % indent)
            print('%sif (name == GL_EXTENSIONS && tracerParams.DisableBufferStorage) { // remove EXT_buffer_storage support' % indent)
            print('%s    gTraceOut->callMutex.lock();' % indent)
            print('%s    vector<string> target;' % indent)
            print('%s    target.push_back("GL_EXT_buffer_storage");' % indent)
            print('%s    tracerParams._tmp_extensions = removeString(_result, target);' % indent)
            print('%s    _result = reinterpret_cast<const GLubyte*>(tracerParams._tmp_extensions.c_str());' % indent)
            print('%s    gTraceOut->callMutex.unlock();' % indent)
            print('%s}' % indent)
            print('%sif (name == GL_RENDERER && tracerParams.RendererName != "") {      // modify renderer\'s name' % indent)
            print('%s    _result = reinterpret_cast<const GLubyte*>(tracerParams.RendererName.c_str());' % indent)
            print('%s}' % indent)
            print('%sif (name == GL_VERSION && tracerParams.EnableRandomVersion) {      // append a random to gl_version' % indent)
            print('%s    const char *renderer = reinterpret_cast<const char*>(_glGetString(GL_RENDERER));' % indent)
            print('%s    if (strstr(renderer, "Mali"))' % indent)
            print('%s    {' % indent)
            print('%s        if (tracerParams.RandomVersion == "")' % indent)
            print('%s        {' % indent)
            print('%s            std::string origStr = reinterpret_cast<const char*>(_result);' % indent)
            print('%s            std::random_device rd;' % indent)
            print('%s            std::default_random_engine rng(rd());' % indent)
            print('%s            int r = rng();' % indent)
            print('%s            std::stringstream ss;' % indent)
            print('%s            ss << origStr << "." << r;' % indent)
            print('%s            tracerParams.RandomVersion = ss.str();' % indent)
            print('%s        }' % indent)
            print('%s        _result = reinterpret_cast<const GLubyte*>(tracerParams.RandomVersion.c_str());' % indent)
            print('%s    }' % indent)
            print('%s}' % indent)


        if func.name in ['glGetIntegerv', 'glGetIntegerv64', 'glGetFloatv']:
            # enforce an alignment that works cross-platform
            print('%sif (pname == GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT)' % indent)
            print('%s{' %indent)
            print('%s    GLint value = tracerParams.ShaderStorageBufferOffsetAlignment;' % indent)
            print('%s    if (*data != value) DBG_LOG("Value of GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT modified from %%d to %%d\\n", (int)*data, value);' % indent)
            print('%s    *data = value;' % indent)
            print('%s}' % indent)
            print('%selse if (pname == GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT)' % indent)
            print('%s{' %indent)
            print('%s    GLint value = tracerParams.UniformBufferOffsetAlignment;' % indent)
            print('%s    if (*data != value) DBG_LOG("Value of GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT modified from %%d to %%d\\n", (int)*data, value);' % indent)
            print('%s    *data = value;' % indent)
            print('%s}' % indent)
            # remove binary program / shader support
            print('%selse if (tracerParams.ErrorOutOnBinaryShaders && (pname == GL_NUM_PROGRAM_BINARY_FORMATS || pname == GL_NUM_SHADER_BINARY_FORMATS))' % indent)
            print('%s{' % indent)
            print('%s    *data = 0;' % indent)
            print('%s}' % indent)
            # override extensions list
            print('%sif (pname == GL_NUM_EXTENSIONS && tracerParams.FilterSupportedExtension) *data = tracerParams.SupportedExtensions.size();' % indent)
            # make sure we have a compatible workgroup size
            print('%sif (pname == GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS) *data = 128;' % indent)
        if func.name in ['glGetIntegeri_v']:
            # make sure we have a compatible workgroup size
            print('%sif (target == GL_MAX_COMPUTE_WORK_GROUP_SIZE) *data = 128;' % indent)

        if func.name in ['glGetTexParameterfv', 'glGetTexParameteriv']:
            print('%sif (target == GL_TEXTURE_MAX_ANISOTROPY_EXT)' % indent)
            print('%s{' %indent)
            print('%s    if (tracerParams.MaximumAnisotropicFiltering != 0)' % indent)
            print('%s    {' % indent)
            print('%s        *params = tracerParams.MaximumAnisotropicFiltering;' %indent)
            print('%s    }' % indent)
            print('%s}' % indent)
        elif func.name in ['glGetBooleanv', 'glGetDoublev', 'glGetFloatv', 'glGetIntegerv', 'glGetIntegerv64']:
            print('%sif (pname == GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)' % indent)
            print('%s{' % indent)
            print('%s    if (tracerParams.MaximumAnisotropicFiltering != 0)' % indent)
            print('%s    {' % indent)
            print('%s        *data = tracerParams.MaximumAnisotropicFiltering;' % indent)
            print('%s    }' % indent)
            print('%s}' % indent)

    def traceApi(self, api):
        print('string removeString(const GLubyte *orig, const vector<string> &target) {')
        print('    std::string extensions = reinterpret_cast<const char*>(orig);')
        print('    istringstream iss(extensions);')
        print('    std::vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}};')
        print('    std::vector<string> retlist;')
        print('    for (const std::string& s : tokens)')
        print('    {')
        print('        bool found = false;')
        print('        for (const std::string &s1 : target)')
        print('        {')
        print('            if (s == s1) {')
        print('                found = true;')
        print('                break;')
        print('            }')
        print('        }')
        print('        if (!found)')
        print('        {')
        print('            retlist.push_back(s);')
        print('        }')
        print('    }')
        print('    std::string retval = std::accumulate(retlist.begin(), retlist.end(), std::string(), [](const std::string& a, const std::string& b) -> std::string { return a + (a.length() > 0 ? " " : "") + b; });')
        print('    return retval;')
        print('}')
        print()

        list(map(self.traceFunction, api.functions))
        for f in api.functions:
            if f.name in internal_functions:
                self.traceFunctionInject(f)
        print()
        print('__eglMustCastToProperFunctionPointerType _wrapProcAddress(')
        print('    const char * procName, __eglMustCastToProperFunctionPointerType procPtr) {')
        print('    if (!procPtr)')
        print('        return procPtr;')
        for function in api.functions:
            ptype = function_pointer_type(function)
            pvalue = function_pointer_value(function)
            print('    if (strcmp("%s", (const char*)procName) == 0) {' % function.name)
            print('        %s = (%s)procPtr;' % (pvalue, ptype))
            print('        return (__eglMustCastToProperFunctionPointerType)&%s;' % ("patrace_" + function.name))
            print('    }')
        print(r'    DBG_LOG("warning: unknown function %s\n", (const char*)procName);')
        print('    return procPtr;')
        print('}')

        print()
        print('#ifdef GLESLAYER')
        print('void layer_init_intercept_map()')
        print('{')
        print('    LAYER_DATA *layer = gLayerCollector[std::string(PATRACE_LAYER_NAME)];')
        for function in api.functions:
            print('    layer->interceptFuncMap[std::string("%s")] = (EGLFuncPointer) &%s;' % (function.name, "patrace_"+function.name))
        print('    DBG_LOG("layer_init_intercept_map called.\\n");')
        print('}')
        print('#endif // GLESLAYER')

    def traceFunctionBody_pre(self, func):
        print('    // traceFunctionBody_pre')
        if (func.name in array_pointer_function_names):
            print('    GLint _array_buffer = 0;')
            print('    _glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &_array_buffer);')
            print('    if (_array_buffer==0) {')
            self.invokeFunction(func, indent='        ')
            print('        return;')
            print('    }')

        instance_count = 'instancecount' if func.name in stdapi.draw_instanced_function_names else '0'

        if func.name in stdapi.draw_function_names and not func.name in stdapi.draw_indirect_function_names:
            print('    if (count && _need_user_arrays()) {')
            arg_names = ', '.join([arg.name for arg in func.args[1:]])
            print('        GLuint _count = _%s_count(%s);' % (func.name, arg_names))
            print('        _trace_user_arrays(_count, %s);' % instance_count)
            print('    }')
        if func.name in stdapi.draw_function_names or func.name == 'glDispatchCompute':
            print('    if (unlikely(stateLoggingEnabled)) {')
            print('        gTraceOut->getStateLogger().logFunction(tid, "' + func.name + '", gTraceOut->callNo, 0);')

        if func.name in stdapi.draw_array_function_names:
            if func.name not in stdapi.draw_indirect_function_names:
                print('        gTraceOut->getStateLogger().logState(tid, first, count, %s);' % instance_count)
            else:
                print('        gTraceOut->getStateLogger().logState(tid);')
            print('    }')
        elif func.name in stdapi.draw_elements_function_names:
            if func.name not in stdapi.draw_indirect_function_names:
                print('        gTraceOut->getStateLogger().logState(tid, count, type, indices, %s);' % instance_count)
            else:
                print('        gTraceOut->getStateLogger().logState(tid);')
            print('    }')
            print('    GLint _element_array_buffer = 0;')
            print('    _glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &_element_array_buffer);')
            if func.name not in stdapi.draw_indirect_function_names:
                print('    GLuint clientSideBufferObjName = 0;')
                print('#if ENABLE_CLIENT_SIDE_BUFFER')
                print('    if (!_element_array_buffer) {')
                print('        clientSideBufferObjName = _glClientSideBufferData(indices, count*_gl_type_size(type));')
                print('    }')
                print('#endif')
        elif func.name == 'glDispatchCompute':
            print('        gTraceOut->getStateLogger().logCompute(tid, num_groups_x, num_groups_y, num_groups_z);')
            print('    }')

        if func.name in ['glBindBuffer',
                         'glBindBufferBase',
                         'glBindBufferRange',
                         'glBufferData',
                         'glBufferSubData',
                         'glMapBuffer',
                         'glMapBufferOES',
                         'glMapBufferRange']:
            print('    updateUsage(target);')

        if func.name == 'glBufferData':
            print('    if (size < 0)')
            print('        return;')
            print('    GLint boundBuffer = getBoundBuffer(target);')
            print('    BufferInitializedSet_t &bufInitSet = GetCurTraceContext(tid)->bufferInitializedSet;')
            print('    if (data != NULL)')
            print('    {')
            print('        bufInitSet.insert(boundBuffer);')
            print('    }')
            print('    else')
            print('    {')
            print('        BufferInitializedSet_t::iterator it = bufInitSet.find(boundBuffer);')
            print('        if (it != bufInitSet.end())')
            print('            bufInitSet.erase(it);')
            print('    }')
        if func.name == 'glDeleteBuffers':
            print('    BufferInitializedSet_t &bufInitSet = GetCurTraceContext(tid)->bufferInitializedSet;')
            print('    for (int i = 0; i < n; ++i)')
            print('    {')
            print('        BufferInitializedSet_t::iterator it = bufInitSet.find(buffers[i]);')
            print('        if (it != bufInitSet.end())')
            print('            bufInitSet.erase(it);')
            print('    }')
            print()
        if func.name == 'glMapBufferRange':
            print('    GLbitfield readAccess = access & ~GL_MAP_INVALIDATE_RANGE_BIT & ~GL_MAP_INVALIDATE_BUFFER_BIT;')
        if func.name == 'glFlushMappedBufferRange':
            print('    pre_glFlushMappedBufferRange(target, offset, length);')
        if func.name in ['glUnmapBuffer', 'glUnmapBufferOES']:
            print('    pre_glUnmapBuffer(target);')
        if func.name == 'glLinkProgram':
            print('    bool link_status = pre_glLinkProgram(tid, program);')
        if func.name in ['eglSwapBuffers',
                         'eglSwapBuffersWithDamageKHR']:
            print('    pre_eglSwapBuffers();')

    def traceFunctionBody_after(self, func):
        print('    // traceFunctionBody_after')
        if func.name == 'eglInitialize':
            print('    after_eglInitialize(dpy);')
        if func.name == 'eglTerminate':
            print('    if (tracerParams.CloseTraceFileByTerminate)')
            print('    {')
            print('        DBG_LOG("eglTerminate called -- closing file handle\\n");')
            print('        gTraceOut->Close();')
            print('    }')
        if func.name == 'eglCreateContext':
            print('    after_eglCreateContext(_result, dpy, config, attrib_list);')
        if func.name == 'eglDestroyContext':
            print('    DBG_LOG("eglDestroyContext called (implies flush to disk)\\n");')
            print('    after_eglDestroyContext(ctx);')
            print('    gTraceOut->mpBinAndMeta->writeHeader(true);')
        if func.name == 'eglMakeCurrent':
            print('    after_eglMakeCurrent(dpy, draw, ctx);')
        if func.name == 'eglCreateWindowSurface':
            print('    after_eglCreateWindowSurface(dpy, config, _result, x, y, width, height);')
        if func.name in ['eglSwapBuffers',
                         'eglSwapBuffersWithDamageKHR']:
            print('    after_eglSwapBuffers();')
        if func.name == 'eglDestroySurface':
            print('    after_eglDestroySurface(surface);')
        if func.name in ['glCompressedTexImage2D', 'glCompressedTexImage3D']:
            print('    if (gTraceOut->mpBinAndMeta == NULL)')
            print(r'        DBG_LOG("gTraceOut->mpBinAndMeta should not be NULL right now!\n");')
            print('    std::map<unsigned int, unsigned int> &texCompressFormats = gTraceOut->mpBinAndMeta->texCompressFormats;')
            print('    std::map<unsigned int, unsigned int>::iterator iter = texCompressFormats.find(internalformat);')
            print('    if (iter == texCompressFormats.end())')
            print('        texCompressFormats[internalformat] = 1;')
            print('    else')
            print('        iter->second += 1;')
        if func.name in ['glCompressedTexSubImage2D', 'glCompressedTexSubImage3D']:
            print('    if (gTraceOut->mpBinAndMeta == NULL)')
            print(r'        DBG_LOG("gTraceOut->mpBinAndMeta should not be NULL right now!\n");')
            print('    std::map<unsigned int, unsigned int> &texCompressFormats = gTraceOut->mpBinAndMeta->texCompressFormats;')
            print('    std::map<unsigned int, unsigned int>::iterator iter = texCompressFormats.find(format);')
            print('    if (iter == texCompressFormats.end())')
            print('        texCompressFormats[format] = 1;')
            print('    else')
            print('        iter->second += 1;')
        if func.name == 'eglGetProcAddress':
            print('#ifndef GLESLAYER')
            print('    _result = _wrapProcAddress(procname, _result);')
            print('#endif  // !GLESLAYER')
        if func.name in ['glCreateProgram']:
            print('    after_glCreateProgram(tid, _result);')
        if func.name == 'glDeleteProgram':
            print('    after_glDeleteProgram(tid, program);')
        if func.name == 'glBindAttribLocation':
            print('    after_glBindAttribLocation(tid, program, index);')
        if func.name in ['glMapBufferRange', 'glMapBuffer', 'glMapBufferOES']:
            print('    after_glMapBufferRange(target, length, access, _result);')
        if func.name == 'glMapBufferRange':
            print('    GetCurTraceContext(tid)->isFullMapping = false;')
        if func.name in stdapi.draw_function_names:
            print('    after_glDraw();')
        if func.name == ['glUnmapBufferOES', 'glUnmapBuffer']:
            print('    after_glUnmapBuffer(target);')

        params = ', '.join([str(arg.name) for arg in func.args])

        if func.name in post_functions:
            print('    after_%s(%s);' % (func.name, params))

class TypeLengthVisitor(stdapi.Visitor):
    def __init__(self):
        self.len = 'variable'

    def visitVoid(self, void):
        self.len = '0'
    def visitLiteral(self, literal):
        self.len = 'PAD_SIZEOF(%s)' % (literal)
    def visitString(self, string):
        pass
    def visitConst(self, const):
        self.visit(const.type)
    def visitStruct(self, struct):
        pass
    def visitArray(self, array):
        #TODO LZ:
        pass
    def visitBlob(self, blob):
        pass
    def visitEnum(self, enum):
        self.len = 'PAD_SIZEOF(int)'
    def visitBitmask(self, bitmask):
        self.visit(bitmask.type)
    def visitPointer(self, pointer):
        pass
    def visitIntPointer(self, pointer):
        self.len = 'PAD_SIZEOF(int)'
    def visitObjPointer(self, pointer):
        pass
    def visitLinearPointer(self, pointer):
        pass
    def visitReference(self, reference):
        pass
    def visitHandle(self, handle):
        self.visit(handle.type)
        # TODO: LZ:
    def visitAlias(self, alias):
        self.visit(alias.type)
    def visitOpaque(self, opaque):
        pass
    def visitInterface(self, interface):
        pass
    def visitPolymorphic(self, polymorphic):
        pass

def funcSerializedLength(func):
    funcLen = 'sizeof(BCall)'
    visitor = TypeLengthVisitor()
    visitor.visit(func.type)
    if visitor.len == 'variable':
        return '0'
    funcLen += ('+'+visitor.len)
    for arg in func.args:
        visitor.len = 'variable'
        visitor.visit(arg.type)
        if visitor.len == 'variable':
            return '0'
        funcLen += ('+'+visitor.len)
    return funcLen

def parseFunctions(functions):
    global gMaxId
    global gIdToFunc
    global gIdToLength

    for func in functions:
        if func.id > gMaxId:
            gMaxId = func.id
    gIdToFunc = dict([(func.id, func) for func in functions])
    gIdToLength = dict([func.id, funcSerializedLength(func)] for func in functions)

def sigEnum(functions):
    print('enum SigEnum {')
    for func in functions:
        print('    %s_id = %d,' % (func.name, func.id))
    print('};')

if __name__ == '__main__':

    tracer = Tracer()

    api.addApi(gles12api.glesapi)
    api.addApi(eglapi.eglapi)
    parseFunctions(api.functions)

    script_dir = os.path.dirname(os.path.realpath(__file__))
    #############################################################
    ##
    file_path = os.path.join(script_dir, 'sig_enum.hpp')
    with open(file_path, 'w') as f:
        orig_stdout = sys.stdout
        sys.stdout = f
        print('#ifndef _TRACER_SIG_ENUM_HPP_')
        print('#define _TRACER_SIG_ENUM_HPP_')
        print('// this file was generated by trace.py')
        sigEnum(api.functions)
        print()
        for f in api.functions:
            if f.name in internal_functions:
                print(f.prototype('inject_' + f.name) + ';')
        print()
        print('#endif')
        sys.stdout = orig_stdout

    #############################################################
    ##
    file_path = os.path.join(script_dir, 'egltrace_auto.cpp')
    with open(file_path, 'w') as f:
        orig_stdout = sys.stdout
        sys.stdout = f
        print('#include <tracer/egltrace.hpp>')
        print()
        print('#include <tracer/sig_enum.hpp>')
        print('#include <tracer/tracerparams.hpp>')
        print('#include <dispatch/eglproc_auto.hpp>')
        print('#include <helper/eglsize.hpp>')
        print('#include "helper/states.h"')
        print('#include <common/trace_model.hpp>')
        print('#include "common/os.hpp"')
        print()
        print('#include <inttypes.h>')
        print('#include <vector>')
        print('#include <string>')
        print('#include <numeric>')
        print('#include <iostream>')
        print('#include <iterator>')
        print('#include <algorithm>')
        print('#include <random>')
        print('#include "dispatch/gleslayer_helper.h"')
        print()
        print('using namespace common;')
        print('using namespace std;')
        print()
        print('// clang also supports gcc pragmas')
        print('#pragma GCC diagnostic ignored "-Wunused-variable"')
        print()
        print()
        api.delFunctionByName("glClientSideBufferData")
        api.delFunctionByName("glClientSideBufferSubData")
        api.delFunctionByName("glCreateClientSideBuffer")
        api.delFunctionByName("glDeleteClientSideBuffer")
        api.delFunctionByName("glCopyClientSideBuffer")
        api.delFunctionByName("glPatchClientSideBuffer")
        api.delFunctionByName("glGenGraphicBuffer_ARM")
        api.delFunctionByName("glGraphicBufferData_ARM")
        api.delFunctionByName("glDeleteGraphicBuffer_ARM")
        api.delFunctionByName("paMandatoryExtensions")
        tracer.traceApi(api)
        sys.stdout = orig_stdout
