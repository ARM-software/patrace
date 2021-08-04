// !!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// The data structures in this file are *not* supposed to be used in tracing or retracing.
// The performance and memory efficiency of these data structures are not good.
// They are *supposed* to be used in either post-processing or GUI tool, because
// they are more friendly to be accessed and iterated.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef _COMMON_TRACE_MODEL_H_
#define _COMMON_TRACE_MODEL_H_

#include <common/in_file_ra.hpp>
#include <common/in_file_mt.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#define RED_COM(x)      (((x)&0xff000000)>>24)
#define GREEN_COM(x)    (((x)&0x00ff0000)>>16)
#define BLUE_COM(x)     (((x)&0x0000ff00)>>8)
#define ALPHA_COM(x)    ((x)&0x000000ff)

namespace common {

// Forwad declaration
class TraceFileTM;
class CallTM;

enum Value_Type_TM {
    Void_Type = 1,
    Int8_Type,
    Uint8_Type,
    Int16_Type,
    Uint16_Type,
    Int_Type,
    Enum_Type,
    Uint_Type,
    Int64_Type,
    Uint64_Type,
    Float_Type,
    String_Type,
    Array_Type,
    Blob_Type,
    Opaque_Type,
    Pointer_Type,
    MemRef_Type,
    Unused_Pointer_Type,
};

enum Opaque_Type_TM {
    BufferObjectReferenceType = 0,
    BlobType,
    ClientSideBufferObjectReferenceType,
    NoopType,
};

struct OpaqueArg {
    union {
        unsigned int pointer_raw; // Type 1, memory offset
        unsigned int pointer_blob; // Type 2
        struct { // Type 3
            unsigned int mClientSideBufferName;
            unsigned int mClientSideBufferOffset;
        };
    };

};

class ValueTM
{
public:
    Value_Type_TM   mType;
    std::string     mName;
    // can be string or enum name
    std::string     mStr;   // string
    unsigned int    mId; //used by tracetoc for array and blob id, not saved to file

    union {
        char                    mInt8;
        unsigned char           mUint8;
        short                   mInt16;
        unsigned short          mUint16;
        int                     mInt;
        unsigned int            mUint;
        long long               mInt64;
        unsigned long long      mUint64;
        float                   mFloat;
        double                  mDouble;
        struct {
            unsigned int        mBlobLen;
            char                *mBlob;
        };
        unsigned int            mEnum;
        struct {
            unsigned int        mArrayLen;
            Value_Type_TM       mEleType;
            ValueTM             *mArray;
        };
        struct {
            Opaque_Type_TM      mOpaqueType;
            ValueTM             *mOpaqueIns;
        };
        ValueTM                 *mPointer;
        void                    *mUnusedPointer;
        struct {
            unsigned int        mClientSideBufferName;
            unsigned int        mClientSideBufferOffset;
        };
    };


    ValueTM()
        : mType(Void_Type)
        , mStr()
        , mId(0)
    {}

    ValueTM(int v)
        : mType(Int_Type)
        , mStr()
        , mId(0)
        , mInt(v)
    {}

    ValueTM(unsigned int v)
        : mType(Uint_Type)
        , mStr()
        , mId(0)
        , mUint(v)
    {}

    ValueTM(long long v)
        : mType(Int64_Type)
        , mStr()
        , mId(0)
        , mInt64(v)
    {}

    ValueTM(const char *b, unsigned int bs)
        : mType(Blob_Type)
        , mStr()
        , mId(0)
        , mBlobLen(0)
        , mBlob(NULL)
    {
        if (bs)
        {
            mBlob = new char[bs];
            if (mBlob)
            {
                memcpy(mBlob, b, bs);
                mBlobLen = bs;
            }
        }
    }

    ValueTM(const std::string &v)
        : mType(String_Type)
        , mStr(v)
        , mId(0)
    {}

    ~ValueTM();

    // Clear all created memory
    void Reset();

    // Set to be array type and set the size of
    // the array to be at least the specific length.
    void ResizeArray(unsigned int size);

    // 'maxLen == 0' means no limitation
    std::string ToStr(const CallTM *call, int maxLen=32);
    std::string ToC(const CallTM *call, bool asSourceCode=false);
    std::string TypeNameToStr();
    char* Serialize(char* dest, bool doPadding);

    ValueTM(const ValueTM &other);
    ValueTM &operator =(const ValueTM &other);

    bool IsVoid() const;
    bool IsPointer() const;
    bool IsUnusedPointer() const;
    bool IsIntegral() const;
    bool IsFloatingPoint() const;
    bool IsNumerical() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsBlob() const;
    bool IsBufferReference() const;
    bool IsClientSideBufferReference() const;

    unsigned long long GetAsUInt64() const;
    long long GetAsInt64() const;

    unsigned char GetAsUByte() const;
    void SetAsUByte(unsigned char value);
    unsigned short GetAsUShort() const;
    void SetAsUShort(unsigned short value);
    unsigned int GetAsUInt() const;
    int GetAsInt() const;
    void SetAsUInt(unsigned int value);
    float GetAsFloat() const;
    void SetAsFloat(float value);
    const std::string GetAsString(int maxLen = -1) const;
    void SetAsString(const std::string & value);
    const std::string GetAsBlob() const;
    void SetAsBlob(const std::string & value);
    unsigned int GetAsBufferReference() const;
    void SetAsBufferReference(unsigned int value);
    void GetAsClientSideBufferReference(unsigned int &name, unsigned int &offset);
    void SetAsClientSideBufferReference(unsigned int name, unsigned int offset);

private:
    void CopyFrom(const ValueTM &other);
};

ValueTM* CreateEnumValue(unsigned int value);
ValueTM* CreateUInt8Value(unsigned int value);
ValueTM* CreateInt32Value(int value);
ValueTM* CreateUInt32Value(unsigned int value);
ValueTM* CreateStringValue(const std::string &value);
ValueTM* CreateInt32ArrayValue(const std::vector<int> *values);
ValueTM* CreateUInt32ArrayValue(const std::vector<unsigned int> &values);
ValueTM* CreateStringArrayValue(const std::vector<std::string> &values);
ValueTM* CreateBlobValue(unsigned int data_size, const char *data);
ValueTM* CreateBlobOpaqueValue(unsigned int data_size, const char *data);
ValueTM* CreateBufferReferenceOpaqueValue(unsigned int value);

class CallTM
{
public:
    CallTM()
    : mCallNo(0),
      mTid(0),
      mCallId(0),
      mCallErrNo(CALL_GL_NO_ERROR),
      mBkColor(0xffffffff),
      mTxtColor(0x000000ff)
    {
        mRet.mName = "ret";
    }

    CallTM(const char *name)
    : mCallNo(0),
      mTid(0),
      mCallId(0),
      mCallErrNo(CALL_GL_NO_ERROR),
      mCallName(name),
      mBkColor(0xffffffff),
      mTxtColor(0x000000ff)
    {
        mRet.mName = "ret";

        mCallId = common::gApiInfo.NameToId(name);
    }

    CallTM(InFileRA &infile, unsigned callNo, const BCall_vlen &call);
    CallTM(InFile &infile, unsigned callNo, const BCall_vlen &call);

    ~CallTM() {
        ClearArguments();
    }

    bool Load(InFileRA *infile);
    void ClearArguments(unsigned int from = 0) {
        for (unsigned int i = from; i < mArgs.size(); ++i)
            delete mArgs[i];
        mArgs.resize(from);
    }

    const std::string Name() const { return mCallName; }

    // Properties always there
    std::streamoff          mReadPos;
    unsigned int            mCallNo;
    unsigned int            mTid;
    unsigned int            mCallId; // This ID is not the same with the ID in the binary trace file. It conforms to the ID in api_info.hpp.
    CALL_ERROR_NO           mCallErrNo;
    std::string             mCallName;
    ValueTM                 mRet;
    std::vector<ValueTM*>   mArgs;

    unsigned int            mBkColor;
    unsigned int            mTxtColor;

    bool mInjected = false;

    std::string ToStr(bool isAbbreviate = true);
    char* Serialize(char* dest, int overrideID = -1, bool injected = false);
    void Stylize();

private:
    CallTM(const CallTM &);
    CallTM &operator =(const CallTM &);
};

class FrameTM
{
public:
    FrameTM():
        mIsLoaded(false), mCallCount(0)
    {}

    ~FrameTM() {
        UnloadCalls();
    }

    bool IsLoaded() const { return mIsLoaded; }
    void LoadCalls(InFileRA *infile) { LoadCalls(infile, true, ""); }
    void LoadCalls(InFileRA *infile, bool loadQuery, const std::string &loadFilter);
    void LoadCallsForTraceToTxt(InFileRA *infile, unsigned int callNum, unsigned int fromFirstCall, unsigned int max_cycle) { LoadCallsForTraceToTxt(infile, true, "", callNum, fromFirstCall, max_cycle); }
    void LoadCallsForTraceToTxt(InFileRA *infile, bool loadQuery, const std::string &loadFilter, unsigned int callNum, unsigned int fromFirstcall, unsigned int max_cycle);
    void UnloadCalls();

    // How many calls in the data of this frame, including loaded and non-loaded
    unsigned int GetCallCount() const { return mCallCount; }
    void SetCallCount(unsigned int c) { mCallCount = c; }
    // How many calls are actually loaded into the call list
    unsigned int GetLoadedCallCount() const { return mCalls.size(); }

    // Properties always there
    std::streamoff          mReadPos;
    unsigned int            mFirstCallOfThisFrame;
    unsigned int            mBytes;

    // Properties only exist after been loaded
    std::vector<CallTM*>    mCalls;

private:
    FrameTM(const FrameTM &);
    FrameTM &operator =(const FrameTM &);

    bool                    mIsLoaded;
    unsigned int            mCallCount;
};

class TraceFileTM
{
public:
    TraceFileTM();
    TraceFileTM(const char *name, bool readHeaderAndExit = false);

    ~TraceFileTM();

    bool Open(const char* name, bool readHeaderAndExit = false, const std::string& ra_target = std::string());
    void Close();
    CallTM *NextCall() const;
    CallTM *NextCallInFrame(unsigned int frameIndex) const;

    // return -1 if failed to find the corresponding frame
    int GetFrameIdx(unsigned int callNo);

    unsigned int FindNext(unsigned int callNo, const char* name);
    unsigned int FindPrevious(unsigned int callNo, const char* name);

    bool GetLoadQueryCalls() const { return mLoadQueryCalls; }
    void SetLoadQueryCalls(bool v) { mLoadQueryCalls = v; }
    void SetLoadFilter(const std::string &str) { mLoadFilterStr = str; }
    const std::string & GetLoadFilter() const { return mLoadFilterStr; }

    std::vector<FrameTM*>   mFrames;

    common::InFileRA*       mpInFileRA;

private:
    TraceFileTM(const TraceFileTM &);
    TraceFileTM &operator =(const TraceFileTM &);

    mutable unsigned int mCurFrameIndex;
    mutable unsigned int mCurCallIndexInFrame;

    void Clear();

    bool mLoadQueryCalls;
    std::string mLoadFilterStr;
};

}

#endif
