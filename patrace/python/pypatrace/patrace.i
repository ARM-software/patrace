%module patrace
%include <std_string.i>
%include <std_vector.i>
%include <typemaps.i>
%{
#define SWIG_FILE_WITH_INIT
#include "common/trace_model.hpp"
#include "common/parse_api.hpp"
#include "common/out_file.hpp"

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
    Unused_Pointer_Type,
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
    bool IsUnusedPointer() const;
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
    %extend {

        ValueTM* _getAsPointer() const
        {
            return $self->mPointer;
        }

        void _setAsPointer(ValueTM* value)
        {
            $self->mPointer = value;
        }

        long long _getAsUnusedPointer() const
        {
            return reinterpret_cast<long long>($self->mUnusedPointer);
        }

        void _setAsUnusedPointer(long long value)
        {
            $self->mUnusedPointer = reinterpret_cast<void*>(value);
        }
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
                    return self.asBlob.encode('utf-8', errors='surrogateescape')
                elif self.IsVoid():
                    return None
                elif self.IsPointer():
                    return self.asPointer
                elif self.IsUnusedPointer():
                    return self.asUnusedPointer
                elif self.IsArray():
                    ret = []
                    for index in range(len(self)):
                        ret.append(self[index].GetValue())
                    return tuple(ret)
                elif self.IsClientSideBufferReference():
                    name, offset = self.asClientSideBufferReference
                    return ClientSideBufferObjectReference(name, offset)
                elif self.mType == Opaque_Type and self.mOpaqueType == NoopType:
                    return 'Ignored'
                else:
                    print('Unexpected value type : %d' % self.mType)
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
                elif self.mType == Uint_Type or self.mType == Enum_Type:
                    self.asUInt = value
                elif self.mType == Pointer_Type:
                    self.asPointer = value
                elif self.mType == Unused_Pointer_Type:
                    self.asUnusedPointer = value
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
                    print('Unexpected value type : %d' % self.mType)
                    raise PyPATraceException(self.mType)

            asPointer = property(_getAsPointer, _setAsPointer)
            asUnusedPointer = property(_getAsUnusedPointer, _setAsUnusedPointer)
            asByte = property(_getAsByte, _setAsByte)
            asUByte = property(GetAsUByte, SetAsUByte)
            asShort = property(_getAsShort, _setAsShort)
            asUShort = property(GetAsUShort, SetAsUShort)
            asInt = property(_getAsInt, _setAsInt)
            asUInt = property(GetAsUInt, SetAsUInt)
            asLongLong = property(_getAsLongLong, _setAsLongLong)
            asULongLong = property(_getAsULongLong, _setAsULongLong)
            asFloat = property(GetAsFloat, SetAsFloat)
            asString = property(GetAsString, SetAsString)
            asBlob = property(GetAsBlob, SetAsBlob)
            asBufferReference = property(GetAsBufferReference, SetAsBufferReference)
            asClientSideBufferReference = property(GetAsClientSideBufferReference, SetAsClientSideBufferReference)
        %}
    }
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
    %extend {

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

            name = property(_getName, _setName)
            number = property(_getNumber, _setNumber)
            thread_id = property(_getThreadID, _setThreadID)
            argCount = property(_getArgCount, _setArgCount)
        %}
    }
};

%rename(InputFile) TraceFileTM;
class TraceFileTM
{
public:
    TraceFileTM(size_t callBatchSize = SIZE_MAX);
    TraceFileTM(const char *name, bool readHeaderAndExit = false, size_t callBatchSize = SIZE_MAX);
    ~TraceFileTM();

    %rename(_next_call) NextCall;
    CallTM *NextCall() const;
    %rename(_next_call_in_frame) NextCallInFrame;
    CallTM *NextCallInFrame(unsigned int frameIndex) const;

    %extend {
        bool Open(const char *name, bool readHeaderAndExit = false)
        {
            common::gApiInfo.RegisterEntries(common::parse_callbacks);
            return $self->Open(name, readHeaderAndExit);
        }

        void Close()
        {
            $self->mpInFileRA->Close();
        }

        const std::string _getJSONHeader() const {
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

            jsonHeader = property(_getJSONHeader)
            version = property(_getVersion)
            frameCount = property(_getFrameCount)
        %}
    }
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

    %extend {
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

            jsonHeader = property(None, _setJsonHeader)
            version = property(_getVersion)
        %}
    }
};

} // namespace common
