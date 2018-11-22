##########################################################################
#
# Copyright 2008-2010 VMware, Inc.
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

"""C basic types"""


import debug

draw_array_function_names = set([
    "glDrawArrays",
    "glDrawArraysEXT",
    "glDrawArraysIndirect",
    "glDrawArraysInstanced",
    "glDrawArraysInstancedARB",
    "glDrawArraysInstancedEXT",
    "glDrawArraysInstancedBaseInstance",
    "glDrawMeshArraysSUN",
    "glMultiDrawArrays",
    "glMultiDrawArraysEXT",
    "glMultiModeDrawArraysIBM",
])

draw_indirect_function_names = set((
    "glDrawArraysIndirect",
    "glDrawElementsIndirect",
))

draw_instanced_function_names = set((
    "glDrawArraysInstanced",
    "glDrawElementsInstanced",
    "glDrawElementsInstancedARB",
    "glDrawElementsInstancedEXT",
    "glDrawElementsInstancedBaseVertex",
    "glDrawElementsInstancedBaseInstance",
    "glDrawElementsInstancedBaseVertexBaseInstance",
))

draw_elements_function_names = set([
    "glDrawElements",
    "glDrawElementsBaseVertex",
    "glDrawElementsIndirect",
    "glDrawElementsInstanced",
    "glDrawElementsInstancedARB",
    "glDrawElementsInstancedEXT",
    "glDrawElementsInstancedBaseVertex",
    "glDrawElementsInstancedBaseInstance",
    "glDrawElementsInstancedBaseVertexBaseInstance",
    "glDrawRangeElements",
    "glDrawRangeElementsEXT",
    "glDrawRangeElementsBaseVertex",
    "glMultiDrawElements",
    "glMultiDrawElementsBaseVertex",
    "glMultiDrawElementsEXT",
    "glMultiModeDrawElementsIBM",
])

draw_function_names = draw_array_function_names | draw_instanced_function_names | draw_elements_function_names

dispatch_compute_names = set((
    'glDispatchCompute',
    'glDispatchComputeIndirect',
))

other_rendering_names = set((
    'glClear',
    'glBlitFramebuffer',
    'glClearBufferiv',
    'glClearBufferuiv',
    'glClearBufferfv',
    'glClearBufferfi'
))

all_rendering_names = draw_function_names | dispatch_compute_names | other_rendering_names

compressed_texture_function_names = set((
    'glCompressedTexImage2D',
    'glCompressedTexImage3D',
    'glCompressedTexImage3DOES',
))

compressed_sub_texture_function_names = set((
    'glCompressedTexSubImage2D',
    'glCompressedTexSubImage3D',
    'glCompressedTexSubImage3DOES',
))

texture_function_names = compressed_sub_texture_function_names | compressed_texture_function_names | set((
    'glTexImage2D',
    'glTexImage3D',
    'glTexImage3DOES',
    'glTexSubImage2D',
    'glTexSubImage3D',
    'glTexSubImage3DOES',
))

class Type:
    """Base class for all types."""

    __tags = set()

    def __init__(self, expr, tag = None):
        self.expr = expr

        # Generate a default tag, used when naming functions that will operate
        # on this type, so it should preferrably be something representative of
        # the type.
        if tag is None:
            if expr is not None:
                tag = ''.join([c for c in expr if c.isalnum() or c in '_'])
            else:
                tag = 'anonynoums'
        else:
            for c in tag:
                assert c.isalnum() or c in '_'

        # Ensure it is unique.
        if tag in Type.__tags:
            suffix = 1
            while tag + str(suffix) in Type.__tags:
                suffix += 1
            tag += str(suffix)

        assert tag not in Type.__tags
        Type.__tags.add(tag)

        self.tag = tag

    def __str__(self):
        """Return the C/C++ type expression for this type."""
        return self.expr

    def visit(self, visitor, *args, **kwargs):
        raise NotImplementedError

    def mutable(self):
        '''Return a mutable version of this type.

        Convenience wrapper around MutableRebuilder.'''
        visitor = MutableRebuilder()
        return visitor.visit(self)


class _Void(Type):
    """Singleton void type."""

    def __init__(self):
        Type.__init__(self, "void")

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitVoid(self, *args, **kwargs)

Void = _Void()


class Literal(Type):
    """Class to describe literal types.

    Types which are not defined in terms of other types, such as integers and
    floats."""

    def __init__(self, expr, kind):
        Type.__init__(self, expr)
        self.kind = kind

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitLiteral(self, *args, **kwargs)


Bool = Literal("bool", "Bool")
SChar = Literal("signed char", "SInt")
UChar = Literal("unsigned char", "UInt")
Short = Literal("short", "SInt")
Int = Literal("int", "SInt")
Long = Literal("long", "SInt")
LongLong = Literal("long long", "SInt")
UShort = Literal("unsigned short", "UInt")
UInt = Literal("unsigned int", "UInt")
ULong = Literal("unsigned long", "UInt")
ULongLong = Literal("unsigned long long", "UInt")
Float = Literal("float", "Float")
Double = Literal("double", "Double")
SizeT = Literal("size_t", "UInt")

Char = Literal("char", "SInt")
WChar = Literal("wchar_t", "SInt")

Int8 = Literal("int8_t", "SInt")
UInt8 = Literal("uint8_t", "UInt")
Int16 = Literal("int16_t", "SInt")
UInt16 = Literal("uint16_t", "UInt")
Int32 = Literal("int32_t", "SInt")
UInt32 = Literal("uint32_t", "UInt")
Int64 = Literal("int64_t", "SInt")
UInt64 = Literal("uint64_t", "UInt")

IntPtr = Literal("intptr_t", "SInt")
UIntPtr = Literal("uintptr_t", "UInt")

class Const(Type):

    def __init__(self, type):
        # While "const foo" and "foo const" are synonymous, "const foo *" and
        # "foo * const" are not quite the same, and some compilers do enforce
        # strict const correctness.
        if isinstance(type, String) or type is WString:
            # For strings we never intend to say a const pointer to chars, but
            # rather a point to const chars.
            expr = "const " + type.expr
        elif isinstance(type, Array) and type.expr.endswith("*"):
            expr = type.expr[0:-1] + "const *"
        elif type.expr.startswith("const ") or '*' in type.expr:
            expr = type.expr + " const"
        else:
            # The most legible
            expr = "const " + type.expr

        Type.__init__(self, expr, 'C' + type.tag)

        self.type = type

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitConst(self, *args, **kwargs)


class Pointer(Type):

    def __init__(self, type):
        Type.__init__(self, type.expr + " *", 'P' + type.tag)
        self.type = type

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitPointer(self, *args, **kwargs)


class IntPointer(Type):
    '''Integer encoded as a pointer.'''

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitIntPointer(self, *args, **kwargs)


class ObjPointer(Type):
    '''Pointer to an object.'''

    def __init__(self, type):
        Type.__init__(self, type.expr + " *", 'P' + type.tag)
        self.type = type

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitObjPointer(self, *args, **kwargs)


class LinearPointer(Type):
    '''Pointer to a linear range of memory.'''

    def __init__(self, type, size = None):
        Type.__init__(self, type.expr + " *", 'P' + type.tag)
        self.type = type
        self.size = size

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitLinearPointer(self, *args, **kwargs)


class Reference(Type):
    '''C++ references.'''

    def __init__(self, type):
        Type.__init__(self, type.expr + " &", 'R' + type.tag)
        self.type = type

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitReference(self, *args, **kwargs)


class Handle(Type):

    def __init__(self, name, type, range=None, key=None):
        Type.__init__(self, type.expr, 'P' + type.tag)
        self.name = name
        self.type = type
        self.range = range
        self.key = key

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitHandle(self, *args, **kwargs)


def ConstPointer(type):
    return Pointer(Const(type))


class Enum(Type):

    __id = 0

    def __init__(self, name, values):
        Type.__init__(self, name)

        self.id = Enum.__id
        Enum.__id += 1

        self.values = list(values)

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitEnum(self, *args, **kwargs)


def FakeEnum(type, values):
    return Enum(type.expr, values)


class Bitmask(Type):

    __id = 0

    def __init__(self, type, values):
        Type.__init__(self, type.expr)

        self.id = Bitmask.__id
        Bitmask.__id += 1

        self.type = type
        self.values = values

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitBitmask(self, *args, **kwargs)

Flags = Bitmask


class Array(Type):

    def __init__(self, type, length):
        Type.__init__(self, type.expr + " *")
        self.type = type
        self.length = length

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitArray(self, *args, **kwargs)

    # todo: print self
class Blob(Type):

    def __init__(self, type, size):
        Type.__init__(self, type.expr + ' *')
        self.type = type
        self.size = size

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitBlob(self, *args, **kwargs)


class Struct(Type):

    __id = 0

    def __init__(self, name, members):
        Type.__init__(self, name)

        self.id = Struct.__id
        Struct.__id += 1

        self.name = name
        self.members = []

        # Eliminate anonymous unions
        for type, name in members:
            if name is not None:
                self.members.append((type, name))
            else:
                assert isinstance(type, Union)
                assert type.name is None
                self.members.extend(type.members)

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitStruct(self, *args, **kwargs)


class Union(Type):

    __id = 0

    def __init__(self, name, members):
        Type.__init__(self, name)

        self.id = Union.__id
        Union.__id += 1

        self.name = name
        self.members = members


class Alias(Type):

    def __init__(self, expr, type, needs_casting=False):
        Type.__init__(self, expr)
        self.type = type
        self.needs_casting = needs_casting

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitAlias(self, *args, **kwargs)

class Arg:

    def __init__(self, type, name, input=True, output=False):
        self.type = type
        self.name = name
        self.input = input
        self.output = output
        self.index = None

    def __str__(self):
        return '%s %s' % (self.type, self.name)


def In(type, name):
    return Arg(type, name, input=True, output=False)

def Out(type, name):
    return Arg(type, name, input=False, output=True)

def InOut(type, name):
    return Arg(type, name, input=True, output=True)


class Function:

    # 0-3 are reserved to memcpy, malloc, free, and realloc
    __id = 4

    def __init__(self, type, name, args, call = '', fail = None, sideeffects=True):
        self.id = Function.__id
        Function.__id += 1

        self.type = type
        self.name = name

        self.args = []
        index = 0
        for arg in args:
            if not isinstance(arg, Arg):
                if isinstance(arg, tuple):
                    arg_type, arg_name = arg
                else:
                    arg_type = arg
                    arg_name = "arg%u" % index
                arg = Arg(arg_type, arg_name)
            arg.index = index
            index += 1
            self.args.append(arg)

        self.call = 'GLES_CALLCONVENTION'
        self.fail = fail
        self.sideeffects = sideeffects

    def prototype(self, name=None):
        if name is not None:
            name = name.strip()
        else:
            name = self.name
        s = name
        if self.call:
            s = self.call + ' ' + s
        if name.startswith('*'):
            s = '(' + s + ')'
        s = self.type.expr + ' ' + s
        s += "("
        if self.args:
            s += ", ".join(["%s %s" % (arg.type, arg.name) for arg in self.args])
        else:
            s += "void"
        s += ")"
        return s

    def prototypeExecutableTrace(self, name=None):
        if name is not None:
            name = name.strip()
        else:
            name = self.name
        s = name

        #if self.call:
        #    s = self.call + ' ' + s

        if name.startswith('*'):
            s = '(' + s + ')'


        # instead of outputting return type
        #s = self.type.expr + ' ' + s
        s = 'void ' + s # just use void, since we never use return values in retrace

        s += "("
        # function params:


        # functions that have Opaque Arguments
        # would it be cleaner to create a new gles12api.py for exe trace and encode info in argument type instead?
        opaque_functions = ['glDrawElements', 'glVertexAttribPointer',
            'glVertexPointer', 'glColorPointer', 'glNormalPointer', 'glTexCoordPointer',
            'glMatrixIndexPointerOES', 'glWeightPointerOES', 'glPointSizePointerOES',
            'glGetVertexAttribPointerv', 'glReadPixels', 'glGetPointerv', 'glGetPointervKHR', 'glGetBufferPointervOES'
        ]

        # remove last argument. An earlier solution I had was to change the type of an argument from
        # GLvoid* to int, and changing the name from [pointer, indices, pixels] to _opaque_type, but doing that
        # breaks gen_handle_mapping_auto_cpp.py DeserializeVisitor opaque function
        modifiedArgs = self.args
        if self.name in opaque_functions:
            modifiedArgs = self.args[0:-1]
        else:
            modifiedArgs = self.args

        #for arg in modifiedArgs:
        #    if arg.output:
        #        arg.name = "old_" + arg.name

        params = ""
        if modifiedArgs:
            params += ", ".join(["%s %s" % (arg.type, arg.name) for arg in modifiedArgs])
        else:
            params += "void"

        pass_old_ret_list = ['glCreateProgram', 'glCreateShader', 'glGetAttribLocation', 'glGetUniformLocation', 'glCreateShaderProgramvEXT']

        # could use self.name with exact match
        if any([name.endswith(call_name) for call_name in pass_old_ret_list]):
            if params == "void":
                params = self.type.expr + " old_ret"
            else:
                params = self.type.expr + " old_ret, " + params

        s += params

        # add extra arguments
        if self.name in opaque_functions:
            s += ', int _opaque_type, common::OpaqueArg params'

        s += ")"
        return s

    def argNames(self):
        return [arg.name for arg in self.args]


def StdFunction(*args, **kwargs):
    kwargs.setdefault('call', '__stdcall')
    return Function(*args, **kwargs)


def FunctionPointer(type, name, args, **kwargs):
    # XXX: We should probably treat function pointers (callbacks or not) in a generic fashion
    return Opaque(name)


class Interface(Type):

    def __init__(self, name, base=None):
        Type.__init__(self, name)
        self.name = name
        self.base = base
        self.methods = []

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitInterface(self, *args, **kwargs)

    def getMethodByName(self, name):
        for methods in self.methods:
            if methods.name == name:
                return methods
        return None

    def iterMethods(self):
        if self.base is not None:
            for method in self.base.iterMethods():
                yield method
        for method in self.methods:
            yield method
        raise StopIteration

    def iterBases(self):
        iface = self
        while iface is not None:
            yield iface
            iface = iface.base
        raise StopIteration

    def iterBaseMethods(self):
        if self.base is not None:
            for iface, method in self.base.iterBaseMethods():
                yield iface, method
        for method in self.methods:
            yield self, method
        raise StopIteration


class Method(Function):

    def __init__(self, type, name, args, call = '__stdcall', const=False, sideeffects=True):
        Function.__init__(self, type, name, args, call = call, sideeffects=sideeffects)
        for index in range(len(self.args)):
            self.args[index].index = index + 1
        self.const = const

    def prototype(self, name=None):
        s = Function.prototype(self, name)
        if self.const:
            s += ' const'
        return s


def StdMethod(*args, **kwargs):
    kwargs.setdefault('call', '__stdcall')
    return Method(*args, **kwargs)


class String(Type):

    def __init__(self, type = Char, length = None, wide = False):
        assert isinstance(type, Type)
        Type.__init__(self, type.expr + ' *')
        self.type = type
        self.length = length
        self.wide = wide

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitString(self, *args, **kwargs)


class Opaque(Type):
    '''Opaque pointer.'''

    def __init__(self, expr, size=''):
        Type.__init__(self, expr)
        self.size = size

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitOpaque(self, *args, **kwargs)


def OpaquePointer(type, size='', *args):
    return Opaque(type.expr + ' *', size)

def OpaqueArray(type, size):
    return Opaque(type.expr + ' *')

def OpaqueBlob(type, size):
    return Opaque(type.expr + ' *')


class Polymorphic(Type):

    def __init__(self, switchExpr, switchTypes, defaultType, contextLess=True):
        Type.__init__(self, defaultType.expr)
        self.switchExpr = switchExpr
        self.switchTypes = switchTypes
        self.defaultType = defaultType
        self.contextLess = contextLess

    def visit(self, visitor, *args, **kwargs):
        return visitor.visitPolymorphic(self, *args, **kwargs)

    def iterSwitch(self):
        cases = [['default']]
        types = [self.defaultType]

        for expr, type in self.switchTypes:
            case = 'case %s' % expr
            try:
                i = types.index(type)
            except ValueError:
                cases.append([case])
                types.append(type)
            else:
                cases[i].append(case)

        return zip(cases, types)


def EnumPolymorphic(enumName, switchExpr, switchTypes, defaultType, contextLess=True):
    enumValues = [expr for expr, type in switchTypes]
    enum = Enum(enumName, enumValues)
    polymorphic = Polymorphic(switchExpr, switchTypes, defaultType, contextLess)
    return enum, polymorphic


class Visitor:
    '''Abstract visitor for the type hierarchy.'''

    def visit(self, type, *args, **kwargs):
        return type.visit(self, *args, **kwargs)

    def visitVoid(self, void, *args, **kwargs):
        raise NotImplementedError

    def visitLiteral(self, literal, *args, **kwargs):
        raise NotImplementedError

    def visitString(self, string, *args, **kwargs):
        raise NotImplementedError

    def visitConst(self, const, *args, **kwargs):
        raise NotImplementedError

    def visitStruct(self, struct, *args, **kwargs):
        raise NotImplementedError

    def visitArray(self, array, *args, **kwargs):
        raise NotImplementedError

    def visitBlob(self, blob, *args, **kwargs):
        raise NotImplementedError

    def visitEnum(self, enum, *args, **kwargs):
        raise NotImplementedError

    def visitBitmask(self, bitmask, *args, **kwargs):
        raise NotImplementedError

    def visitPointer(self, pointer, *args, **kwargs):
        raise NotImplementedError

    def visitIntPointer(self, pointer, *args, **kwargs):
        raise NotImplementedError

    def visitObjPointer(self, pointer, *args, **kwargs):
        raise NotImplementedError

    def visitLinearPointer(self, pointer, *args, **kwargs):
        raise NotImplementedError

    def visitReference(self, reference, *args, **kwargs):
        raise NotImplementedError

    def visitHandle(self, handle, *args, **kwargs):
        raise NotImplementedError

    def visitAlias(self, alias, *args, **kwargs):
        raise NotImplementedError

    def visitOpaque(self, opaque, *args, **kwargs):
        raise NotImplementedError

    def visitInterface(self, interface, *args, **kwargs):
        raise NotImplementedError

    def visitPolymorphic(self, polymorphic, *args, **kwargs):
        raise NotImplementedError
        #return self.visit(polymorphic.defaultType, *args, **kwargs)


class OnceVisitor(Visitor):
    '''Visitor that guarantees that each type is visited only once.'''

    def __init__(self):
        self.__visited = set()

    def visit(self, type, *args, **kwargs):
        if type not in self.__visited:
            self.__visited.add(type)
            return type.visit(self, *args, **kwargs)
        return None


class Rebuilder(Visitor):
    '''Visitor which rebuild types as it visits them.

    By itself it is a no-op -- it is intended to be overwritten.
    '''

    def visitVoid(self, void):
        return void

    def visitLiteral(self, literal):
        return literal

    def visitString(self, string):
        return string

    def visitConst(self, const):
        const_type = self.visit(const.type)
        if const_type is const.type:
            return const
        else:
            return Const(const_type)

    def visitStruct(self, struct):
        members = [(self.visit(type), name) for type, name in struct.members]
        return Struct(struct.name, members)

    def visitArray(self, array):
        type = self.visit(array.type)
        return Array(type, array.length)

    def visitBlob(self, blob):
        type = self.visit(blob.type)
        return Blob(type, blob.size)

    def visitEnum(self, enum):
        return enum

    def visitBitmask(self, bitmask):
        type = self.visit(bitmask.type)
        return Bitmask(type, bitmask.values)

    def visitPointer(self, pointer):
        pointer_type = self.visit(pointer.type)
        if pointer_type is pointer.type:
            return pointer
        else:
            return Pointer(pointer_type)

    def visitIntPointer(self, pointer):
        return pointer

    def visitObjPointer(self, pointer):
        pointer_type = self.visit(pointer.type)
        if pointer_type is pointer.type:
            return pointer
        else:
            return ObjPointer(pointer_type)

    def visitLinearPointer(self, pointer):
        pointer_type = self.visit(pointer.type)
        if pointer_type is pointer.type:
            return pointer
        else:
            return LinearPointer(pointer_type)

    def visitReference(self, reference):
        reference_type = self.visit(reference.type)
        if reference_type is reference.type:
            return reference
        else:
            return Reference(reference_type)

    def visitHandle(self, handle):
        handle_type = self.visit(handle.type)
        if handle_type is handle.type:
            return handle
        else:
            return Handle(handle.name, handle_type, range=handle.range, key=handle.key)

    def visitAlias(self, alias):
        alias_type = self.visit(alias.type)
        if alias_type is alias.type:
            return alias
        else:
            return Alias(alias.expr, alias_type)

    def visitOpaque(self, opaque):
        return opaque

    def visitInterface(self, interface, *args, **kwargs):
        return interface

    def visitPolymorphic(self, polymorphic):
        switchExpr = polymorphic.switchExpr
        switchTypes = [(expr, self.visit(type)) for expr, type in polymorphic.switchTypes]
        defaultType = self.visit(polymorphic.defaultType)
        return Polymorphic(switchExpr, switchTypes, defaultType, polymorphic.contextLess)


class MutableRebuilder(Rebuilder):
    '''Type visitor which derives a mutable type.'''

    def visitConst(self, const):
        # Strip out const qualifier
        return const.type

    def visitAlias(self, alias):
        # Tear the alias on type changes
        type = self.visit(alias.type)
        if type is alias.type:
            return alias
        return type

    def visitReference(self, reference):
        # Strip out references
        return reference.type


class Traverser(Visitor):
    '''Visitor which all types.'''

    def visitVoid(self, void, *args, **kwargs):
        pass

    def visitLiteral(self, literal, *args, **kwargs):
        pass

    def visitString(self, string, *args, **kwargs):
        pass

    def visitConst(self, const, *args, **kwargs):
        self.visit(const.type, *args, **kwargs)

    def visitStruct(self, struct, *args, **kwargs):
        for type, name in struct.members:
            self.visit(type, *args, **kwargs)

    def visitArray(self, array, *args, **kwargs):
        self.visit(array.type, *args, **kwargs)

    def visitBlob(self, array, *args, **kwargs):
        pass

    def visitEnum(self, enum, *args, **kwargs):
        pass

    def visitBitmask(self, bitmask, *args, **kwargs):
        self.visit(bitmask.type, *args, **kwargs)

    def visitPointer(self, pointer, *args, **kwargs):
        self.visit(pointer.type, *args, **kwargs)

    def visitIntPointer(self, pointer, *args, **kwargs):
        pass

    def visitObjPointer(self, pointer, *args, **kwargs):
        self.visit(pointer.type, *args, **kwargs)

    def visitLinearPointer(self, pointer, *args, **kwargs):
        self.visit(pointer.type, *args, **kwargs)

    def visitReference(self, reference, *args, **kwargs):
        self.visit(reference.type, *args, **kwargs)

    def visitHandle(self, handle, *args, **kwargs):
        self.visit(handle.type, *args, **kwargs)

    def visitAlias(self, alias, *args, **kwargs):
        self.visit(alias.type, *args, **kwargs)

    def visitOpaque(self, opaque, *args, **kwargs):
        pass

    def visitInterface(self, interface, *args, **kwargs):
        if interface.base is not None:
            self.visit(interface.base, *args, **kwargs)
        for method in interface.iterMethods():
            for arg in method.args:
                self.visit(arg.type, *args, **kwargs)
            self.visit(method.type, *args, **kwargs)

    def visitPolymorphic(self, polymorphic, *args, **kwargs):
        self.visit(polymorphic.defaultType, *args, **kwargs)
        for expr, type in polymorphic.switchTypes:
            self.visit(type, *args, **kwargs)


class Collector(Traverser):
    '''Visitor which collects all unique types as it traverses them.'''

    def __init__(self):
        self.__visited = set()
        self.types = []

    def visit(self, type):
        if type in self.__visited:
            return
        self.__visited.add(type)
        Visitor.visit(self, type)
        self.types.append(type)



class API:
    '''API abstraction.

    Essentially, a collection of types, functions, and interfaces.
    '''

    def __init__(self, name = None):
        self.name = name
        self.headers = []
        self.functions = []
        self.interfaces = []

    def getAllTypes(self):
        collector = Collector()
        for function in self.functions:
            for arg in function.args:
                collector.visit(arg.type)
            collector.visit(function.type)
        for interface in self.interfaces:
            collector.visit(interface)
            for method in interface.iterMethods():
                for arg in method.args:
                    collector.visit(arg.type)
                collector.visit(method.type)
        return collector.types

    def getAllInterfaces(self):
        types = self.getAllTypes()
        interfaces = [type for type in types if isinstance(type, Interface)]
        for interface in self.interfaces:
            if interface not in interfaces:
                interfaces.append(interface)
        return interfaces

    def addFunction(self, function):
        self.functions.append(function)

    def addFunctions(self, functions):
        for function in functions:
            self.addFunction(function)

    def addInterface(self, interface):
        self.interfaces.append(interface)

    def addInterfaces(self, interfaces):
        self.interfaces.extend(interfaces)

    def addApi(self, api):
        self.headers.extend(api.headers)
        self.addFunctions(api.functions)
        self.addInterfaces(api.interfaces)

    def getFunctionByName(self, name):
        for function in self.functions:
            if function.name == name:
                return function
        return None

    def delFunctionByName(self, name):
        for function in self.functions:
            if function.name == name:
                self.functions.remove(function)
                return

def getSerializationType(ty):
    if isinstance(ty, Const) or isinstance(ty, Alias) or isinstance(ty, Handle):
        return getSerializationType(ty.type)
    elif isinstance(ty, Enum):
        # we would like to serialize enumerator into 32 bits integer.
        return UInt
    elif isinstance(ty, IntPointer):
        return Int
    else:
        return ty

def isEnum(ty):
    if isinstance(ty, Const) or isinstance(ty, Alias) or isinstance(ty, Handle) or isinstance(ty, Array):
        return isEnum(ty.type)
    elif isinstance(ty, Enum):
        return True
    else:
        return False

def isString(ty):
    if isinstance(getSerializationType(ty), String):
        return True
    else:
        return False

# C string (i.e., zero terminated)
CString = String(Char)
WString = String(WChar, wide=True)
ConstCString = String(Const(Char))
ConstWString = String(Const(WChar), wide=True)
