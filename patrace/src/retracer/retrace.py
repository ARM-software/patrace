from __future__ import print_function
import os.path
import sys
import argparse

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import specs.stdapi as stdapi
import specs.gles12api as gles12api

broken_funcs = [ # these fail to compile and should be fixed
    'glGetProgramResourceName',
    'glGetProgramResourceiv',
    'glGetSynciv',
    'glGetTransformFeedbackVarying',
    'glGetProgramPipelineInfoLog',
    'glGetProgramBinary',
    'glGetBufferPointerv',
    'glGetActiveUniformBlockName',
    'glGetPerfQueryInfoINTEL',
    'glGetPerfQueryIdByNameINTEL',
    'glGetPerfQueryDataINTEL',
    'glGetPerfCounterInfoINTEL',
    'glGetNextPerfQueryIdINTEL',
    'glGetFirstPerfQueryIdINTEL',
    'glCreatePerfQueryINTEL',
    'glGetPerfMonitorCounterDataAMD',
    'glGetPerfMonitorCounterInfoAMD',
    'glGetPerfMonitorCounterStringAMD',
    'glGetPerfMonitorGroupStringAMD',
    'glGetPerfMonitorCountersAMD',
    'glGetPerfMonitorGroupsAMD',
    'glGetObjectLabel',
    'glGetObjectLabelKHR',
    'glGetProgramPipelineInfoLogEXT',
    'glGetObjectLabelEXT',
    'glGetProgramBinaryOES',
    'glGetBufferPointervOES',
    'glGetPointervKHR',
    'glGetPointerv',
    'glGetVertexAttribPointerv',
    'glGetShaderSource',
    'glGetShaderInfoLog',
    'glGetProgramInfoLog',
    'glGetAttachedShaders',
    'glGetActiveUniform',
    'glGetActiveAttrib',
    'glDebugMessageCallbackKHR',
    'glDebugMessageCallback',
    'glGetDebugMessageLogKHR',
    'glGetDebugMessageLog',
    'glGetFragmentShadingRatesEXT', # Out(Pointer(basic type)) broken

    # For the below: Their array of values is stupidly stored since the apitrace days as a pointer value. That
    # means we do not have the original values nor can we infer their size from the size in the original call.
    # And there is no easy way to introspect what size it should be using only the location info.
    'glGetUniformuiv',
    'glGetUniformiv',
    'glGetUniformfv',
]

check_ret_funcs = [
    'glIsEnabledi', 'glGetGraphicsResetStatusEXT', 'glGetFramebufferPixelLocalStorageSizeEXT',
    'glIsVertexArray', 'glIsTransformFeedback', 'glIsProgramPipeline', 'glIsQuery', 'glIsSampler',
    'glIsSync', 'glIsTransformFeedback', 'glGetProgramResourceIndex', 'glGetProgramResourceLocation',
    'glGetGraphicsResetStatusKHR', 'glIsEnablediEXT', 'glIsEnablediOES', 'glGetError', 'glIsEnabled',
    'glIsBuffer', 'glIsFramebuffer', 'glIsProgram', 'glIsRenderbuffer', 'glIsShader',
    'glIsTexture', 'glIsRenderbufferOES', 'glIsFramebufferOES', 'glIsVertexArrayOES', 'glIsQueryEXT',
    'glIsProgramPipelineEXT', 'glGetFragDataLocation', 'glGetGraphicsResetStatus',
]

notSupportedFuncs = set([
    'glVertexAttrib1fv',
])

autogenBindFuncs = {
    'glBindBuffer': 'glGenBuffers',
    'glBindFramebuffer': 'glGenFramebuffers',
    'glBindRenderbuffer': 'glGenRenderbuffers',
    'glBindTexture': 'glGenTextures',
    'glBindTransformFeedback': 'glGenTransformFeedbacks',
    'glBindVertexArray': 'glGenVertexArrays',
    'glBindSampler': 'glGenSamplers',
}

bind_framebuffer_function_names = set([
    "glBindFramebuffer",
    "glBindFramebufferEXT",
    "glBindFramebufferOES",
])

# These functions do not use the return value when retracing
ignored_ret_val_functions = [
    "glGetAttribLocation",
    "glCheckFramebufferStatusOES",
    "glQueryMatrixxOES",
    "glUnmapBufferOES",
    "glUnmapBuffer",
    "glCreateClientSideBuffer",
    "glCheckFramebufferStatus",
    "glClientWaitSync",
]

ignored_funcs = [
    'glDebugMessageCallback',
    'glDebugMessageControl',
]

# For these shader related functions, defer these calls if shader cache is enabled'.
shadercache_funcs = [
    'glCompileShader', 'glAttachShader', 'glShaderSource', 'glGetShaderInfoLog', 'glGetShaderiv'
]

# Query Objcet
get_query_object_funcs = [
    'glGetQueryObjectuiv', 'glGetQueryObjectui64vEXT', 'glGetQueryObjecti64vEXT',
    'glGetQueryObjectivEXT', 'glGetQueryObjectuivEXT'
]

# Filled out in main()
reverse_lookup_maps = set(["program", "shader" ,"pipeline", "texture", "buffer"])

maps_with_getters = [
    'texture',
    'buffer',
    'program',
    'shader',
    'renderbuffer',
    'sampler',
    'sync',
    'uniformLocation',
]


def isString(ty):
    if isinstance(ty, stdapi.String):
        return True
    elif isinstance(ty, stdapi.Const):
        return isString(ty.type)
    else:
        return False


def lookupHandleAsR(handle, value):
    return lookupHandle(handle, value, 'RValue')


def lookupHandleAsL(handle, value):
    return lookupHandle(handle, value, 'LValue')


def _lookupHandle(handle, value, function, member):
    if handle.key is None:
        return "context.%s.%s(%s)" % (member, function, value)
    else:
        key_name, key_type = handle.key
        return "context.%s[%s].%s(%s)" % (member, key_name + 'New', function, value)


def lookupHandle(handle, value, function):
    if handle.name in maps_with_getters:
        member = 'get{name}Map()'.format(name=handle.name.capitalize())
    else:
        member = '_{name}_map'.format(name=handle.name)

    return _lookupHandle(handle, value, function, member)


def reverseLookupHandleAsL(handle, value):
    return reverseLookupHandle(handle, value, 'LValue')


def reverseLookupHandle(handle, value, function):
    if handle.name in maps_with_getters:
        member = 'get{name}RevMap()'.format(name=handle.name.capitalize())
    else:
        member = '_{name}_rev_map'.format(name=handle.name)

    return _lookupHandle(handle, value, function, member)


class DeserializeVisitor(stdapi.Visitor):
    def visitVoid(self, void, arg, name, func):
        pass

    def visitLiteral(self, literal, arg, name, func):
        print('    %s %s; // literal' % (literal, name))
        print('    _src = ReadFixed<%s>(_src, %s);' % (literal, name))
        #if literal.expr == 'float':
        #    print '    DBG_LOG("%%f", %s);' % name
        #else:
        #    print '    DBG_LOG("%%d", %s);' % name

    def visitString(self, string, arg, name, func):
        print('    char* %s; // string' % (name))
        print('    _src = ReadString(_src, %s);' % (name))
        #print '    DBG_LOG("%%s", %s);' % name

    def visitConst(self, const, arg, name, func):
        self.visit(const.type, arg, name, func)

    def visitStruct(self, struct, arg, name, func):
        print('    #error')

    def visitArray(self, array, arg, name, func):
        eleSerialType = stdapi.getSerializationType(array.type)
        if isString(array.type):
            print('    Array<const char*> %s; // string array' % (name))
            print('    _src = ReadStringArray(_src, %s);' % (name))
            #print '    for (unsigned int i = 0; i < %s.cnt; ++i)' % (name)
            #print '        DBG_LOG("%%s", %s.v[i]);' % (name)
        else:
            print('    Array<%s> %s; // array' % (eleSerialType, name))
            print('    _src = Read1DArray(_src, %s);' % (name))
            #print '    for (unsigned int i = 0; i < %s.cnt; ++i)' % (name)
            #print '        DBG_LOG("%%f", %s.v[i]);' % (name)

    def visitBlob(self, blob, arg, name, func, indent=4):
        print(' ' * indent + 'Array<char> %s; // blob' % (name))
        print(' ' * indent + '_src = Read1DArray(_src, %s);' % (name))

    def visitEnum(self, enum, arg, name, func):
        print('    int %s; // enum' % (name))
        print('    _src = ReadFixed(_src, %s);' % (name))
        if ((func.name in stdapi.compressed_texture_function_names and name == 'internalformat') or
	    (func.name in stdapi.compressed_sub_texture_function_names and name == 'format')):
            print('#ifdef __APPLE__')
            print('    // Since we\'re only supporting Apple devices with GLES3')
            print('    // (which implies ETC2 support), we take advantage of ETC2\'s')
            print('    // backwards compatibility to get ETC1 support as well.')
            print('    // (This only makes sense for the internalformat, but is added to all parsed enums.)')
            print('    if (%s == GL_ETC1_RGB8_OES) {' % (name))
            print('        %s = GL_COMPRESSED_RGB8_ETC2;' % (name))
            print('    }')
            print('#endif')
        #print '    DBG_LOG("%%d", %s);' % name
    def visitBitmask(self, bitmask, arg, name, func):
        self.visit(bitmask.type, arg, name, func)

    def visitPointer(self, pointer, arg, name, func):
        print('    %s* %s; // pointer' % (pointer.type, name))
        print('    unsigned int isValidPtr;')
        print('    _src = ReadFixed(_src, isValidPtr);')
        print('    if (isValidPtr)')
        print('        _src = ReadFixed<%s>(_src, %s)' % (stdapi.getSerializationType(pointer.type), name))

    def visitIntPointer(self, pointer, arg, name, func):
        print('    #error // this should not be used in gles calls')

    def visitObjPointer(self, pointer, arg, name, func):
        print('    #error // obj pointer')

    def visitLinearPointer(self, pointer, arg, name, func):
        print('    unsigned int %s; // linear pointer' % name)
        print('    _src = ReadFixed<unsigned int>(_src, %s);' % name)

    def visitReference(self, reference, arg, name, func):
        print('    #error')

    def visitHandle(self, handle, arg, name, func):
        self.visit(handle.type, arg, name, func)
        # TODO: LZ:

    def visitAlias(self, alias, arg, name, func):
        self.visit(alias.type, arg, name, func)

    def visitOpaque(self, opaque, arg, name, func):
        if func.name in stdapi.texture_function_names:
            print('    GLvoid* %s = NULL;' % (name))
            print('    Array<char> %sBlob;' % (name))
            print('    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)')
            print('    {')
            print('        _src = Read1DArray(_src, %sBlob);' % (name))
            print('        %s = %sBlob.v;' % (name, name))
            print('    }')
            print('    else')
            print('    {')
            print('        unsigned int _opaque_type = 0;')
            print('        _src = ReadFixed(_src, _opaque_type);')
            print('        switch (_opaque_type)')
            print('        {')
            print('        case BlobType:')
            print('            _src = Read1DArray(_src, %sBlob);' % (name))
            print('            %s = %sBlob.v;' % (name, name))
            print('            break;')
            print('        case BufferObjectReferenceType:')
            print('            unsigned int bufferOffset = 0;')
            print('            _src = ReadFixed<unsigned int>(_src, bufferOffset);')
            print('            %s = (GLvoid *)static_cast<uintptr_t>(bufferOffset);' % (name))
            print('            break;')
            print('        }')
            print('    }')
        elif func.name in ["glReadPixels", 'glReadnPixels', 'glReadnPixelsEXT', 'glReadnPixelsKHR']:
            print('    GLvoid* %s = NULL;' % (name))
            print('    bool isPixelPackBufferBound = false;')
            print('    unsigned int _opaque_type = 0;')
            print('    _src = ReadFixed(_src, _opaque_type);')
            print('    if (gRetracer.getFileFormatVersion() <= HEADER_VERSION_3)')
            print('    {')
            print('       unsigned int t = 0;')
            print('       _src = ReadFixed<unsigned int>(_src, t);')
            print('       // old_pixels will not be used')
            print('       %s = (GLvoid *)static_cast<uintptr_t>(t);' % (name))
            print('    }')
            print('    else // if (gRetracer.getFileFormatVersion() > HEADER_VERSION_3)')
            print('    {')
            print('        if (_opaque_type == BufferObjectReferenceType)')
            print('        {')
            print('            isPixelPackBufferBound = true;')
            print('            unsigned int bufferOffset = 0;')
            print('            _src = ReadFixed<unsigned int>(_src, bufferOffset);')
            print('            %s = (GLvoid *)static_cast<uintptr_t>(bufferOffset);' % (name))
            print('        }')
            print('        else if (_opaque_type == NoopType)')
            print('        {')
            print('        }')
            print('        else')
            print('        {')
            print('            DBG_LOG("ERROR: Unsupported opaque type\\n");')
            print('        }')
            print('    }')
        else:
            print('    unsigned int csbName = 0; // ClientSideBuffer name')
            print('    GLvoid* %s = NULL;' % (name))
            print('    unsigned int _opaque_type = 0;')
            print('    _src = ReadFixed(_src, _opaque_type);')
            print('    if (_opaque_type == BufferObjectReferenceType) { // simple memory offset')
            print('        unsigned int %s_raw; // raw ptr' % (name))
            print('        _src = ReadFixed<unsigned int>(_src, %s_raw);' % name)
            print('        %s = (GLvoid *)static_cast<uintptr_t>(%s_raw);' % (name, name))
            print('    } else if (_opaque_type == BlobType) { // blob type')
            print('        Array<float> %s_blob;' % (name))
            print('        _src = Read1DArray(_src, %s_blob);' % name)
            print('        %s = %s_blob.v;' % (name, name))
            print('    } else if (_opaque_type == ClientSideBufferObjectReferenceType) { // a memory reference to client-side buffer')
            print('        unsigned int offset = 0;')
            print('        _src = ReadFixed<unsigned int>(_src, csbName);')
            print('        _src = ReadFixed<unsigned int>(_src, offset);')
            print('        %s = gRetracer.mCSBuffers.translate_address(gRetracer.getCurTid(), csbName, (ptrdiff_t)offset);' % (name))
            print('    } else if (_opaque_type == NoopType) { // Do nothing')
            print('    }')

    def visitInterface(self, interface, arg, name, func):
        print('    #error')

    def visitPolymorphic(self, polymorphic, arg, name, func):
        print('    #error')


class outAllocateVisitor(stdapi.Visitor):

    def visitVoid(self, void, arg, name):
        pass

    def visitLiteral(self, literal, arg, name):
        print('    %s %s; // literal' % (literal, name))

    def visitString(self, string, arg, name):
        print('    #error // string')

    def visitConst(self, const, arg, name):
        self.visit(const.type, arg, name)

    def visitStruct(self, struct, arg, name):
        print('    #error')

    def visitArray(self, array, arg, name):
        print('    std::vector<%s> _%s(%s);' % (array.type, name, array.length))
        print('    %s *%s = _%s.data();' % (array.type, name, name))

    def visitBlob(self, blob, arg, name):
        print('    char %s[512]; // blob' % (name))

    def visitEnum(self, enum, arg, name):
        print('    int %s; // enum' % (name))

    def visitBitmask(self, bitmask, arg, name):
        self.visit(bitmask.type, arg, name)

    def visitPointer(self, pointer, arg, name):
        print(r'    DBG_LOG("Not supported yet!!!!!\\n");')

    def visitIntPointer(self, pointer, arg, name):
        print('    #error // int pointer')

    def visitObjPointer(self, pointer, arg, name):
        print('    #error // obj pointer')

    def visitLinearPointer(self, pointer, arg, name):
        print('    #error // linear pointer')

    def visitReference(self, reference, arg, name):
        print('    #error // reference')

    def visitHandle(self, handle, arg, name):
        self.visit(handle.type, arg, name)
        # TODO: LZ:

    def visitAlias(self, alias, arg, name):
        self.visit(alias.type, arg, name)

    def visitOpaque(self, opaque, arg, name):
        pass

    def visitInterface(self, interface, arg, name):
        print('    #error')

    def visitPolymorphic(self, polymorphic, arg, name):
        print('    #error')


class LookUpHandleVisitor(stdapi.Visitor):
    def __init__(self):
        self.inLoop = False

    def visitVoid(self, void, arg, lvalue, rvalue):
        pass

    def visitLiteral(self, literal, arg, lvalue, rvalue):
        pass

    def visitString(self, string, arg, lvalue, rvalue):
        pass

    def visitConst(self, const, arg, lvalue, rvalue):
        self.visit(const.type, arg, lvalue, rvalue)

    def visitStruct(self, struct, arg, lvalue, rvalue):
        print('    #error // struct')

    def visitArray(self, array, arg, lvalue, rvalue):
        if not isinstance(array.type.type, stdapi.Handle):
            return

        datatype = stdapi.getSerializationType(array.type)

        print('    {datatype}* {newname} = {oldname}.v;'.format(
            datatype=datatype,
            newname=lvalue,
            oldname=rvalue
        ))

        print('    for (unsigned int i = 0; i < %s.cnt; ++i) {' % (rvalue))
        self.inLoop = True
        self.visit(array.type, arg, lvalue + '[i]', rvalue + '[i]')
        print('    }')

    def visitBlob(self, blob, arg, lvalue, rvalue):
        pass

    def visitEnum(self, enum, arg, lvalue, rvalue):
        pass

    def visitBitmask(self, bitmask, arg, lvalue, rvalue):
        pass

    def visitPointer(self, pointer, arg, lvalue, rvalue):
        print(r'    DBG_LOG("Not supported yet!!!!!\\n");')

    def visitIntPointer(self, pointer, arg, lvalue, rvalue):
        print('    #error // int pointer')

    def visitObjPointer(self, pointer, arg, lvalue, rvalue):
        print('    #error // obj pointer')

    def visitLinearPointer(self, pointer, arg, lvalue, rvalue):
        print('    #error // linear pointer')

    def visitReference(self, reference, arg, lvalue, rvalue):
        print('    #error // reference')

    def visitHandle(self, handle, arg, lvalue, rvalue):
        arg.has_new_value = True
        if self.inLoop:
            formatstring = '        {variablename} = {value};'
        else:
            formatstring = '    {datatype} {variablename} = {value};'
        print(formatstring.format(datatype=handle,
                                  variablename=lvalue,
                                  value=lookupHandleAsR(handle, rvalue)))
        # TODO: LZ:

    def visitAlias(self, alias, arg, lvalue, rvalue):
        self.visit(alias.type, arg, lvalue, rvalue)

    def visitOpaque(self, opaque, arg, lvalue, rvalue):
        pass

    def visitInterface(self, interface, arg, lvalue, rvalue):
        print('    #error // interface')

    def visitPolymorphic(self, polymorphic, arg, lvalue, rvalue):
        print('    #error')

class RegisterHandleVisitor(stdapi.Visitor):
    def __init__(self):
        self.inLoop = False

    def visitVoid(self, void, lvalue, rvalue):
        pass

    def visitLiteral(self, literal, lvalue, rvalue):
        pass

    def visitString(self, string, lvalue, rvalue):
        pass

    def visitConst(self, const, lvalue, rvalue):
        self.visit(const.type, lvalue, rvalue)

    def visitStruct(self, struct, lvalue, rvalue):
        print('    #error // struct')

    def visitArray(self, array, lvalue, rvalue):
        print('    for (unsigned int i = 0; i < %s.cnt; ++i)' % (lvalue))
        print('    {')
        self.inLoop = True
        self.visit(array.type, lvalue + '[i]', rvalue + '[i]')
        print('    }')
        # TODO LZ:

    def visitBlob(self, blob, lvalue, rvalue):
        pass

    def visitEnum(self, enum, lvalue, rvalue):
        pass

    def visitBitmask(self, bitmask, lvalue, rvalue):
        pass

    def visitPointer(self, pointer, lvalue, rvalue):
        print(r'    DBG_LOG("Not supported yet!!!!!\\n");')

    def visitIntPointer(self, pointer, lvalue, rvalue):
        print('    #error // int pointer')

    def visitObjPointer(self, pointer, lvalue, rvalue):
        print('    #error // obj pointer')

    def visitLinearPointer(self, pointer, lvalue, rvalue):
        print('    #error // linear pointer RegisterHandleVisitor')

    def visitReference(self, reference, lvalue, rvalue):
        print('    #error')

    def visitHandle(self, handle, lvalue, rvalue):
        indent = 8 if self.inLoop else 4
        print('%s%s = %s;' % (' ' * indent, lookupHandleAsL(handle, lvalue), rvalue))
        if handle.name in reverse_lookup_maps:
            print('%s%s = %s;' % (' ' * indent, reverseLookupHandleAsL(handle, rvalue), lvalue))
        # TODO: LZ:

    def visitAlias(self, alias, lvalue, rvalue):
        self.visit(alias.type, lvalue, rvalue)

    def visitOpaque(self, opaque, lvalue, rvalue):
        pass

    def visitInterface(self, interface, lvalue, rvalue):
        print('    #error')

    def visitPolymorphic(self, polymorphic, lvalue, rvalue):
        print('    #error')


class HasHandleVisitor(RegisterHandleVisitor):
    def __init__(self):
        self.has_handle = False

    def visitArray(self, array, lvalue, rvalue):
        self.visit(array.type, lvalue, rvalue)

    def visitHandle(self, handle, lvalue, rvalue):
        self.has_handle = True


class Retracer(object):
    def deserialize(self, func):
        print('    // ------------- deserialize --------------')
        print('    // ------- params & ret definition --------')
        #if func.type is stdapi.Void:
        #    print '    unsigned int retSkip;'
        #else:
        #    print "    %s ret;" % (func.type.mutable())
        for arg in func.args:
            #arg_type = arg.type.mutable()
            #print '    %s %s;' % (arg_type, arg.name)
            arg.has_new_value = False

            if arg.output:
                DeserializeVisitor().visit(arg.type, arg, 'old_' + arg.name, func)
            else:
                DeserializeVisitor().visit(arg.type, arg, arg.name, func)
        print()
        DeserializeVisitor().visit(func.type, None, 'old_ret', func)
        if func.name == 'glAssertBuffer_ARM':
            print('    const char *ptr = (const char *)glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);')
            print('    MD5Digest md5_bound_calc(ptr, size);')
            print('    std::string md5_bound = md5_bound_calc.text();')
            print('    if (md5_bound != md5) {')
            print('        DBG_LOG("glAssertBuffer_ARM: MD5 sums differ!\\n");')
            print('        abort();')
            print('    }')
            print('    glUnmapBuffer(target);')

    def outAllocate(self, func):
        print('    // ----------- out parameters  ------------')
        if func.type is not stdapi.Void:
            print('    %s ret = 0;' % (func.type))
        elif func.name in ['glReadPixels', 'glReadnPixels', 'glReadnPixelsEXT', 'glReadnPixelsKHR']:
            print('    GLvoid* pixels = 0;')
            print()
            print('    // If GLES3 or higher, the glReadPixels operation can')
            print('    // potentionally be meant to a PIXEL_PACK_BUFFER.')
            print('    if (!isPixelPackBufferBound)')
            print('    {')
            print('        size_t size = _gl_image_size(format, type, width, height, 1);')
            print('        pixels = new char[size];')
            print('    }')
            print('    else')
            print('    {')
            print('        // If a pixel pack buffer is bound, old_pixels is')
            print('        // treated as an offset')
            print('        pixels = old_pixels;')
            print('    }')
            print()

        for arg in func.args:
            if arg.output:
                outAllocateVisitor().visit(arg.type, arg, arg.name)

    def assistantParams(self, func):
        for arg in func.args:
            if arg.type is gles12api.GLuniformLocation:
                if 'program' not in func.argNames():
                    print('    // --------- assistant parameters ---------')
                    print('    GLint programNew = gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program;')
                    print('    if (programNew == 0) // might mean that a pipeline object is active (or it is just an error)')
                    print('    {')
                    print('        GLint pipeline = 0;')
                    print('        _glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &pipeline);')
                    print('        if (pipeline != 0)')
                    print('        {')
                    print('            _glGetProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, &programNew);')
                    print('        }')
                    print('    }')

    def lookupHandles(self, func):
        print('    // ----------- look up handles ------------')
        # handle special cases: glBindBuffer, glBindTexture
        # the gl specification is so ugly, which forces us to auto-generate new name
        # if the input doesn't exist
        if func.name in autogenBindFuncs:
            # The object name argument
            arg = func.args[-1]
            print('    Context& context = gRetracer.getCurrentContext();')
            print('    unsigned int %sNew = %s;' % (arg.name, lookupHandleAsR(arg.type, arg.name)))
            print('    if (%sNew == 0 && %s != 0)' % (arg.name, arg.name))
            print('    {')
            print('        %s(1, &%sNew);' % (autogenBindFuncs[func.name], arg.name))
            print('        %s = %sNew;' % (lookupHandleAsL(arg.type, arg.name), arg.name))
            print('        %s = %s;' % (reverseLookupHandleAsL(arg.type, arg.name+'New'), arg.name))
            print('    }')
            arg.has_new_value = True
            return

        if func.name in ['glDeleteBuffers',
                         'glDeleteFramebuffers',
                         'glDeleteRenderbuffers',
                         'glDeleteTextures',
                         'glDeleteTransformFeedbacks',
                         'glDeleteQueries',
                         'glDeleteSamplers',
                         'glDeleteVertexArrays']:
            print('    // hardcode in retrace!')
            return

        has_handle = False

        for arg in func.args:
            hhv = HasHandleVisitor()
            hhv.visit(arg.type, 'old_' + arg.name, arg.name)
            if hhv.has_handle:
                has_handle = True
                break

        if has_handle:
            print('    Context& context = gRetracer.getCurrentContext();')
            # Piggy-back information that we declared a context
            func.has_context = True

        args = [a for a in func.args if not a.output]
        for arg in args:
            LookUpHandleVisitor().visit(arg.type, arg, arg.name + 'New', arg.name)

        if func.name in ['glDeleteProgram', 'glDeleteShader']:
            print('    %s = 0;' % lookupHandleAsL(func.args[0].type, func.args[0].name))

    def registerHandles(self, func):
        print('    // ---------- register handles ------------')
        # the types are really tricky, because GLES API doesn't treat them in a consistent way.
        if func.name == 'glGetUniformLocation':
            print('    if (old_ret == -1)')
            print('        return;')
        elif func.name in ['glMapBuffer', 'glMapBufferRange', 'glMapBufferOES', 'glMapBufferRangeEXT']:
            print('    (void)old_ret;  // Unused variable')
            print()
            print('    Context& context = gRetracer.getCurrentContext();')
            print('    GLuint buffer = getBoundBuffer(target);')
            print('    if (ret == 0) { DBG_LOG("map buffer failed: 0x%04x! binding is 0x%04x, access is 0x%04x, buffer is %u\\n", (unsigned)_glGetError(), (unsigned)target, (unsigned)access, (unsigned)buffer); }')
            print()
            if func.name in ['glMapBuffer', 'glMapBufferOES']:
                print('    if (access == GL_WRITE_ONLY)')
                print('    {')
                print('        context._bufferToData_map[buffer] = ret;')
                print('    }')
            else:
                print('    if (access & GL_MAP_WRITE_BIT)')
                print('    {')
                print('        context._bufferToData_map[buffer] = ret;')
                print('    }')
            print('    else')
            print('    {')
            print('        context._bufferToData_map[buffer] = 0;')
            print('    }')
            if func.name in ['glMapBufferRange']:
                print()
                print('    if (access & GL_MAP_PERSISTENT_BIT_EXT)')
                print('    {')
                print('        DBG_LOG("WARNING! GL_MAP_PERSISTENT_BIT is set to access parameter for glMapBufferRange(). \\n");')
                print('        DBG_LOG("It may cause the retrace to work abnormally.\\n");')
                print('        DBG_LOG("Suggest adding a parameter to /system/lib/egl/tracerparams.cfg to disable the GL_EXT_buffer_storage extension.\\n");')
                print('        DBG_LOG("    echo \\"DisableBufferStorage true\\" >> /system/lib/egl/tracerparams.cfg\\n");')
                print('        DBG_LOG("Then trace again to get a new pat file.\\n");')
                print('    }')
            func.has_context = True
        elif func.name in ['glReadPixels', 'glReadnPixels', 'glReadnPixelsEXT', 'glReadnPixelsKHR']:
            print('    if (!isPixelPackBufferBound)')
            print('    {')
            print('        delete[] static_cast<char*>(pixels);')
            print('    }')
        elif func.name == 'glGenGraphicBuffer_ARM':
            print('    Context& context = gRetracer.getCurrentContext();')
            print('    context.getGraphicBufferMap().LValue(old_ret) = ret;')

        if func.name in ignored_ret_val_functions:
            print('    (void)ret;  // Ignored return value')
            return
        if func.name in get_query_object_funcs:
            print('    if (pname == GL_QUERY_RESULT_AVAILABLE && old_params[0] == 1 && params[0] == 0)')
            print('    {')
            print('        usleep(200);')
            print('        goto loop;')
            print('    }')
            return

        has_handle = False
        if not hasattr(func, 'has_context'):
            output_types = [arg.type for arg in func.args if arg.output]
            if func.type is not stdapi.Void:
                output_types.append(func.type)

            for typ in output_types:
                hhv = HasHandleVisitor()
                hhv.visit(typ, '', '')
                if hhv.has_handle:
                    has_handle = True
                    break

            if has_handle:
                print('    Context& context = gRetracer.getCurrentContext();')

        if func.type is not stdapi.Void:
            RegisterHandleVisitor().visit(func.type, 'old_ret', 'ret')

        for arg in func.args:
            if arg.output:
                RegisterHandleVisitor().visit(arg.type, 'old_' + arg.name, arg.name)

    def invokeFunction(self, func):
        is_draw_array = func.name in stdapi.draw_array_function_names
        is_draw_elements = func.name in stdapi.draw_elements_function_names
        indent = ''

        if func.name == 'glAssertBuffer_ARM':
            return

        print('    // ------------- pre retrace ------------------')
        if func.name == 'glUseProgram':
            print('    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].getContext()->_current_program = programNew;')
        if func.name == 'glViewport':  # record viewport size to get the size of texture bound to FBO
            print('    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.x = x;')
            print('    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.y = y;')
            print('    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.w = width;')
            print('    gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppVP.h = height;')
        if func.name == 'glScissor':
            print('    if (gRetracer.mOptions.mDoOverrideResolution)')
            print('    {')
            print('        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.x = x;')
            print('        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.y = y;')
            print('        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.w = width;')
            print('        gRetracer.mState.mThreadArr[gRetracer.getCurTid()].mCurAppSR.h = height;')
            print('    }')
            print('    else')
            print('    {')
            indent = '    '
        if func.name in ['glCopyImageSubDataEXT', 'glCopyImageSubData', 'glCopyImageSubDataOES']:
            print('    srcName = lookUpPolymorphic(srcName, srcTarget);')
            print('    dstName = lookUpPolymorphic(dstName, dstTarget);')
        if func.name == 'glStateDump_ARM':
            print('    gRetracer.getStateLogger().logState(gRetracer.getCurTid());')
            return
        if func.name in stdapi.all_rendering_names:
            print('    gRetracer.IncCurDrawId();')
        if is_draw_array or is_draw_elements or func.name in stdapi.dispatch_compute_names:
            print('    if (unlikely(stateLoggingEnabled)) {')
            print('        gRetracer.getStateLogger().logFunction(gRetracer.getCurTid(), "' + func.name + '", gRetracer.GetCurCallId(), gRetracer.GetCurDrawId());')
            instance_count = 'instancecount' if func.name in stdapi.draw_instanced_function_names else '0'
        if func.name == 'glDrawElementsIndirect':
            print('        gRetracer.getStateLogger().logDrawElementsIndirect(gRetracer.getCurTid(), type, indirect, 1);')
        elif func.name == 'glDrawArraysIndirect':
            print('        gRetracer.getStateLogger().logDrawArraysIndirect(gRetracer.getCurTid(), indirect, 1);')
        elif func.name == 'glMultiDrawElementsIndirectEXT':
            print('        gRetracer.getStateLogger().logDrawElementsIndirect(gRetracer.getCurTid(), type, indirect, drawcount);')
        elif func.name == 'glMultiDrawArraysIndirectEXT':
            print('        gRetracer.getStateLogger().logDrawArraysIndirect(gRetracer.getCurTid(), indirect, drawcount);')
        elif is_draw_array:
            print('        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), first, count, %s);' % instance_count)
        elif is_draw_elements:
            print('        gRetracer.getStateLogger().logState(gRetracer.getCurTid(), count, type, indices, %s);' % instance_count)
        elif func.name == 'glDispatchCompute':
            print('        gRetracer.getStateLogger().logCompute(gRetracer.getCurTid(), num_groups_x, num_groups_y, num_groups_z);')
        elif func.name == 'glDispatchComputeIndirect':
            print('        gRetracer.getStateLogger().logComputeIndirect(gRetracer.getCurTid(), indirect);')
        if is_draw_array or is_draw_elements or func.name in stdapi.dispatch_compute_names:
            print('    }')
        if is_draw_array or is_draw_elements or func.name == 'glClear':
            print('    pre_glDraw();')

        # sum up the data size of VBO, texture and client-side buffer
        if func.name == 'glBufferData' or func.name == 'glBufferSubData':
            print('#ifndef NDEBUG')
            print('    gRetracer.mVBODataSize += size;')
            print('#endif')
        elif func.name.startswith('glCompressedTex'):
            print('#ifndef NDEBUG')
            print('    gRetracer.mCompressedTextureDataSize += imageSize;')
            print('#endif')
        elif func.name == 'glTexImage2D' or func.name == 'glTexSubImage2D':
            print('#ifndef NDEBUG')
            print('    gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, 1);')
            print('#endif')
        elif func.name == 'glTexImage3DOES' or func.name == 'glTexSubImage3DOES':
            print('#ifndef NDEBUG')
            print('    gRetracer.mTextureDataSize += _gl_image_size(format, type, width, height, depth);')
            print('#endif')
        elif func.name in ['glUnmapBuffer', 'glUnmapBufferOES']:
            print('    GLuint bufferId = getBoundBuffer(target);')
            print('    gRetracer.getCurrentContext()._bufferToData_map.erase(bufferId);')

        if func.name == 'glCreateClientSideBuffer':
            print('    unsigned int name = old_ret;')

        print('    // ------------- retrace ------------------')
        if func.name in ['glTexStorageAttribs2DEXT', 'glTexStorageAttribs3DEXT']:
            print('    if (gRetracer.mOptions.texAfrcRate != -1) {')
            print('        compressionControlInfo compressInfo = {0};')
            print('        if (convertToBPC(target, gRetracer.mOptions.texAfrcRate, internalformat, compressInfo) == true) {')
            print('            //if (gRetracer.mOptions.mDebug > 0)')
            print('                DBG_LOG("Enable %s (target 0x%%04x, internalformat 0x%%04x) fixed rate attrib(0x%%04x, 0x%%04x).\\n", target, internalformat, compressInfo.extension, compressInfo.rate);' % (func.name))
            print('            std::vector<int> new_attrib_list = AddtoAttribList(attrib_list, compressInfo.extension, compressInfo.rate, GL_NONE);')
            if func.name == 'glTexStorageAttribs2DEXT':
                print('            glTexStorageAttribs2DEXT(target, levels, internalformat, width, height, new_attrib_list.data());')
            else:
                print('            glTexStorageAttribs3DEXT(target, levels, internalformat, width, height, depth, new_attrib_list.data());')
            print('        }')
            print('        else {')
            print('            DBG_LOG("%s(target 0x%%04x, internalformat 0x%%04x): No supported fixed rate.\\n", target, internalformat);' % (func.name))
            if func.name == 'glTexStorageAttribs2DEXT':
                print('            glTexStorageAttribs2DEXT(target, levels, internalformat, width, height, attrib_list);')
            else:
                print('            glTexStorageAttribs3DEXT(target, levels, internalformat, width, height, depth, attrib_list);')
            print('        }')
            print('    }')
            print('    else {')
            if func.name == 'glTexStorageAttribs2DEXT':
                print('         glTexStorageAttribs2DEXT(target, levels, internalformat, width, height, attrib_list);')
            else:
                print('         glTexStorageAttribs3DEXT(target, levels, internalformat, width, height, depth, attrib_list);')
            print('    }')
            return
        if func.name in bind_framebuffer_function_names:
            print('    hardcode_glBindFramebuffer(target, framebufferNew);')
            return
        if func.name == 'glDeleteBuffers':
            print('    hardcode_glDeleteBuffers(n, buffers);')
            return
        if func.name == 'glDeleteFramebuffers':
            print('    hardcode_glDeleteFramebuffers(n, framebuffers);')
            return
        if func.name == 'glDeleteRenderbuffers':
            print('    hardcode_glDeleteRenderbuffers(n, renderbuffers);')
            return
        if func.name == 'glDeleteTextures':
            print('    hardcode_glDeleteTextures(n, textures);')
            return
        if func.name == 'glDeleteTransformFeedbacks':
            print('    hardcode_glDeleteTransformFeedbacks(n, ids);')
            return
        if func.name == 'glDeleteQueries':
            print('    hardcode_glDeleteQueries(n, ids);')
            return
        if func.name == 'glDeleteSamplers':
            print('    hardcode_glDeleteSamplers(count, samplers);')
            return
        if func.name == 'glDeleteVertexArrays':
            print('    hardcode_glDeleteVertexArrays(n, arrays);')
            return
        if func.name == 'glCreateClientSideBuffer':
            print('    glCreateClientSideBuffer(name);')
            return
        if func.name == 'glMapBufferOES':
            print('    static bool support_checked = false;')
            print('    static bool supported = false;')
            print('    if (!support_checked) { support_checked = true; supported = isGlesExtensionSupported("GL_OES_mapbuffer"); }')
            print('    if (supported) {')
            print('        ret = glMapBufferOES(target, access);')
            print('        if (ret == 0) DBG_LOG("glMapBufferOES(0x%04x, 0x%04x) failed: 0x%04x\\n", (unsigned)target, (unsigned)access, (unsigned)glGetError());')
            print('    }')
            print('    else')
            print('    {')
            print('        // remap to glMapBufferRange')
            print('        GLint size;')
            print('        unsigned int access_bit = 0;')
            print('        glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);')
            print('        if (access == GL_WRITE_ONLY)')
            print('        {')
            print('            access_bit = GL_MAP_WRITE_BIT;')
            print('        }')
            print('        else if (access == GL_READ_ONLY)')
            print('        {')
            print('            access_bit = GL_MAP_READ_BIT;')
            print('        }')
            print('        else if (access == GL_READ_WRITE)')
            print('        {')
            print('            access_bit = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;')
            print('        }')
            print('        ret = glMapBufferRange(target, 0, size, access_bit);')
            print('        if (ret == 0) DBG_LOG("glMapBufferOES -> glMapBufferRange(0x%04x, 0, %d, 0x%04x) failed: 0x%04x\\n", (unsigned)target, size, access_bit, (unsigned)glGetError());')
            print('    }')
            return
        if func.name in ['glTexStorage2D', 'glTexStorage2DEXT', 'glTexStorage3D', 'glTexStorage3DEXT']:
            print('    if (gRetracer.mOptions.texAfrcRate != -1) {')
            print('        compressionControlInfo compressInfo = {0};')
            print('        if (convertToBPC(target, gRetracer.mOptions.texAfrcRate, internalformat, compressInfo) == true) {')
            print('            GLint const attrib_list[3] = { compressInfo.extension, compressInfo.rate, GL_NONE };')
            print('            //if (gRetracer.mOptions.mDebug > 0)')
            print('                DBG_LOG("Force %s (target 0x%%04x, internalformat 0x%%04x) to fixed rate attrib (0x%%04x, 0x%%04x).\\n", target, internalformat, compressInfo.extension, compressInfo.rate);' % (func.name))
            if func.name == 'glTexStorage2D' or func.name == 'glTexStorage2DEXT':
                print('            glTexStorageAttribs2DEXT(target, levels, internalformat, width, height, attrib_list);')
            else:
                print('            glTexStorageAttribs3DEXT(target, levels, internalformat, width, height, depth, attrib_list);')
            print('        }')
            print('        else {')
            print('            DBG_LOG("%s(target 0x%%04x, internalformat 0x%%04x): No supported fixed rate.\\n", target, internalformat);' % (func.name))
            if func.name == 'glTexStorage2D' or func.name == 'glTexStorage2DEXT':
                print('            %s(target, levels, internalformat, width, height);' % (func.name))
            else:
                print('            %s(target, levels, internalformat, width, height, depth);' % (func.name))
            print('        }')
            print('    }')
            print('    else {')
            indent = '    '
        if func.name in ['glViewport']:
            print('    if (!gRetracer.mOptions.mDoOverrideResolution)')
            print('    {')
            indent = '    '
        if func.name in ['glDiscardFramebufferEXT', 'glInvalidateFramebuffer', 'glInvalidateSubFramebuffer']:
            print('    RetraceOptions& opt = gRetracer.mOptions;')
            print('    if(opt.mForceOffscreen)')
            print('    {')
            print('        for (int i = 0; i < numAttachments; i++)')
            print('        {')
            print('            unsigned int attachment = attachments[i];')
            print('            if (attachment == GL_COLOR)')
            print('                attachments[i] = GL_COLOR_ATTACHMENT0;')
            print('            else if (attachment == GL_DEPTH)')
            print('                attachments[i] = GL_DEPTH_ATTACHMENT;')
            print('            else if (attachment == GL_STENCIL)')
            print('                attachments[i] = GL_STENCIL_ATTACHMENT;')
            print('        }')
            print('    }')
        if func.name in get_query_object_funcs:
            print('loop:')
        if func.name in ['glReadBuffer']:
            print('    if (gRetracer.mOptions.mForceOffscreen)')
            print('    {')
            print('        if (src == GL_BACK || src == GL_FRONT)    src = GL_COLOR_ATTACHMENT0;')
            print('    }')

        if func.name in ['glRenderbufferStorageMultisampleEXT', 'glRenderbufferStorageMultisample', 'glFramebufferTexture2DMultisampleEXT', 'glTexStorage2DMultisample',
                         'glTexStorage3DMultisample']:
            print('if (gRetracer.mOptions.mOverrideMSAA != -1) samples = gRetracer.mOptions.mOverrideMSAA;')

        if func.name == 'glLinkProgram2' or func.name == 'glLinkProgram':
            if func.name == 'glLinkProgram':
                print('    const int status = -1;')
            print('    if (gRetracer.mOptions.mShaderCacheFile.size() > 0 && gRetracer.mOptions.mShaderCacheLoad)')
            print('    {')
            print('        load_from_shadercache(programNew, program, status);')
            print('    }')
            print('    else')
            print('    {')
            print('        _glLinkProgram(programNew);')
            print('        post_glLinkProgram(programNew, program, (int)status);')
            print('    }')
            return

        if func.name in ['glTexParameterf', 'glTexParameteri', 'glSamplerParameterf', 'glSamplerParameteri']:
            print('    if (param > 1 && pname == GL_TEXTURE_MAX_ANISOTROPY_EXT && gRetracer.mOptions.mForceAnisotropicLevel != -1) param = gRetracer.mOptions.mForceAnisotropicLevel;')
        elif func.name in ['glTexParameterfv', 'glTexParameteriv', 'glSamplerParameterfv', 'glSamplerParameteriv']:
            print('    if (*params > 1 && pname == GL_TEXTURE_MAX_ANISOTROPY_EXT && gRetracer.mOptions.mForceAnisotropicLevel != -1) *params = gRetracer.mOptions.mForceAnisotropicLevel;')

        args = [arg.name + "New" if arg.has_new_value else arg.name
                for arg in func.args]
        arg_names = ", ".join(args)

        if func.name in shadercache_funcs:
            print('    if (gRetracer.mOptions.mShaderCacheFile.size() == 0 || !gRetracer.mOptions.mShaderCacheLoad)')
            print('    {')
            print('        {name}({args});'.format(name=func.name, args=arg_names))
            print('    }')
            if func.name == 'glAttachShader':
                print('    gRetracer.getCurrentContext().addShaderID(programNew, shaderNew);')
            if func.name == 'glShaderSource':
                print('    post_glShaderSource(shaderNew, shader, count, string, length);')
        elif func.name == 'glTexStorage2DEXT':
            print('    if (gRetracer.mOptions.mLocalApiVersion >= PROFILE_ES3)')
            print('    {')
            print('    %s%s%s(%s);' % (indent, indent, 'glTexStorage2D', arg_names))
            print('    }')
            print('    else')
            print('    {')
            print('    %s%s%s(%s);' % (indent, indent, func.name, arg_names))
            print('    }')
        elif func.name == 'glTexStorage3DEXT':
            print('    if (gRetracer.mOptions.mLocalApiVersion >= PROFILE_ES3)')
            print('    {')
            print('    %s%s%s(%s);' % (indent, indent, 'glTexStorage3D', arg_names))
            print('    }')
            print('    else')
            print('    {')
            print('    %s%s%s(%s);' % (indent, indent, func.name, arg_names))
            print('    }')
        elif func.name == 'glClientWaitSync':
            print('    %sret = %s(%s);' % (indent, func.name, arg_names))
            indent = '    '
            print('    while ((old_ret == GL_ALREADY_SIGNALED || old_ret == GL_CONDITION_SATISFIED) && ret == GL_TIMEOUT_EXPIRED)')
            print('    {')
            print('    %sif (old_ret == GL_ALREADY_SIGNALED || old_ret == GL_CONDITION_SATISFIED) timeout = UINT64_MAX;' % indent)
            print('    %sret = %s(%s);' % (indent, func.name, arg_names))
            print('    }')
            indent = ''
        elif func.type is not stdapi.Void:
            print('    %sret = %s(%s);' % (indent, func.name, arg_names))
        else:
            print('    %s%s(%s);' % (indent, func.name, arg_names))

        if func.name in ['glViewport', 'glScissor', 'glTexStorage2D', 'glTexStorage2DEXT', 'glTexStorage3D', 'glTexStorage3DEXT']:
            print('    }')

        if func.name in check_ret_funcs:
            print('    if ((unsigned)old_ret != (unsigned)ret) DBG_LOG("%s mismatch old %%u != new %%u\\n", (unsigned)old_ret, (unsigned)ret);' % func.name)
        elif func.name in ['glGetStringi', 'glGetString']:
            print('    (void)ret;')

        if func.name == 'glCompileShader':
            print('    if (gRetracer.mOptions.mShaderCacheFile.size() == 0 || !gRetracer.mOptions.mShaderCacheLoad)')
            print('    {')
            print('        post_glCompileShader(shaderNew, shader);')
            print('    }')

        if func.name == 'glDeleteShader':
            print('    gRetracer.getCurrentContext().deleteShader(shaderNew);')
        if func.name == 'glDeleteProgram':
            print('    gRetracer.getCurrentContext().deleteShaderIDs(programNew);')

    def retraceFunctionBody(self, func):
        #print '    DBG_LOG("retrace %s _src = %%p\\n", _src);' % func.name
        print()
        self.deserialize(func)
        self.assistantParams(func)
        self.lookupHandles(func)
        self.outAllocate(func)
        self.invokeFunction(func)
        self.registerHandles(func)
        print()

    def retraceFunction(self, func):
        print('PUBLIC void retrace_%s(char* _src) {' % func.name)
        if func.name in notSupportedFuncs:
            print('    DBG_LOG("%s is not supported\\n");' % func.name)
        elif func.name in ignored_funcs:
            pass
        else:
            self.retraceFunctionBody(func)
        print('}')
        print()

    def retraceFunctions(self, functions):
        for func in functions:
            if not func.name in broken_funcs:
                self.retraceFunction(func)
        print()

    def callbackArray(self, functions):
        print('const common::EntryMap retracer::gles_callbacks = {')
        for func in functions:
            if func.sideeffects:
                print('    {"%s", std::make_pair((void*)retrace_%s, false)},' % (func.name, func.name))
            elif not func.name in broken_funcs:
                print('    {"%s", std::make_pair((void*)retrace_%s, true)},' % (func.name, func.name))
            else:
                print('    {"%s", std::make_pair((void*)ignore, true)},' % (func.name))
        print('};')
        print()

strings = {
    'header': r"""//This file was generated by retrace.py
#include <retracer/retracer.hpp>
#include <retracer/retrace_api.hpp>
#include <retracer/afrc_enum.hpp>
#include <retracer/forceoffscreen/offscrmgr.h>
#include <common/gl_extension_supported.hpp>
#include <dispatch/eglproc_auto.hpp>
#include <helper/eglsize.hpp>
#include <common/trace_model.hpp>
#include <common/os.hpp>
#include "helper/states.h"
#include "common/file_format.hpp"

#include "json/writer.h"

using namespace common;
using namespace retracer;

// clang also supports gcc pragmas
#pragma GCC diagnostic ignored "-Wunused-variable"
""",
    'convertToBPC': r"""
static bool convertToBPC(int target, int flag, int internalformat, compressionControlInfo &compressInfo)
{
    _glGetError();  // clear error
    _glGetInternalformativ(target, internalformat, GL_NUM_SURFACE_COMPRESSION_FIXED_RATES_EXT, 1, &compressInfo.rate_size);
    if (compressInfo.rate_size == 0) {
        DBG_LOG("!!!!!! WARNING: rate_size is 0. No supported fixed rate for internalformat 0x%04x on target 0x%04x. glGetError 0x%x.\n", internalformat, target, _glGetError());
        return false;
    }

    std::vector<GLint> rates(compressInfo.rate_size);
    _glGetInternalformativ(target, internalformat, GL_SURFACE_COMPRESSION_EXT, compressInfo.rate_size, rates.data());
    compressInfo.rates = rates.data();
    getCompressionInfo(gRetracer.mOptions.texAfrcRate, false, compressInfo);
    return true;
}
    """,
}


def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--file', help='Output file')
    args = parser.parse_args()

    api = gles12api.glesapi

    #############################################################
    ##
    if args.file:
        file_path = args.file
    else:
        script_dir = os.path.dirname(os.path.realpath(__file__))
        file_path = os.path.join(script_dir, 'retrace_gles_auto.cpp')
    with open(file_path, 'w') as f:
        orig_stdout = sys.stdout
        sys.stdout = f

        print(strings['header'])
        print(strings['convertToBPC'])

        retracer = Retracer()
        retracer.retraceFunctions(api.functions)
        retracer.callbackArray(api.functions)
        sys.stdout = orig_stdout

if __name__ == '__main__':
    main()
