%module patrace
%include "std_string.i"
%include "std_vector.i"
%include "typemaps.i"
%{
#define SWIG_FILE_WITH_INIT
#include <common/trace_model.hpp>
#include <common/parse_api.hpp>
#include <common/out_file.hpp>

// disable specific warnings for SWIG-generated codes
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
%}

%template(ValueVector) std::vector<common::ValueTM*>;
%template(UIntVector) std::vector<unsigned int>;
%template(StringVector) std::vector<std::string>;

%pythoncode %{
class PyPATraceException(Exception):

    def __init__(self, value_type):
        self.value_type = value_type

class ClientSideBufferObjectReference(object):

    def __init__(self, name, offset):
        self.name = name
        self.offset = offset

    def __repr__(self):
        return 'ClientSideBufferObjectReference(%d, 0x%X)' % (self.name, self.offset)
%}
namespace common
{

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
};

enum Opaque_Type_TM {
    BufferObjectReferenceType = 0,
    BlobType,
    ClientSideBufferObjectReferenceType,
    NoopType,
};

%rename(Value) ValueTM;
class ValueTM
{
public:
    ValueTM();
    ~ValueTM();

    ValueTM(int);
    ValueTM(long long);
    ValueTM(const std::string &v);

    Value_Type_TM   mType;
    std::string     mName;
    Opaque_Type_TM  mOpaqueType;

    // set as void type
    void Reset();
    void ResizeArray(unsigned int size);

    bool IsVoid() const;
    bool IsPointer() const;
    bool IsIntegral() const;
    bool IsFloatingPoint() const;
    bool IsNumerical() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsBlob() const;
    bool IsBufferReference() const;
    bool IsClientSideBufferReference() const;

    unsigned char GetAsUByte() const;
    void SetAsUByte(unsigned char value);
    unsigned short GetAsUShort() const;
    void SetAsUShort(unsigned short value);
    unsigned int GetAsUInt() const;
    void SetAsUInt(unsigned int value);
    float GetAsFloat() const;
    void SetAsFloat(float value);
    const std::string GetAsString() const;
    void SetAsString(const std::string & value);
    const std::string GetAsBlob() const;
    void SetAsBlob(const std::string & value);
    unsigned int GetAsBufferReference() const;
    void SetAsBufferReference(unsigned int value);

    void GetAsClientSideBufferReference(unsigned int & OUTPUT, unsigned int & OUTPUT);

    void SetAsClientSideBufferReference(unsigned int name, unsigned int offset);
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

%extend ValueTM {

    short _getAsByte() const
    {
        assert($self->IsIntegral());
        return $self->mInt8;
    }
    void _setAsByte(char value)
    {
        $self->Reset();
        $self->mType = common::Int8_Type;
        $self->mInt8 = value;
    }

    short _getAsShort() const
    {
        assert($self->IsIntegral());
        return $self->mInt16;
    }
    void _setAsShort(short value)
    {
        $self->Reset();
        $self->mType = common::Int16_Type;
        $self->mInt16 = value;
    }

    int _getAsInt() const
    {
        assert($self->IsIntegral());
        return $self->mInt;
    }
    void _setAsInt(int value)
    {
        $self->Reset();
        $self->mType = common::Int_Type;
        $self->mInt = value;
    }

    long long _getAsLongLong() const
    {
        assert($self->IsIntegral());
        return $self->mInt64;
    }
    void _setAsLongLong(long long value)
    {
        $self->Reset();
        $self->mType = common::Int64_Type;
        $self->mInt64 = value;
    }
    unsigned long long _getAsULongLong() const
    {
        assert($self->IsIntegral());
        return $self->mUint64;
    }
    void _setAsULongLong(unsigned long long value)
    {
        $self->Reset();
        $self->mType = common::Uint64_Type;
        $self->mUint64 = value;
    }

    unsigned int _getArrayLength() const
    {
        assert($self->IsArray());
        return $self->mArrayLen;
    }
    void _setArrayItem(unsigned int index, ValueTM *v)
    {
        $self->ResizeArray(index + 1);
        $self->mArray[index] = *v;
        $self->mEleType = v->mType;
    }
    ValueTM *_getArrayItem(unsigned int index)
    {
        if (index >= $self->mArrayLen)
            $self->ResizeArray(index + 1);
        return &($self->mArray[index]);
    }

    %pythoncode %{
        __swig_getmethods__["asByte"] = _getAsByte
        __swig_setmethods__["asByte"] = _setAsByte
        if _newclass: asByte = property(_getAsByte, _setAsByte)
        __swig_getmethods__["asUByte"] = GetAsUByte
        __swig_setmethods__["asUByte"] = SetAsUByte
        if _newclass: asUByte = property(GetAsUByte, SetAsUByte)

        __swig_getmethods__["asShort"] = _getAsShort
        __swig_setmethods__["asShort"] = _setAsShort
        if _newclass: asShort = property(_getAsShort, _setAsShort)
        __swig_getmethods__["asUShort"] = GetAsUShort
        __swig_setmethods__["asUShort"] = SetAsUShort
        if _newclass: asUShort = property(GetAsUShort, SetAsUShort)

        __swig_getmethods__["asInt"] = _getAsInt
        __swig_setmethods__["asInt"] = _setAsInt
        if _newclass: asInt = property(_getAsInt, _setAsInt)
        __swig_getmethods__["asUInt"] = GetAsUInt
        __swig_setmethods__["asUInt"] = SetAsUInt
        if _newclass: asUInt = property(GetAsUInt, SetAsUInt)

        __swig_getmethods__["asLongLong"] = _getAsLongLong
        __swig_setmethods__["asLongLong"] = _setAsLongLong
        if _newclass: asLongLong = property(_getAsLongLong, _setAsLongLong)
        __swig_getmethods__["asULongLong"] = _getAsULongLong
        __swig_setmethods__["asULongLong"] = _setAsULongLong
        if _newclass: asULongLong = property(_getAsULongLong, _setAsULongLong)

        __swig_getmethods__["asFloat"] = GetAsFloat
        __swig_setmethods__["asFloat"] = SetAsFloat
        if _newclass: asFloat = property(GetAsFloat, SetAsFloat)

        __swig_getmethods__["asString"] = GetAsString
        __swig_setmethods__["asString"] = SetAsString
        if _newclass: asString = property(GetAsString, SetAsString)

        __swig_getmethods__["asBlob"] = GetAsBlob
        __swig_setmethods__["asBlob"] = SetAsBlob
        if _newclass: asBlob = property(GetAsBlob, SetAsBlob)

        __swig_getmethods__["asBufferReference"] = GetAsBufferReference
        __swig_setmethods__["asBufferReference"] = SetAsBufferReference
        if _newclass: asBufferReference = property(GetAsBufferReference, SetAsBufferReference)

        __swig_getmethods__["asClientSideBufferReference"] = GetAsClientSideBufferReference
        __swig_setmethods__["asClientSideBufferReference"] = SetAsClientSideBufferReference
        if _newclass: asClientSideBufferReference = property(GetAsClientSideBufferReference, SetAsClientSideBufferReference)

        def __setitem__(self, index, value):
            self._setArrayItem(index, value)
        def __getitem__(self, index):
            return self._getArrayItem(index)
        def __len__(self):
            return self._getArrayLength();
        def __iter__(self):
            for index in range(len(self)):
                yield self._getArrayItem(index)

        def GetValue(self):
            if self.mType == Int8_Type:
                return self.asByte
            elif self.mType == Uint8_Type:
                return self.asUByte
            elif self.mType == Int16_Type:
                return self.asShort
            elif self.mType == Uint16_Type:
                return self.asUShort
            elif self.mType == Int_Type:
                return self.asInt
            elif self.mType == Uint_Type or self.mType == Enum_Type:
                return self.asUInt
            elif self.mType == Int64_Type:
                return self.asLongLong
            elif self.mType == Uint64_Type:
                return self.asULongLong
            elif self.mType == Float_Type:
                return self.asFloat
            elif self.IsBufferReference():
                return self.asBufferReference
            elif self.IsString():
                return self.asString
            elif self.IsBlob():
                return self.asBlob
            elif self.IsVoid():
                return None
            elif self.IsPointer():
                return self.asUInt
            elif self.IsArray():
                ret = []
                for index in range(len(self)):
                    ret.append(self[index].GetValue())
                return tuple(ret)
            elif self.IsClientSideBufferReference():
                name, offset = self.asClientSideBufferReference
                return ClientSideBufferObjectReference(name, offset)
            else:
                print 'Unexpected value type : %d' % self.mType
                return None

        def SetValue(self, value):
            if self.mType == Int8_Type:
                self.asByte = value
            elif self.mType == Uint8_Type:
                self.asUByte = value
            elif self.mType == Int16_Type:
                self.asShort = value
            elif self.mType == Uint16_Type:
                self.asUShort = value
            elif self.mType == Int_Type:
                self.asInt = value
            elif self.mType == Uint_Type or self.mType == Enum_Type or self.mType == Pointer_Type:
                self.asUInt = value
            elif self.mType == Int64_Type:
                self.asLongLong = value
            elif self.mType == Uint64_Type:
                self.asULongLong = value
            elif self.mType == Float_Type:
                self.asFloat = value
            elif self.IsString():
                self.asString = value
            elif self.IsBlob():
                self.asBlob = value
            elif self.IsVoid():
                pass
            elif self.IsArray():
                self.ResizeArray(len(value))
                for index in range(len(value)):
                    self[index].SetValue(value[index])
            elif self.IsClientSideBufferReference():
                self.asClientSideBufferReference = value.name, value.offset
            else:
                print 'Unexpected value type : %d' % self.mType
                raise PyPATraceException(self.mType)
    %}

};

%rename(Call) CallTM;
class CallTM
{
public:
    CallTM();
    CallTM(const char *name);
    ~CallTM();

    std::string ToStr(bool isAbbreviate = true);

    %rename(return_value) mRet;
    ValueTM mRet;

    %rename(args) mArgs;
    std::vector<ValueTM*>   mArgs;
};

%extend CallTM {

    const std::string _getName() const
    {
        return $self->mCallName;
    }
    void _setName(const std::string &name)
    {
        $self->mCallName = name;
        $self->mCallId = common::gApiInfo.NameToId(name.c_str());
    }

    unsigned int _getNumber() const
    {
        return $self->mCallNo;
    }
    void _setNumber(unsigned int no)
    {
        $self->mCallNo = no;
    }

    unsigned int _getThreadID() const
    {
        return $self->mTid;
    }
    void _setThreadID(unsigned int tid)
    {
        $self->mTid = tid;
    }

    unsigned int _getArgCount() const
    {
        return $self->mArgs.size();
    }
    void _setArgCount(unsigned int count)
    {
        const unsigned int oldCount = $self->mArgs.size();
        if (count > oldCount)
        {
            $self->mArgs.resize(count);
            for (unsigned int i = oldCount; i < count; ++i)
                $self->mArgs[i] = new common::ValueTM;
        }
        else if (count < oldCount)
        {
            for (unsigned int i = count; i < oldCount; ++i)
                delete $self->mArgs[i];
            $self->mArgs.resize(count);
        }
    }

    %pythoncode %{
        def __repr__(self):
            return self.ToStr(False)

        __swig_getmethods__["name"] = _getName
        __swig_setmethods__["name"] = _setName
        if _newclass: name = property(_getName, _setName)

        __swig_getmethods__["number"] = _getNumber
        __swig_setmethods__["number"] = _setNumber
        if _newclass: number = property(_getNumber, _setNumber)

        __swig_getmethods__["thread_id"] = _getThreadID
        __swig_setmethods__["thread_id"] = _setThreadID
        if _newclass: thread_id = property(_getThreadID, _setThreadID)

        __swig_getmethods__["argCount"] = _getArgCount
        __swig_setmethods__["argCount"] = _setArgCount
        if _newclass: argCount = property(_getArgCount, _setArgCount)

        def GetArgument(self, index):
            return self.args[index].GetValue()

        def SetArgument(self, index, value):
            self.args[index].SetValue(value)

        def GetArguments(self):
            args = []
            for arg in self.args:
                try:
                    args.append(arg.GetValue())
                except PyPATraceException as e:
                    print('[%d]%s : Unsupported argument type 0x%X' % (self.number, self.name, e.value_type))
                    raise
            return args

        def GetArgumentsDict(self):
            args = {}
            for arg in self.args:
                try:
                    args[arg.mName] = arg.GetValue()
                except PyPATraceException as e:
                    print('[%d]%s : Unsupported argument type 0x%X' % (self.number, self.name, e.value_type))
                    raise
            return args

        def GetReturnValue(self):
            return self.return_value.GetValue()
    %}
};

%rename(InputFile) TraceFileTM;
class TraceFileTM
{
public:
    TraceFileTM();
    TraceFileTM(const char *name, bool readHeaderAndExit = false);
    ~TraceFileTM();

    %rename(_next_call) NextCall;
    CallTM *NextCall() const;
    %rename(_next_call_in_frame) NextCallInFrame;
    CallTM *NextCallInFrame(unsigned int frameIndex) const;
};

%extend TraceFileTM {
    bool Open(const char *name, bool readHeaderAndExit = false)
    {
        common::gApiInfo.RegisterEntries(common::parse_callbacks);
        return $self->Open(name, readHeaderAndExit);
    }

    void Close()
    {
        $self->mpInFileRA->Close();
    }

    const std::string _getJSONHeader() const
    {
        return $self->mpInFileRA->getJSONHeaderAsString();
    }

    const unsigned int _getVersion() const
    {
        return $self->mpInFileRA->getHeaderVersion() - common::HEADER_VERSION_1 + 1;
    }

    const unsigned int _getFrameCount() const
    {
        return $self->mFrames.size();
    }

    const unsigned int FrameCallCount(unsigned int frameIndex) const
    {
        return $self->mFrames[frameIndex]->GetCallCount();
    }

    %pythoncode %{

        def __enter__(self):
            return self

        def __exit__(self, type, value, traceback):
            self.Close()

        def Calls(self):
            c = self._next_call()
            while c:
                yield c
                c = self._next_call()

        def FrameCalls(self, frameIndex):
            c = self._next_call_in_frame(frameIndex)
            while c:
                yield c
                c = self._next_call_in_frame(frameIndex)

        __swig_getmethods__["jsonHeader"] = _getJSONHeader
        if _newclass: jsonHeader = property(_getJSONHeader)

        __swig_getmethods__["version"] = _getVersion
        if _newclass: version = property(_getVersion)

        __swig_getmethods__["frameCount"] = _getFrameCount
        if _newclass: frameCount = property(_getFrameCount)
    %}
};

%rename(OutputFile) OutFile;
class OutFile
{
public:
    OutFile();
    OutFile(const char *name);
    ~OutFile();

    bool Open(const char *name);
    void Close();
};

%extend OutFile {

    void WriteCall(CallTM *call)
    {
        const unsigned int WRITE_BUF_LEN = 150*1024*1024;
        static char buffer[WRITE_BUF_LEN];
        char *dest = buffer;
        dest = call->Serialize(dest);
        $self->Write(buffer, dest-buffer);
    }

    void _setJsonHeader(const std::string &str)
    {
        $self->mHeader.jsonLength = str.size();
        $self->WriteHeader(str.c_str(), str.size());
    }

    unsigned int _getVersion() const

    {
        return $self->mHeader.version - 2;
    }

    %pythoncode %{
        def __enter__(self):
            return self

        def __exit__(self, type, value, traceback):
            self.Close()

        __swig_setmethods__["jsonHeader"] = _setJsonHeader
        if _newclass: jsonHeader = property(fset=_setJsonHeader)

        __swig_getmethods__["version"] = _getVersion
        if _newclass: version = property(_getVersion)
    %}
};

}
