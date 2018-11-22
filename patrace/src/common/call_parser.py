import os.path
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import specs.stdapi as stdapi
import specs.gles12api as gles12api
import specs.eglapi as eglapi

notSupportedFuncs=set([
    'eglCreatePixmapSurfaceHI',
])

texture_function_names = set((
    'glCompressedTexImage2D',
    'glCompressedTexImage3D',
    'glCompressedTexImage3DOES',
    'glCompressedTexSubImage2D',
    'glCompressedTexSubImage3D',
    'glCompressedTexSubImage3DOES',
    'glTexImage2D',
    'glTexImage3D',
    'glTexImage3DOES',
    'glTexSubImage2D',
    'glTexSubImage3D',
    'glTexSubImage3DOES',
))

literalToType = {
    'char'                  :'Int8_Type',
    'unsigned char'         :'Uint8_Type',
    'short'                 :'Int16_Type',
    'unsigned short'        :'Uint16_Type',
    'int'                   :'Int_Type',
    'unsigned int'          :'Uint_Type',
    'long long'             :'Int64_Type',
    'unsigned long long'    :'Uint64_Type',
    'intptr_t'              :'Int_Type',
    'uintptr_t'             :'UInt_Type',
    'int8_t'                :'Int8_Type',
    'uint8_t'               :'Uint8_Type',
    'int16_t'               :'Int16_Type',
    'uint16_t'              :'Uint16_Type',
    'int32_t'               :'Int_Type',
    'uint32_t'              :'Uint_Type',
    'int64_t'               :'Int64_Type',
    'uint64_t'              :'Uint64_Type',
    'float'                 :'Float_Type',
    # not care the data now
    'GLvoid *'              :'Unused_Pointer_Type',
    'void *'                :'Unused_Pointer_Type',
}
literalToMember = {
    'char'                  :'mInt8',
    'unsigned char'         :'mUint8',
    'short'                 :'mInt16',
    'unsigned short'        :'mUint16',
    'int'                   :'mInt',
    'unsigned int'          :'mUint',
    'long long'             :'mInt64',
    'unsigned long long'    :'mUint64',
    'intptr_t'              :'mInt',
    'uintptr_t'             :'mUint',
    'int8_t'                :'mInt8',
    'uint8_t'               :'mUint8',
    'int16_t'               :'mInt16',
    'uint16_t'              :'mUint16',
    'int32_t'               :'mInt',
    'uint32_t'              :'mUint',
    'int64_t'               :'mInt64',
    'uint64_t'              :'mUint64',
    'float'                 :'mFloat',
    # not care the data now
    'GLvoid *'              :'mUnusedPointer',
    'void *'                :'mUnusedPointer',
}

class ParseVisitor(stdapi.Visitor):
    def visitVoid(self, void, arg, name, func):
        print '    pValueTM->mType = Void_Type;'
    def visitLiteral(self, literal, arg, name, func):
        print '    %s %s; // literal' % (literal, name)
        print '    _src = ReadFixed<%s>(_src, %s);' % (literal, name)
        print '    pValueTM->mType = %s;' % literalToType[literal.__str__()]
        print '    pValueTM->%s = %s;' % (literalToMember[literal.__str__()], name)
    def visitString(self, string, arg, name, func):
        print '    char* %s; // string' % (name)
        print '    _src = ReadString(_src, %s);' % (name)
        print '    pValueTM->mType = String_Type;'
        print '    if (%s)' % (name)
        print '        pValueTM->mStr = %s;' % name
    def visitConst(self, const, arg, name, func):
        self.visit(const.type, arg, name, func)
    def visitStruct(self, struct, arg, name, func):
        print '    #error'
    def visitArray(self, array, arg, name, func):
        eleSerialType = stdapi.getSerializationType(array.type)
        print '    pValueTM->mType = Array_Type;'
        if stdapi.isString(array.type):
            print '    pValueTM->mEleType = String_Type;'
            print '    Array<const char*> %s; // string array' % (name)
            print '    _src = ReadStringArray(_src, %s);' % (name)
        else:
            print '    pValueTM->mEleType = %s;' % literalToType[eleSerialType.__str__()]
            print '    Array<%s> %s; // array' % (eleSerialType, name)
            print '    _src = Read1DArray(_src, %s);' % (name)
        print '    pValueTM->mArrayLen = %s.cnt;' % name
        print '    if (pValueTM->mArrayLen) {'
        print '        pValueTM->mArray = new ValueTM[pValueTM->mArrayLen];'
        print '        for (unsigned int i = 0; i < pValueTM->mArrayLen; i++) {'
        if stdapi.isString(array.type):
            print '            pValueTM->mArray[i].mType = String_Type;'
            print '            pValueTM->mArray[i].mStr = %s.v[i];' % name
        else:
            print '            pValueTM->mArray[i].mType = %s;' % literalToType[eleSerialType.__str__()]
            print '            pValueTM->mArray[i].%s = %s.v[i];' % (literalToMember[eleSerialType.__str__()], name)
        print '        }'
        print '    } else {'
        print '        pValueTM->mArray = NULL;'
        print '    }'
    def visitBlob(self, blob, arg, name, func):
        print '    Array<char> %s; // blob' % (name)
        print '    _src = Read1DArray(_src, %s);' % (name)
        print '    pValueTM->mType = Blob_Type;'
        print '    pValueTM->mBlobLen = %s.cnt;' % name
        print '    if (pValueTM->mBlobLen) {'
        print '        pValueTM->mBlob = new char[pValueTM->mBlobLen];'
        print '        memcpy(pValueTM->mBlob, %s.v, pValueTM->mBlobLen);' % name
        print '    } else {'
        print '        pValueTM->mBlob = NULL;'
        print '    }'
    def visitEnum(self, enum, arg, name, func):
        print '    int %s; // enum' % (name)
        print '    _src = ReadFixed(_src, %s);' % (name)
        print '    pValueTM->mType = Enum_Type;'
        print '    pValueTM->mEnum = %s;' % name
    def visitBitmask(self, bitmask, arg, name, func):
        self.visit(bitmask.type, arg, name, func)
    def visitPointer(self, pointer, arg, name, func):
        print '    pValueTM->mType = Pointer_Type;'
        ptrSerialType = stdapi.getSerializationType(pointer.type)
        print '    %s %s; // pointer' % (ptrSerialType, name)
        print '    unsigned int isValidPtr;'
        print '    _src = ReadFixed(_src, isValidPtr);'
        print '    if (isValidPtr) {'
        print '        _src = ReadFixed<%s>(_src, %s);'  % (ptrSerialType, name)
        print '        pValueTM->mPointer = new ValueTM;'
        print '        pValueTM->mPointer->mType = %s;' % literalToType[ptrSerialType.__str__()]
        print '        pValueTM->mPointer->%s = %s;' % (literalToMember[ptrSerialType.__str__()], name)
        print '    } else {'
        print '        pValueTM->mPointer = NULL;'
        print '    }'
    def visitIntPointer(self, pointer, arg, name, func):
        print '    int %s; // IntPointer' % name
        print '    _src = ReadFixed(_src, %s);' % name
        print '    pValueTM->mType = Int_Type;'
        print '    pValueTM->mInt = %s;' % name
    def visitObjPointer(self, pointer, arg, name, func):
        print '    #error'
    def visitLinearPointer(self, pointer, arg, name, func):
        print '    unsigned int %s; // linear pointer' % name
        print '    _src = ReadFixed<unsigned int>(_src, %s);' % name
        print '    pValueTM->mType = Int_Type;'
        print '    pValueTM->mInt = %s;' % name
    def visitReference(self, reference, arg, name, func):
        print '    #error'
    def visitHandle(self, handle, arg, name, func):
        self.visit(handle.type, arg, name, func)
        # TODO: LZ:
    def visitAlias(self, alias, arg, name, func):
        self.visit(alias.type, arg, name, func)
    def visitOpaque(self, opaque, arg, name, func):
        if func.name in texture_function_names:
            print '    if (infile.getHeaderVersion() <= HEADER_VERSION_3)'
            print '    {'
            print '        pValueTM->mType = Opaque_Type;'
            print '        pValueTM->mOpaqueType = BlobType;'
            print '        pValueTM->mOpaqueIns = new ValueTM;'
            print '        Array<char> pixels_blob; // blob'
            print '        _src = Read1DArray(_src, pixels_blob);'
            print '        pValueTM->mOpaqueIns->mType = Blob_Type;'
            print '        pValueTM->mOpaqueIns->mBlobLen = pixels_blob.cnt;'
            print '        if (pValueTM->mOpaqueIns->mBlobLen) {'
            print '            pValueTM->mOpaqueIns->mBlob = new char[pValueTM->mOpaqueIns->mBlobLen];'
            print '            memcpy(pValueTM->mOpaqueIns->mBlob, pixels_blob.v, pixels_blob.cnt);'
            print '        } else {'
            print '            pValueTM->mOpaqueIns->mBlob = NULL;'
            print '        }'
            print '    } else {'
            self.visitOpaqueSpecial(opaque, arg, name, func)
            print '    }'
        elif func.name == "glReadPixels":
            print '    if (infile.getHeaderVersion() <= HEADER_VERSION_3)'
            print '    {'
            print '        pValueTM->mType = Opaque_Type;'
            print '        pValueTM->mOpaqueType = NoopType;'
            print '        pValueTM->mOpaqueIns = NULL;'
            print '    }'
            print '    else {'
            self.visitOpaqueSpecial(opaque, arg, name, func)
            print '    }'
        else:
            self.visitOpaqueSpecial(opaque, arg, name, func)

    def visitOpaqueSpecial(self, opaque, arg, name, func):
        print '    pValueTM->mType = Opaque_Type;'
        print '    _src = ReadFixed(_src, pValueTM->mOpaqueType);'
        print '    pValueTM->mOpaqueIns = NULL;'
        print '    if (pValueTM->mOpaqueType == BufferObjectReferenceType) {'
        print '        pValueTM->mOpaqueIns = new ValueTM;'
        print '        unsigned int %s_raw; // raw ptr' % (name)
        print '        _src = ReadFixed<unsigned int>(_src, %s_raw);' % name
        print '        pValueTM->mOpaqueIns->mType = Uint_Type;'
        print '        pValueTM->mOpaqueIns->mUint = %s_raw;' % name
        print '    } else if (pValueTM->mOpaqueType == BlobType) {'
        print '        pValueTM->mOpaqueIns = new ValueTM;'
        print '        ValueTM *argValueTM = pValueTM;'
        print '        pValueTM = argValueTM->mOpaqueIns;'
        self.visit(stdapi.Blob(stdapi.SChar, ''), arg, name+'_blob', func)
        print '        pValueTM = argValueTM;'
        print '    } else if (pValueTM->mOpaqueType == ClientSideBufferObjectReferenceType) {'
        print '        pValueTM->mOpaqueIns = new ValueTM;'
        print '        unsigned int buffer_name, offset;'
        print '        _src = ReadFixed<unsigned int>(_src, buffer_name);'
        print '        _src = ReadFixed<unsigned int>(_src, offset);'
        print '        pValueTM->mOpaqueIns->mType = MemRef_Type;'
        print '        pValueTM->mOpaqueIns->mClientSideBufferName = buffer_name;'
        print '        pValueTM->mOpaqueIns->mClientSideBufferOffset = offset;'
        print '    }'
    def visitInterface(self, interface, arg, name, func):
        print '    #error'
    def visitPolymorphic(self, polymorphic, arg, name, func):
        print '    #error'

class CallParser(object):
    def parseParamRet(self, func):
        print '    ValueTM *pValueTM = NULL;'
        print
        for arg in func.args:
            print '    // %s' % arg.name
            print '    {'
            print '    pValueTM = new ValueTM;'
            print '    pValueTM->mName = "%s";' % arg.name
            ParseVisitor().visit(arg.type, arg, arg.name, func)
            print '    callTM.mArgs.push_back(pValueTM);'
            print '    }'
            print
        print
        print '    pValueTM = &(callTM.mRet);'
        ParseVisitor().visit(func.type, None, 'ret', func)

    def parseFunctionBody(self, func):
        self.parseParamRet(func)
        print

    def parseFunction(self, func):
        if func.name in notSupportedFuncs:
            return
        print 'static void parse_%s(char* _src, CallTM& callTM, const InFileBase &infile) {' % func.name
        print '    callTM.mCallId = %d;' % func.id
        print '    callTM.mCallName = "%s";' % func.name
        print
        self.parseFunctionBody(func)
        print '}'
        print

    def parseFunctions(self, functions):
        for func in functions:
            self.parseFunction(func)
        print

    def callbackArray(self, functions):
        print 'const common::EntryMap parse_callbacks = {'
        for func in functions:
            if func.name not in notSupportedFuncs:
                print '    {"%s", (void*)parse_%s},' % (func.name, func.name)
        print '};'
        print

if __name__ == '__main__':
    api = gles12api.glesapi
    api.addApi(eglapi.eglapi)
    callParser = CallParser()

    script_dir = os.path.dirname(os.path.realpath(__file__))

    #############################################################
    ##
    file_path = os.path.join(script_dir, 'call_parser.cpp')
    with open(file_path, 'w') as f:
        orig_stdout = sys.stdout
        sys.stdout = f
        print '//Generated by %s' % sys.argv[0]
        print '#include <stdint.h>'
        print '#include <common/parse_api.hpp>'
        print '#include <common/file_format.hpp>'
        print '#include <common/trace_model.hpp>'
        print '#include <dispatch/eglimports.hpp>'
        print
        print 'namespace common {'
        print
        callParser.parseFunctions(api.functions)
        callParser.callbackArray(api.functions)
        print
        print '}'
        sys.stdout = orig_stdout
