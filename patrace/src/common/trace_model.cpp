#include <common/trace_model.hpp>

#include <common/parse_api.hpp>
#include <common/api_info.hpp>
#include <common/file_format.hpp>

#include <eglstate/common.hpp>

#include <GLES3/gl32.h>
#include <cmath>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <float.h>
#endif

using namespace std;

namespace
{

const char *DEFAULT_LOAD_FILTER_STRING = "gl";

void replaceString(std::string& haystack, const std::string& needle, const std::string& replacement)
{
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos)
    {
        haystack.replace(pos, needle.length(), replacement);
        pos += replacement.length();
    }
}

bool IsQueryCall(const std::string &callName)
{
    // Firstly check the exceptions of glGet
    if (callName == "glGetAttribLocation" ||
        callName == "glGetUniformLocation")
    {
        return false;
    }

    if (callName.find("eglGet") != std::string::npos
        || callName.find("eglQuery") != std::string::npos
        || callName.find("eglWait") != std::string::npos
        || callName.find("glGet") != std::string::npos
        || callName.find("glIs") != std::string::npos)
    {
        return true;
    }
    return false;
}

bool MatchFilterString(const std::string &callName, const std::string &filterStr)
{
    // this function is called for each line/call in an open trace frame
    return (callName.find(filterStr) != std::string::npos);
}

}

namespace common {

unsigned int gValueTypeSize[] = {
    0,
    0,
    1, // Int8
    1, // Unit8
    2, // Int16
    2, // Uint16
    4, // Int
    4, // Enum
    4, // Uint
    8, // Int64
    8, // Uint64
    4, // Float
    0, // String
    0, // Array
    0, // Blob
};

ValueTM::~ValueTM()
{
    Reset();
}

ValueTM::ValueTM(const ValueTM &other) : mName(NULL)
{
    CopyFrom(other);
}

ValueTM & ValueTM::operator =(const ValueTM &other)
{
    if (&other == this)
        return *this;

    Reset();
    CopyFrom(other);
    return *this;
}

void ValueTM::CopyFrom(const ValueTM &other)
{
    mType = other.mType;
    if (other.mType == Blob_Type)
    {
        mBlobLen = other.mBlobLen;
        mBlob = new char[mBlobLen];
        memcpy(mBlob, other.mBlob, mBlobLen);
    }
    else if (other.mType == Array_Type)
    {
        mArrayLen = other.mArrayLen;
        mEleType = other.mEleType;
        mArray = new ValueTM[mArrayLen];
        for (unsigned int i = 0; i < mArrayLen; ++i)
            mArray[i] = other.mArray[i];
    }
    else if (other.mType == Opaque_Type)
    {
        mOpaqueType = other.mOpaqueType;
        mOpaqueIns = new ValueTM;
        *mOpaqueIns = *(other.mOpaqueIns);
    }
    else if (other.mType == Pointer_Type)
    {
        mPointer = new ValueTM;
        *mPointer = *(other.mPointer);
    }
    else if (other.mType == Unused_Pointer_Type)
    {
        mUnusedPointer = other.mUnusedPointer;
    }
    else if (other.mType == String_Type)
    {
        mStr = other.mStr;
    }
    else
    {
        // copy the maximum memory
        mDouble = other.mDouble;
    }
}

void ValueTM::Reset()
{
    switch (mType) {
    case Blob_Type:
        delete [] mBlob;
        mBlob = NULL;
        mBlobLen = 0;
        break;
    case Array_Type:
        delete [] mArray;
        mArray = NULL;
        mArrayLen = 0;
        mEleType = Void_Type;
        break;
    case Pointer_Type:
        delete mPointer;
        mPointer = NULL;
        break;
    case Unused_Pointer_Type:
        mUnusedPointer = NULL;
        break;
    case Opaque_Type:
        delete mOpaqueIns;
        mOpaqueIns = NULL;
        mOpaqueType = BufferObjectReferenceType;
        break;
    default:
        break;
    };
    mType = Void_Type;
    mName = std::string();
}

bool ValueTM::IsVoid() const
{
    return mType == Void_Type;
}

bool ValueTM::IsPointer() const
{
    return mType == Pointer_Type;
}

bool ValueTM::IsUnusedPointer() const
{
    return mType == Unused_Pointer_Type;
}

bool ValueTM::IsIntegral() const
{
    return mType == Enum_Type ||
           mType == Int8_Type ||
           mType == Uint8_Type ||
           mType == Int16_Type ||
           mType == Uint16_Type ||
           mType == Int_Type ||
           mType == Uint_Type ||
           mType == Int64_Type ||
           mType == Uint64_Type;
}

bool ValueTM::IsFloatingPoint() const
{
    return mType == Float_Type;
}

bool ValueTM::IsNumerical() const
{
    return IsIntegral() ||
        IsFloatingPoint();
}

bool ValueTM::IsString() const
{
    return mType == String_Type;
}

bool ValueTM::IsArray() const
{
    return mType == Array_Type;
}

bool ValueTM::IsBlob() const
{
    return mType == Blob_Type || (mType == Opaque_Type && mOpaqueType == BlobType);
}

bool ValueTM::IsBufferReference() const
{
    return mType == Opaque_Type && mOpaqueType == BufferObjectReferenceType;
}

bool ValueTM::IsClientSideBufferReference() const
{
    return mType == Opaque_Type && mOpaqueType == ClientSideBufferObjectReferenceType;
}

unsigned char ValueTM::GetAsUByte() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<unsigned char>(mUint8);
    case Uint16_Type: return static_cast<unsigned char>(mUint16);
    case Uint_Type: return static_cast<unsigned char>(mUint);
    case Uint64_Type: return static_cast<unsigned char>(mUint64);
    case Int8_Type: return static_cast<unsigned char>(mInt8);
    case Int16_Type: return static_cast<unsigned char>(mInt16);
    case Int_Type: return static_cast<unsigned char>(mInt);
    case Int64_Type: return static_cast<unsigned char>(mInt64);
    case Enum_Type: return static_cast<unsigned char>(mEnum);
    case Float_Type: return static_cast<unsigned char>(mFloat);
    default:
        DBG_LOG("Unexpected value type %d in GetAsUByte()\n", mType);
        return 0;
    }
}

unsigned short ValueTM::GetAsUShort() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<unsigned short>(mUint8);
    case Uint16_Type: return static_cast<unsigned short>(mUint16);
    case Uint_Type: return static_cast<unsigned short>(mUint);
    case Uint64_Type: return static_cast<unsigned short>(mUint64);
    case Int8_Type: return static_cast<unsigned short>(mInt8);
    case Int16_Type: return static_cast<unsigned short>(mInt16);
    case Int_Type: return static_cast<unsigned short>(mInt);
    case Int64_Type: return static_cast<unsigned short>(mInt64);
    case Enum_Type: return static_cast<unsigned short>(mEnum);
    case Float_Type: return static_cast<unsigned short>(mFloat);
    default:
        DBG_LOG("Unexpected value type %d in GetAsUShort()\n", mType);
        return 0;
    }
}

unsigned ValueTM::GetAsUInt() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<unsigned>(mUint8);
    case Uint16_Type: return static_cast<unsigned>(mUint16);
    case Uint_Type: return static_cast<unsigned>(mUint);
    case Uint64_Type: return static_cast<unsigned>(mUint64);
    case Int8_Type: return static_cast<unsigned>(mInt8);
    case Int16_Type: return static_cast<unsigned>(mInt16);
    case Int_Type: return static_cast<unsigned>(mInt);
    case Int64_Type: return static_cast<unsigned>(mInt64);
    case Enum_Type: return static_cast<unsigned>(mEnum);
    case Float_Type: return static_cast<unsigned>(mFloat);
    default:
        DBG_LOG("Unexpected value type %d in GetAsUInt()\n", mType);
        return 0;
    }
}

int ValueTM::GetAsInt() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<int>(mUint8);
    case Uint16_Type: return static_cast<int>(mUint16);
    case Uint_Type: return static_cast<int>(mUint);
    case Uint64_Type: return static_cast<int>(mUint64);
    case Int8_Type: return static_cast<int>(mInt8);
    case Int16_Type: return static_cast<int>(mInt16);
    case Int_Type: return static_cast<int>(mInt);
    case Int64_Type: return static_cast<int>(mInt64);
    case Enum_Type: return static_cast<int>(mEnum);
    case Float_Type: return static_cast<int>(mFloat);
    default:
        DBG_LOG("Unexpected value type %d in GetAsInt()\n", mType);
        return 0;
    }
}

float ValueTM::GetAsFloat() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<float>(mUint8);
    case Uint16_Type: return static_cast<float>(mUint16);
    case Uint_Type: return static_cast<float>(mUint);
    case Uint64_Type: return static_cast<float>(mUint64);
    case Int8_Type: return static_cast<float>(mInt8);
    case Int16_Type: return static_cast<float>(mInt16);
    case Int_Type: return static_cast<float>(mInt);
    case Int64_Type: return static_cast<float>(mInt64);
    case Enum_Type: return static_cast<float>(mEnum);
    case Float_Type: return static_cast<float>(mFloat);
    default:
        DBG_LOG("Unexpected value type %d in GetAsFloat()\n", mType);
        return 0;
    }
}

long long ValueTM::GetAsInt64() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<long long>(mUint8);
    case Uint16_Type: return static_cast<long long>(mUint16);
    case Uint_Type: return static_cast<long long>(mUint);
    case Uint64_Type: return static_cast<long long>(mUint64);
    case Int8_Type: return static_cast<long long>(mInt8);
    case Int16_Type: return static_cast<long long>(mInt16);
    case Int_Type: return static_cast<long long>(mInt);
    case Int64_Type: return static_cast<long long>(mInt64);
    case Enum_Type: return static_cast<long long>(mEnum);
    case Float_Type: return static_cast<long long>(mFloat);
    case Opaque_Type:
        switch (mOpaqueType)
        {
        case BufferObjectReferenceType:
            return static_cast<long long>(mOpaqueIns->mUint);
        default:
            break;
        }
        // fall through
    default:
        DBG_LOG("Unexpected value type %d in GetAsInt64()\n", mType);
        return 0;
    }
}

unsigned long long ValueTM::GetAsUInt64() const
{
    switch (mType)
    {
    case Uint8_Type: return static_cast<unsigned long long>(mUint8);
    case Uint16_Type: return static_cast<unsigned long long>(mUint16);
    case Uint_Type: return static_cast<unsigned long long>(mUint);
    case Uint64_Type: return static_cast<unsigned long long>(mUint64);
    case Int8_Type: return static_cast<unsigned long long>(mInt8);
    case Int16_Type: return static_cast<unsigned long long>(mInt16);
    case Int_Type: return static_cast<unsigned long long>(mInt);
    case Int64_Type: return static_cast<unsigned long long>(mInt64);
    case Enum_Type: return static_cast<unsigned long long>(mEnum);
    case Float_Type: return static_cast<unsigned long long>(mFloat);
    case Opaque_Type:
        switch (mOpaqueType)
        {
        case BufferObjectReferenceType:
            return static_cast<unsigned long long>(mOpaqueIns->mUint);
        default:
            break;
        }
        // fall through
    default:
        DBG_LOG("Unexpected value type %d in GetAsUInt64()\n", mType);
        return 0;
    }
}

void ValueTM::SetAsUByte(unsigned char value)
{
    Reset();
    mType = Uint8_Type;
    mUint8 = value;
}

void ValueTM::SetAsUShort(unsigned short value)
{
    Reset();
    mType = Uint16_Type;
    mUint16 = value;
}

void ValueTM::SetAsUInt(unsigned int value)
{
    Reset();
    mType = Uint_Type;
    mUint = value;
}

void ValueTM::SetAsFloat(float value)
{
    Reset();
    mType = Float_Type;
    mFloat = value;
}

const std::string ValueTM::GetAsString(int maxLen) const
{
    if (IsString() && maxLen < 0)
    {
        return mStr;
    }
    else if (IsString() && maxLen >= 0)
    {
        return std::string(mStr, 0, maxLen);
    }
    else
    {
        printf("Unexpected value type %d in GetAsString()\n", mType);
        return "";
    }
}

void ValueTM::SetAsString(const std::string & value)
{
    Reset();
    mType = String_Type;
    mStr = value;
}

const std::string ValueTM::GetAsBlob() const
{
    if (mType == Blob_Type)
    {
        return std::string(mBlob, mBlobLen);
    }
    else if (mType == Opaque_Type && mOpaqueType == BlobType)
    {
        return std::string(mOpaqueIns->mBlob, mOpaqueIns->mBlobLen);
    }
    else
    {
        printf("Unexpected value type %d in GetAsBlob()\n", mType);
        return "";
    }
}

void ValueTM::SetAsBlob(const std::string & value)
{
    if (mType == Opaque_Type && mOpaqueType == BlobType)
    {
        delete [] mOpaqueIns->mBlob;
        mOpaqueIns->mBlob = NULL;
        mOpaqueIns->mBlobLen = 0;
        if (value.size())
        {
            mOpaqueIns->mBlobLen = value.size();
            mOpaqueIns->mBlob = new char[mOpaqueIns->mBlobLen];
            memcpy(mOpaqueIns->mBlob, value.c_str(), value.size());
        }
    }
    else
    {
        Reset();
        mType = Blob_Type;
        mBlobLen = 0;
        mBlob = NULL;
        if (value.size())
        {
            mBlobLen = value.size();
            mBlob = new char[mBlobLen];
            memcpy(mBlob, value.c_str(), mBlobLen);
        }
    }
}

unsigned int ValueTM::GetAsBufferReference() const
{
    if (IsBufferReference())
    {
        return mOpaqueIns->GetAsUInt();
    }
    else
    {
        printf("Unexpected value type %d in GetAsBufferReference()\n", mType);
        return 0;
    }
}

void ValueTM::SetAsBufferReference(unsigned int value)
{
    Reset();
    mType = Opaque_Type;
    mOpaqueType = BufferObjectReferenceType;
    mOpaqueIns = new ValueTM(value);
}

void ValueTM::GetAsClientSideBufferReference(unsigned int &name, unsigned int &offset)
{
    if (IsClientSideBufferReference())
    {
        name = mOpaqueIns->mClientSideBufferName;
        offset = mOpaqueIns->mClientSideBufferOffset;
    }
    else
    {
        printf("Unexpected value type %d in GetAsBlob()\n", mType);
    }
}

void ValueTM::SetAsClientSideBufferReference(unsigned int name, unsigned int offset)
{
    Reset();
    mType = Opaque_Type;
    mOpaqueType = ClientSideBufferObjectReferenceType;
    mOpaqueIns = new ValueTM();
    mOpaqueIns->mClientSideBufferName = name;
    mOpaqueIns->mClientSideBufferOffset = offset;
}

void ValueTM::ResizeArray(unsigned int size)
{
    if (mType != Array_Type)
    {
        Reset();
        if (size > 0)
            mArray = new ValueTM[size];
        else
            mArray = 0;
        mType = Array_Type;
        mArrayLen = size;
        mEleType = Void_Type;
    }
    else if (mArrayLen != size)
    {
        ValueTM *buffer = 0;
        if (size > 0)
        {
            buffer = new ValueTM[size];
            for (unsigned int i = 0; i < std::min(mArrayLen, size); ++i)
                buffer[i] = mArray[i];
        }
        delete [] mArray;
        mArray = buffer;
        mArrayLen = size;
    }
}

std::string ValueTM::ToStr(const CallTM *call, int maxLen)
{
    std::stringstream sstream;

    if (mName.size())
        sstream<<mName<<"=";

    sstream << ToC(call, false);

    if (maxLen != 0 && int(sstream.str().length()) > maxLen)
        return sstream.str().substr(0, maxLen)+"...";
    else
        return sstream.str();
}

std::string ValueTM::ToC(const CallTM *call, bool asSourceCode)
{
    const std::string &funcName = call->mCallName;
    std::stringstream sstream;

    const char * enumTmp = NULL;
    std::string strTmp;

    switch (mType) {
    case Void_Type:
        return "void";
    case Int8_Type:
        sstream<<(int)mInt8;
        break;
    case Int_Type:
        if (funcName == "glSamplerParameteri" || funcName == "glTexParameteri" ||
            funcName == "glTexEnvx" || funcName == "glTexParameterx") // special case this
        {
            GLenum pname = call->mArgs[1]->GetAsUInt();
            if (pname != GL_TEXTURE_MAX_LOD && pname != GL_TEXTURE_MIN_LOD && pname != GL_TEXTURE_BASE_LEVEL
                && pname != GL_TEXTURE_MAX_LEVEL)
            {
                sstream << EnumString(mInt, funcName);
                return sstream.str();
            }
        }
        sstream<<mInt;
        break;
    case Int64_Type:
        sstream<<mInt64;
        break;
    case Uint64_Type:
        sstream<<mUint64;
        break;
    case Int16_Type:
        sstream<<mInt16;
        break;
    case Uint16_Type:
        sstream<<mUint16;
        break;
    case Uint8_Type:
        sstream<<(int)mUint8;
        if (asSourceCode) sstream<<'u';
        break;
    case Uint_Type:
        sstream<<mUint;
        if (asSourceCode) sstream<<'u';
        break;
    case Enum_Type:
        enumTmp = EnumString(mEnum, funcName);
        if (enumTmp == NULL) {
            sstream << "0x" << std::hex << std::uppercase << mEnum;
            //DBG_LOG("enum %s in %s not found\n", sstream.str().c_str(), funcName.c_str());
        } else {
            sstream<<enumTmp;
        }
        break;
    case Float_Type:
        if (funcName == "glSamplerParameterf" || funcName == "glTexParameterf") // special case this
        {
            GLenum pname = call->mArgs[1]->GetAsUInt();
            if (pname != GL_TEXTURE_MAX_LOD && pname != GL_TEXTURE_MIN_LOD && pname != GL_TEXTURE_BASE_LEVEL
                && pname != GL_TEXTURE_MAX_LEVEL)
            {
                sstream << EnumString(mFloat, funcName);
                return sstream.str();
            }
        }
        if (asSourceCode){
            // check float is not NaN or inf
            if( std::isfinite(mFloat)) {
                sstream<<mFloat;
            } else {
                sstream<<-1; // could define a special "bad float" variable, but, just output -1 for now.
            }
        } else{
            sstream<<mFloat;
        }
        break;
    case String_Type:
        strTmp = std::string(mStr);
        replaceString(strTmp, "\"", "\\\""); // escape quotes
        sstream << "\"" << strTmp << "\""; //surround with quotes
        break;
    case Array_Type:
        if (mArrayLen) {
            sstream << "{";
            for (unsigned int i = 0; i < mArrayLen-1; ++i) {
                sstream << mArray[i].ToC(call) << ", ";
            }
            sstream << mArray[mArrayLen-1].ToC(call);
            sstream << "}";
        } else if (mArrayLen == 0) {
            sstream<<"NULL";
        } else {
            sstream<<"badptr";
        }
        break;
    case MemRef_Type:
        sstream<<mClientSideBufferName<<" + "<<mClientSideBufferOffset;
        break;
    case Opaque_Type:
        // output an enum, the struct containing Opaque variables must be declared before call is output as c-code
        switch (mOpaqueType)
        {
            case BufferObjectReferenceType:
                sstream << "common::BufferObjectReferenceType/*" << mOpaqueIns->mUint << "*/";
                break;
            case BlobType:
                sstream << "common::BlobType/*BlobSize:" << mOpaqueIns->mBlobLen << "*/";
                break;
            case ClientSideBufferObjectReferenceType:
                sstream << "common::ClientSideBufferObjectReferenceType(" << mOpaqueIns->mClientSideBufferName << ", " << mOpaqueIns->mClientSideBufferOffset << ")";
                break;
            case NoopType:
                break;
        }
        break;
    case Pointer_Type:
        if (mPointer)
            sstream << mPointer->ToStr(call);
        else
            sstream<<"NULL";
        break;
    case Unused_Pointer_Type:
        sstream << mUnusedPointer;
        break;
    case Blob_Type:
        if (mBlobLen) {
            if (asSourceCode) {sstream << "(GLubyte*)";}
            sstream << "_binary_blob_" << mId << "_bin_start";
            sstream << "/*BlobSize" << mBlobLen << "*/";
        } else {
            sstream << "NULL";
        }
        break;
    };

    return sstream.str();
}


std::string ValueTM::TypeNameToStr()
{
    switch (mType) {
        case Void_Type:
            return "void";
        case Int64_Type:
            return "GLint64";
        case Int16_Type:
        case Int8_Type:
        case Int_Type:
            return "GLint";
        case Uint64_Type:
            return "GLuint64";
        case Uint16_Type:
        case Uint8_Type:
        case Uint_Type:
            return "GLuint";
        case Enum_Type:
            return "GLenum";
        case Float_Type:
            return "float";
        case String_Type:
            return "const char*";
        case Array_Type:
            if (mArrayLen) {
                return mArray[0].TypeNameToStr();
            } else
                return "<empty>";
        case MemRef_Type:
            return "memref";
        case Opaque_Type:
            return "opaque";
        case Pointer_Type:
            if (mPointer)
                return mPointer->TypeNameToStr()+"*";
            else
                return "NULL";
        case Unused_Pointer_Type:
            return "void*";
        case Blob_Type:
            return "blob";
        default:
            return "unknown_type";
    };
}


char* ValueTM::Serialize(char* dest, bool doPadding)
{
    switch (mType) {
    case Int8_Type:
        dest = WriteFixed(dest, mInt8, doPadding);
        break;
    case Int_Type:
        dest = WriteFixed(dest, mInt, doPadding);
        break;
    case Uint8_Type:
        dest = WriteFixed(dest, mUint8, doPadding);
        break;
    case Uint_Type:
        dest = WriteFixed(dest, mUint, doPadding);
        break;
    case Int16_Type:
        dest = WriteFixed(dest, mInt16, doPadding);
        break;
    case Uint16_Type:
        dest = WriteFixed(dest, mUint16, doPadding);
        break;
    case Int64_Type:
        dest = WriteFixed(dest, mInt64, doPadding);
        break;
    case Uint64_Type:
        dest = WriteFixed(dest, mUint64, doPadding);
        break;
    case Enum_Type:
        dest = WriteFixed(dest, mEnum, doPadding);
        break;
    case Float_Type:
        dest = WriteFixed(dest, mFloat, doPadding);
        break;
    case String_Type:
        dest = WriteString(dest, mStr.c_str());
        break;
    case Array_Type:
        if (mEleType != String_Type) {
            unsigned int byLen = mArrayLen * gValueTypeSize[mEleType];
            dest = WriteFixed(dest, byLen, true);
            for (unsigned int i = 0; i < mArrayLen; ++i)
                dest = mArray[i].Serialize(dest, false);
            dest = padwrite(dest);
        } else {
            if (mArrayLen == 0)
                dest = WriteStringArray(dest, 0, NULL);
            else {
                const char** strArr = new const char*[mArrayLen];
                for (unsigned int i = 0; i < mArrayLen; ++i)
                    strArr[i] = mArray[i].mStr.c_str();
                dest = WriteStringArray(dest, mArrayLen, strArr);
                delete [] strArr;
            }
        }
        break;
    case Blob_Type:
        dest = Write1DArray<char>(dest, mBlobLen, mBlob);
        break;
    case Opaque_Type:
        dest = WriteFixed<unsigned int>(dest, mOpaqueType);
        if (mOpaqueIns)
            dest = mOpaqueIns->Serialize(dest, true);
        break;
    case Pointer_Type:
        dest = WriteFixed<unsigned int>(dest, mPointer ? 1 : 0);
        if (mPointer)
            dest = mPointer->Serialize(dest, true);
        break;
    case Unused_Pointer_Type:
        dest = WriteFixed<void*>(dest, mUnusedPointer, doPadding);
        break;
    case MemRef_Type:
        dest = WriteFixed(dest, mClientSideBufferName);
        dest = WriteFixed(dest, mClientSideBufferOffset);
        break;
    default:
        DBG_LOG("Failed to serialize type: %d\n", mType);
        break;
    };
    return dest;
}

ValueTM* CreateEnumValue(unsigned int value)
{
    ValueTM *enum_value = new ValueTM;
    enum_value->mType = Enum_Type;
    enum_value->mEnum = value;
    return enum_value;
}

ValueTM* CreateUInt8Value(unsigned int value)
{
    assert(value <= 0xFF);
    ValueTM *uint8_value = new ValueTM;
    uint8_value->mType = Uint8_Type;
    uint8_value->mUint8 = static_cast<unsigned char>(value);
    return uint8_value;
}

ValueTM * CreateInt32Value(int value)
{
    return new ValueTM(value);
}

ValueTM* CreateStringValue(const std::string &value)
{
    return new ValueTM(value);
}

ValueTM * CreateUInt32Value(unsigned int value)
{
    return new ValueTM(value);
}

ValueTM * CreateInt32ArrayValue(const std::vector<int> *values)
{
    if (values == NULL)
    {
        ValueTM *array = new ValueTM;
        array->mType = Array_Type;
        array->mArrayLen = 0;
        array->mEleType = Int_Type;
        array->mArray = NULL;
        return array;
    }
    else
    {
        const unsigned int length = values->size();
        ValueTM *array = new ValueTM;
        array->mType = Array_Type;
        array->mArrayLen = length;
        array->mEleType = Int_Type;
        array->mArray = new ValueTM[length];
        for (unsigned int index = 0; index < length; index++)
        {
            array->mArray[index].mType = Int_Type;
            array->mArray[index].mUint = (*values)[index];
        }
        return array;
    }
}

ValueTM * CreateUInt32ArrayValue(const std::vector<unsigned int> &values)
{
    const unsigned int length = values.size();
    ValueTM *array = new ValueTM;
    array->mType = Array_Type;
    array->mArrayLen = length;
    array->mEleType = Uint_Type;
    array->mArray = new ValueTM[length];
    for (unsigned int index = 0; index < length; index++)
    {
        array->mArray[index].mType = Uint_Type;
        array->mArray[index].mUint = values[index];
    }
    return array;
}

ValueTM * CreateStringArrayValue(const std::vector<std::string> &values)
{
    const unsigned int length = values.size();
    ValueTM *array = new ValueTM;
    array->mType = Array_Type;
    array->mArrayLen = length;
    array->mEleType = String_Type;
    array->mArray = new ValueTM[length];
    for (unsigned int index = 0; index < length; index++)
    {
        array->mArray[index].mType = String_Type;
        array->mArray[index].mStr = values[index];
    }
    return array;
}

ValueTM * CreateBlobValue(unsigned int data_size, const char *data)
{
    return new ValueTM(data, data_size);
}

ValueTM * CreateBlobOpaqueValue(unsigned int data_size, const char *data)
{
    ValueTM *value = new ValueTM;
    value->mType = Opaque_Type;
    value->mOpaqueType = BlobType;
    value->mOpaqueIns = CreateBlobValue(data_size, data);
    return value;
}

ValueTM * CreateBufferReferenceOpaqueValue(unsigned int offset)
{
    ValueTM *value = new ValueTM;
    value->mType = Opaque_Type;
    value->mOpaqueType = BufferObjectReferenceType;
    value->mOpaqueIns = CreateUInt32Value(offset);
    return value;
}

CallTM::CallTM(InFileBase &infile, unsigned callNo, const BCall_vlen &call)
 : mCallNo(callNo), mTid(call.tid), mCallId(call.funcId), mBkColor(0xffffffff), mTxtColor(0x000000ff)
{
    const std::string name = infile.ExIdToName(mCallId);
    const void *fptr = parse_callbacks.at(name);
    mRet.mName = "ret";
    mCallErrNo = static_cast<CALL_ERROR_NO>(call.errNo);
    mReadPos = infile.GetReadPos();
    char *src = infile.dataPointer();
    (*(ParseFunc)fptr)(src, *this, infile);
}

bool CallTM::Load(InFileRA *infile)
{
    void*           fptr;
    common::BCall   curCall;
    char*           src;

    mReadPos = infile->GetReadPos();
    if (!infile->GetNextCall(fptr, curCall, src)) {
        DBG_LOG("File inconsistent!\n");
        return false;
    }
    mTid = curCall.tid;
    mCallErrNo = static_cast<CALL_ERROR_NO>(curCall.errNo);

    if (fptr)
        (*(ParseFunc)fptr)(src, *this, *infile);
    else
        mCallName = infile->ExIdToName(curCall.funcId);

    this->Stylize();

    return true;
}

std::string CallTM::ToStr(bool isAbbreviate)
{
    std::string strArgs;
    for (unsigned int i = 0; i < mArgs.size(); ++i) {
        strArgs += mArgs[i]->ToStr(this, isAbbreviate ? 32 : 0);
        if (i != mArgs.size()-1)
            strArgs += ", ";
    }

    std::string strErr;
    if (mCallErrNo != CALL_GL_NO_ERROR) {
        switch (mCallErrNo) {
        case CALL_GL_INVALID_ENUM:
            strErr = " ERR: GL_INVALID_ENUM";
            break;
        case CALL_GL_INVALID_VALUE:
            strErr = " ERR: GL_INVALID_VALUE";
            break;
        case CALL_GL_INVALID_OPERATION:
            strErr = " ERR: GL_INVALID_OPERATION";
            break;
        case CALL_GL_INVALID_FRAMEBUFFER_OPERATION:
            strErr = " ERR: GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case CALL_GL_OUT_OF_MEMORY:
            strErr = " ERR: GL_OUT_OF_MEMORY";
            break;
        default:
            break;
        }
    }
    return mRet.ToStr(this, isAbbreviate ? 32 : 0) + " " + mCallName + "(" + strArgs + ")" + strErr;
}

char* CallTM::Serialize(char* dest, int overrideID)
{
    if (mCallId == 0) // This call is not supported by ApiInfo
    {
        DBG_LOG("ERROR: Call %s not supported by ApiInfo -- this will probably not work!\n", !mCallName.empty() ? mCallName.c_str() : "???");
        return dest;
    }

    BCall_vlen *pCallVlen = (BCall_vlen*)dest;
    int len = gApiInfo.IdToLenArr[mCallId];
    if (len) {
        BCall *pCall = (BCall*)dest;
        if (overrideID == -1)
            pCall->funcId = mCallId;
        else
            pCall->funcId = overrideID;
        pCall->tid = mTid;
        pCall->reserved = 0;
        dest += sizeof(BCall);
    } else {
        if (overrideID == -1)
            pCallVlen->funcId = mCallId;
        else
            pCallVlen->funcId = overrideID;
        pCallVlen->tid = mTid;
        pCallVlen->reserved = 0;
        dest += sizeof(BCall_vlen);
    }

    for (unsigned int i = 0; i < mArgs.size(); ++i) {
        dest = mArgs[i]->Serialize(dest, true);
    }

    if (mRet.mType != Void_Type)
        dest = mRet.Serialize(dest, true);

    if (len == 0)
        pCallVlen->toNext = (dest - (char*)pCallVlen);

    return dest;
}

void CallTM::Stylize()
{
    static const string strDraw = "glDraw";
    static const string strBindFB = "glBindFramebuffer";

    if (strDraw.compare(0, strDraw.length(), mCallName.c_str(), strDraw.length()) == 0)
        mTxtColor = 0x00ff00ff;
    if (strBindFB.compare(mCallName) == 0)
        mTxtColor = 0xff0000ff;
    if (mTid != 0)
        mBkColor = 0xffff8fff;
    if (mCallErrNo != CALL_GL_NO_ERROR) {
        mTxtColor = 0xffffffff;
        mBkColor = 0x000000ff;
    }
}

std::string CallTM::ToCppCall()
{
    std::string arrayDeclarations; // float* arrNameXXXX[] = {0,1,2,3,4,5};
    std::vector<std::string> arrayVariableNames; // arrNameXXXX

    for (unsigned i = 0; i < mArgs.size(); i++)
    {
        ValueTM& arg = *mArgs[i];
        if (arg.mType == Array_Type)
        {
            if (!arg.mArrayLen)
            {
                continue;
            }
            else
            {
                std::string arrayValues = arg.ToC(this, true);
                // Shaders often span multiple lines, add quotes to line start and end
                if (arg.mEleType == String_Type)
                {
                    replaceString(arrayValues, "\r\n", "\n");
                    replaceString(arrayValues, "\r", "\n");
                    replaceString(arrayValues, "\n", "\\n\"\n    \""); //replace newlines with  quote, literalnewline,  newline, indent quote
                }

                std::stringstream sstream;
                sstream << "array_" << mArgs[i]->mId;
                arrayVariableNames.push_back(sstream.str());

                std::string arrayDecl = mArgs[i]->TypeNameToStr() + " " + arrayVariableNames.back() + "[]=" + arrayValues + ";\n";
                arrayDeclarations = arrayDeclarations + arrayDecl;
            }
        }
    }

    // special case for annoying "cameleon" variable. The Opaque Value
    std::string opaqueArgDecl = "";

    for (unsigned int i = 0; i < mArgs.size(); ++i)
    {
        ValueTM& arg = *mArgs[i];
        if (arg.mType == Opaque_Type)
        {
            std::stringstream sstream;

            sstream << "common::OpaqueArg params;\n";

            switch(arg.mOpaqueType)
            {
                case BufferObjectReferenceType:
                    sstream<<"    params.pointer_raw = "<<arg.mOpaqueIns->mUint<<";\n";
                    break;
                case BlobType:
                    //sstream<<"    params.pointer_blob = "<<arg.mOpaqueIns->mBlob<<";\n";
                    break;
                case ClientSideBufferObjectReferenceType:
                    sstream<<"    params.mClientSideBufferName = "<<arg.mOpaqueIns->mClientSideBufferName<<";\n";
                    sstream<<"    params.mClientSideBufferOffset = "<<arg.mOpaqueIns->mClientSideBufferOffset<<";\n";
                    break;
                case NoopType:
                    break;
            };

            opaqueArgDecl = sstream.str();
            break;
        }
    }

    // convert function parameters to a long string
    std::string strArgs = "";
    int numArrays = 0;
    for (unsigned int i = 0; i < mArgs.size(); ++i) {
        ValueTM& arg = *mArgs[i];
        switch (arg.mType) {
            case Array_Type:
            if ( arg.mArrayLen > 0 ) {
                strArgs += "&" + arrayVariableNames[numArrays++]+"[0]";
                } else {
                    // NULL's are stored as arrays, dont skip them, but use ToC()
                    strArgs += arg.ToC(this, true);
                }
                break;

            default:
                strArgs += arg.ToC(this, true);
        }

        if (i != mArgs.size()-1)
        strArgs += ", ";
    }

    // special case for functions where we need to know old return value
    typedef std::list<std::string> StringList;
    StringList pass_old_ret_list;
    pass_old_ret_list.push_back("glCreateProgram");
    pass_old_ret_list.push_back("glCreateShader");
    pass_old_ret_list.push_back("glGetAttribLocation");
    pass_old_ret_list.push_back("glGetUniformLocation");
    pass_old_ret_list.push_back("glCreateShaderProgramv");
    pass_old_ret_list.push_back("glCreateShaderProgramvEXT");
    StringList::iterator it = std::find( pass_old_ret_list.begin(), pass_old_ret_list.end(), std::string(mCallName) );
    if ( it != pass_old_ret_list.end() ) {
        if ( strArgs == "" ) {
            strArgs = mRet.ToC(this, true) + "/*old_ret*/";
        } else {
            strArgs = mRet.ToC(this, true) + "/*old_ret*/, " + strArgs;
        }
    }

    // special case. need to map return value from eglCreateContext
    if (mCallName == "eglCreateContext" || mCallName == "eglCreateWindowSurface")
    {
        strArgs = mRet.ToC(this, true) + ", " + strArgs;
    }

    std::string callWithParams = std::string(mCallName) + "(" + strArgs + ");";

    // Need a better solution...
    if (numArrays)
    {
        // since any declared literals are only used by this call, surround with braces to avoid name-crashes
        return "{ " + arrayDeclarations + callWithParams + " }";
    }
    else if (opaqueArgDecl != "")
    {
        std::string callWithParams = std::string(mCallName) + "(" + strArgs + ", params);";
        return "{ " + opaqueArgDecl + callWithParams + " }";
    } else {
        return callWithParams;
    }
    return "dummy";
}

void FrameTM::LoadCalls(InFileRA *infile, bool loadQuery, const std::string &loadFilter)
{
    if (mIsLoaded)
        return;

    infile->SetReadPos(mReadPos);

    common::BCall       curCall;

    for (unsigned int i = 0; i < GetCallCount(); ++i) {
        CallTM*             newCallTM = new CallTM;

        if (!newCallTM->Load(infile)) {
            DBG_LOG("File inconsistent!\n");
            delete newCallTM;
            break;
        }

        newCallTM->mCallNo = mFirstCallOfThisFrame + i;

        if (!loadQuery && IsQueryCall(newCallTM->mCallName))
            delete newCallTM;
        else if (loadFilter.size() && !MatchFilterString(newCallTM->mCallName, loadFilter))
            delete newCallTM;
        else
            mCalls.push_back(newCallTM);
    }

    mIsLoaded = true;
}

void FrameTM::LoadCallsForTraceToTxt(InFileRA *infile, bool loadQuery, const std::string &loadFilter, unsigned int callNum, unsigned int fromFirstCall, unsigned int max_cycle)
{
    infile->SetReadPos(mReadPos);

    common::BCall       curCall;

    unsigned int cycleNum = callNum;

    for (unsigned int i = 0; i < cycleNum; ++i) {
        CallTM*             newCallTM = new CallTM;

        if (!newCallTM->Load(infile)) {
            DBG_LOG("File inconsistent!\n");
            delete newCallTM;
            break;
        }

        newCallTM->mCallNo = mFirstCallOfThisFrame + (fromFirstCall * max_cycle) + i;//1000000 equals MAX_CYCLE in trace_to_txt/main.cpp

        if (!loadQuery && IsQueryCall(newCallTM->mCallName))
            delete newCallTM;
        else if (loadFilter.size() && !MatchFilterString(newCallTM->mCallName, loadFilter))
            delete newCallTM;
        else
            mCalls.push_back(newCallTM);
    }

    mReadPos = infile->GetReadPos();
    mIsLoaded = true;
}

void FrameTM::UnloadCalls()
{
    if (!mIsLoaded)
        return;

    for (unsigned int i = 0; i < mCalls.size(); ++i)
        delete mCalls[i];
    mCalls.clear();
    mIsLoaded = false;
}

TraceFileTM::TraceFileTM()
: mpInFileRA(new InFileRA),
  mCurFrameIndex(0),
  mCurCallIndexInFrame(0),
  mLoadQueryCalls(false),
  mLoadFilterStr(DEFAULT_LOAD_FILTER_STRING)
{
}

TraceFileTM::~TraceFileTM() {
    Clear();
    delete mpInFileRA;
    mpInFileRA = NULL;
}

void TraceFileTM::Close()
{
    mpInFileRA->Close();
}

TraceFileTM::TraceFileTM(const char *name, bool readHeaderAndExit)
: mpInFileRA(new InFileRA),
  mCurFrameIndex(0),
  mCurCallIndexInFrame(0),
  mLoadQueryCalls(false),
  mLoadFilterStr(DEFAULT_LOAD_FILTER_STRING)
{
    Open(name, readHeaderAndExit);
}

void TraceFileTM::Clear()
{
    for (unsigned int i = 0; i < mFrames.size(); ++i)
        delete mFrames[i];
    mFrames.clear();
}

bool TraceFileTM::Open(const char* name, bool readHeaderAndExit, const std::string& ra_target)
{
    // Opens random access file
    // Creates the first frame object
    // scans tracefile for calls pushing, creating new frimes when hitting frame terminators.
    // Each frame stores readpos in RA file

    gApiInfo.RegisterEntries(parse_callbacks);

    if (!mpInFileRA->Open(name, readHeaderAndExit, ra_target))
        return false;

    Clear();

    if (readHeaderAndExit)
        return true;

    unsigned short eglSwapBuffers_id = mpInFileRA->NameToExId("eglSwapBuffers");
    unsigned short eglSwapBuffersWithDamage_id = mpInFileRA->NameToExId("eglSwapBuffersWithDamageKHR");

    void*               fptr = (void*)0xdeadbeef;
    common::BCall       curCall;
    char*               src = (char*)0x0;
    unsigned int        callNo = 0;

    FrameTM*            newFrame = new FrameTM;
    newFrame->mReadPos = mpInFileRA->GetReadPos();
    newFrame->mFirstCallOfThisFrame = 0;
    const int tid = mpInFileRA->getDefaultThreadID();

    std::streamoff lastReadPos = 0;

    // scan file for every single call, divide into frames
    while (mpInFileRA->GetNextCall(fptr, curCall, src))
    {
        // separate into frames according to eglSwapBuffers or
        // eglDestroySurface call of retraced thread
        if (curCall.tid == tid &&
            (curCall.funcId == eglSwapBuffers_id ||
             curCall.funcId == eglSwapBuffersWithDamage_id)) {
                newFrame->SetCallCount(callNo-newFrame->mFirstCallOfThisFrame+1);
                newFrame->mBytes = mpInFileRA->GetReadPos() - newFrame->mReadPos;
                mFrames.push_back(newFrame);

                newFrame = new FrameTM;
                newFrame->mReadPos = mpInFileRA->GetReadPos();
                newFrame->mFirstCallOfThisFrame = callNo+1;
        }

        // Save the position after each successful call reading
        lastReadPos = mpInFileRA->GetReadPos();
        callNo++;
    }

    // if current created frame is valid calculate number of calls and megabytes of calls in it.
    if (callNo > newFrame->mFirstCallOfThisFrame)
    {
        newFrame->SetCallCount(callNo-newFrame->mFirstCallOfThisFrame);
        newFrame->mBytes = lastReadPos - newFrame->mReadPos;
        mFrames.push_back(newFrame);
    }
    else // frame contains zero calls?
    {
        delete newFrame; // so, now we have a hanging pointer n mFrames? Does this ever occur?
        newFrame = 0x0;
    }

    return true;
}

CallTM *TraceFileTM::NextCall() const
{
    if (mFrames.size() == 0)
        return NULL;

    FrameTM *curFrame = mFrames[mCurFrameIndex];
    curFrame->LoadCalls(mpInFileRA);
    if (mCurCallIndexInFrame >= curFrame->GetLoadedCallCount())
    {
        curFrame->UnloadCalls();
        mCurCallIndexInFrame = 0;

        ++mCurFrameIndex;
        if (mCurFrameIndex >= mFrames.size())
        {
            // reset to the first frame
            mCurFrameIndex = 0;
            mFrames[mCurFrameIndex]->LoadCalls(mpInFileRA);
            return NULL;
        }

        mFrames[mCurFrameIndex]->LoadCalls(mpInFileRA);
        curFrame = mFrames[mCurFrameIndex];
    }
    return curFrame->mCalls[mCurCallIndexInFrame++];
}

CallTM *TraceFileTM::NextCallInFrame(unsigned int frameIndex) const
{
    if (mFrames.size() == 0 || mFrames.size() <= frameIndex)
        return NULL;

    FrameTM *curFrame = mFrames[frameIndex];
    curFrame->LoadCalls(mpInFileRA);
    if (mCurCallIndexInFrame >= curFrame->GetLoadedCallCount())
    {
        curFrame->UnloadCalls();
        mCurCallIndexInFrame = 0;
        return NULL;
    }
    return curFrame->mCalls[mCurCallIndexInFrame++];
}

unsigned int TraceFileTM::FindNext(unsigned int callNo, const char* name)
{
    int fr = GetFrameIdx(callNo);
    if (fr == -1)
        return callNo;

    for (; fr < int(mFrames.size()); ++fr) {
        FrameTM& frTM = *mFrames[fr];
        bool needToUnload = false;
        if (frTM.IsLoaded() == false)
        {
            frTM.LoadCalls(mpInFileRA, mLoadQueryCalls, mLoadFilterStr);
            needToUnload = true;
        }

        const unsigned int loadedCallCount = frTM.GetLoadedCallCount();
        for (unsigned int ca = 0; ca < loadedCallCount; ++ca) {
            if (frTM.mCalls[ca]->mCallNo <= callNo)
                continue;

            string callStr = frTM.mCalls[ca]->ToStr(false);
            if (callStr.find(name) != string::npos)
                return frTM.mCalls[ca]->mCallNo;
        }

        if (needToUnload)
            frTM.UnloadCalls();
    }

    return callNo;
}

unsigned int TraceFileTM::FindPrevious(unsigned int callNo, const char* name)
{
    int fr = GetFrameIdx(callNo);
    if (fr == -1)
        return callNo;

    for (; fr >= 0; --fr) {
        FrameTM& frTM = *mFrames[fr];
        bool needToUnload = false;
        if (frTM.IsLoaded() == false)
        {
            frTM.LoadCalls(mpInFileRA, mLoadQueryCalls, mLoadFilterStr);
            needToUnload = true;
        }

        const unsigned int loadedCallCount = frTM.GetLoadedCallCount();
        if (loadedCallCount > 0)
        {
            for (int ca = loadedCallCount-1; ca >= 0; --ca) {
                if (frTM.mCalls[ca]->mCallNo >= callNo)
                    continue;

                string callStr = frTM.mCalls[ca]->ToStr(false);
                if (callStr.find(name) != string::npos)
                    return frTM.mCalls[ca]->mCallNo;
            }
        }

        if (needToUnload)
            frTM.UnloadCalls();
    }

    return callNo;
}

int TraceFileTM::GetFrameIdx(unsigned int callNo) {
    unsigned int fr = 0;
    for (; fr < mFrames.size(); ++fr) {
        FrameTM& frTM = *mFrames[fr];
        if (callNo >= frTM.mFirstCallOfThisFrame &&
            callNo < frTM.mFirstCallOfThisFrame + frTM.GetCallCount())
            break;
    }
    return (fr == mFrames.size()) ? -1 : fr;
}



}
