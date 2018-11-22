#include <exception>

#include "tool/trace_interface.hpp"

#include "common/in_file.hpp"
#include "common/file_format.hpp"
#include "common/os.hpp"
#include "common/out_file.hpp"
#include "common/api_info.hpp"
#include "common/parse_api.hpp"
#include "common/trace_model.hpp"

using namespace common;

class PATCall : public CallInterface
{
public:
    PATCall(common::CallTM *call = NULL) : _call(call), _owned(false)
    {
        if (!_call)
        {
            _call = new common::CallTM;
            _owned = true;
        }
    }

    ~PATCall()
    {
        if (_owned)
        {
            delete _call;
        }
    }

    virtual const char *GetName() const
    {
        return _call->mCallName.c_str();
    }
    virtual void SetName(const char *name)
    {
        _call->mCallName = name;
        _call->mCallId = common::gApiInfo.NameToId(name);
    }

    virtual unsigned int GetNumber() const
    {
        return _call->mCallNo;
    }
    virtual void SetNumber(unsigned int no)
    {
        _call->mCallNo = no;
    }

    virtual UInt32 GetThreadID() const
    {
        return _call->mTid;
    }
    virtual void SetThreadID(UInt32 tid)
    {
        _call->mTid = tid;
    }

    virtual UInt32 ret_to_uint() const
    {
        return _call->mRet.mUint;
    }

    virtual UInt32 arg_num() const
    {
        return _call->mArgs.size();
    }

    virtual bool arg_to_bool(UInt32 index) const
    {
        const common::ValueTM *arg = _call->mArgs[index];
        if (arg)
        {
            if (arg->mType == common::Pointer_Type)
                return arg->mPointer;
            else if (arg->mType == common::Array_Type)
                return arg->mArrayLen;
        }
        return false;
    }

    virtual UInt32 arg_to_uint(UInt32 index) const
    {
        return _call->mArgs[index]->mUint;
    }

    virtual UInt8* arg_to_pointer(UInt32 index, bool* bufferObject) const
    {
        const ValueTM *arg = _call->mArgs[index];
        if (arg->mType == Blob_Type)
        {
            return (UInt8*)(arg->mBlob);
        }
        else if (arg->mType == Opaque_Type && arg->mOpaqueType == BlobType)
        {
            return (UInt8*)(arg->mOpaqueIns->mBlob);
        }
        else if (arg->mType == Opaque_Type
                 && arg->mOpaqueType == BufferObjectReferenceType
                 && arg->mOpaqueIns->mType == Uint_Type)
        {
            if (bufferObject)
            {
                *bufferObject = true;
            }
            return reinterpret_cast<UInt8*>(arg->mOpaqueIns->mUint);
        }
        else
        {
            assert(false); // should not get here
            return nullptr;
        }
    }

    virtual UInt32 array_size(UInt32 index) const
    {
        if(_call->mArgs[index]->mType != common::Array_Type)
            return 0;
        return _call->mArgs[index]->mArrayLen;
    }

    virtual UInt32 array_arg_to_uint(UInt32 index, UInt32 array_index) const
    {
        return _call->mArgs[index]->mArray[array_index].mUint;
    }

    virtual const char* array_arg_to_str(UInt32 index, UInt32 array_index) const
    {
        return _call->mArgs[index]->mArray[array_index].mStr.c_str();
    }

    virtual bool arg_is_client_side_buffer(UInt32 index) const
    {
        return _call->mArgs[index]->mType == common::Opaque_Type &&
               _call->mArgs[index]->mOpaqueType == common::ClientSideBufferObjectReferenceType;
    }

    virtual UInt32 arg_to_client_side_buffer(UInt32 index) const
    {
        return _call->mArgs[index]->mOpaqueIns->mClientSideBufferName;
    }

    virtual bool set_signature(const char *fun_name)
    {
        _call->mCallId = common::gApiInfo.NameToId(fun_name);
        return true;
    }

    virtual bool clear_args(UInt32 num)
    {
        _call->ClearArguments(num);
        return true;
    }

    virtual void push_back_null_arg()
    {
        common::ValueTM *arg = new common::ValueTM;
        arg->mType = common::Pointer_Type;
        arg->mPointer = NULL;
        _call->mArgs.push_back(arg);
    }

    virtual void push_back_sint_arg(SInt32 value)
    {
        _call->mArgs.push_back(new common::ValueTM(value));
    }

    virtual void push_back_enum_arg(SInt32 value)
    {
        _call->mArgs.push_back(new common::ValueTM(value));
    }

    virtual void push_back_str_arg(const std::string &str)
    {
        common::ValueTM *arg = new common::ValueTM;
        arg->mType = common::String_Type;
        arg->mStr = str;
        _call->mArgs.push_back(arg);
    }

    virtual void push_back_pointer_arg(UInt32 size, const UInt8 *value)
    {
        if (size && value)
        {
            UInt8 *buffer = new UInt8[size];
            memcpy(buffer, value, size);
            _call->mArgs.push_back(new common::ValueTM((char*)buffer, size));
        }
        else
        {
            _call->mArgs.push_back(new common::ValueTM(0, 0));
        }
    }

    virtual void push_back_array_arg(UInt32 size)
    {
        common::ValueTM *arg = new common::ValueTM;
        arg->mType = common::Array_Type;
        arg->mArrayLen = size;
        arg->mEleType = common::Void_Type;
        arg->mArray = new common::ValueTM[size];
        _call->mArgs.push_back(arg);
    }

    virtual void set_array_item(UInt32 arg_index, UInt32 array_index, const std::string &str)
    {
        common::ValueTM *arg = _call->mArgs[arg_index];
        arg->mEleType = common::String_Type;
        arg->mArray[array_index].mType = common::String_Type;
        arg->mArray[array_index].mStr = str;
    }

    common::CallTM *_call;
    bool _owned; // should delete the internal call if owned it
};

class PATInputFile : public InputFileInterface
{
public:
    PATInputFile()
    : _fileTM(new TraceFileTM),
      _filepath(NULL), _curFrame(NULL),
      _curFrameIndex(0), _curCallIndexInFrame(0)
    {
    }
    ~PATInputFile()
    {
        close();
        delete _fileTM;
    }

    virtual bool open(const char *filepath)
    {
        common::gApiInfo.RegisterEntries(common::parse_callbacks);
        _filepath = filepath;
        if(!_fileTM->Open(_filepath, false))
            return false;
        _curFrame = _fileTM->mFrames[_curFrameIndex];
        _curFrame->LoadCalls(_fileTM->mpInFileRA);
        return true;
    }

    virtual bool close()
    {
        return true;
    }

    virtual bool reset()
    {
        _curFrameIndex = 0;
        if (_curFrame) _curFrame->UnloadCalls();
        _curFrame = _fileTM->mFrames[_curFrameIndex];
        _curFrame->LoadCalls(_fileTM->mpInFileRA);
        _curCallIndexInFrame = 0;
        return true;
    }

    virtual CallInterface * next_call() const
    {
        if (_curCallIndexInFrame >= _curFrame->GetLoadedCallCount())
        {
            _curFrameIndex++;
            if (_curFrameIndex >= _fileTM->mFrames.size())
                return NULL;
            if (_curFrame) _curFrame->UnloadCalls();
            _curFrame = _fileTM->mFrames[_curFrameIndex];
            _curFrame->LoadCalls(_fileTM->mpInFileRA);
            _curCallIndexInFrame = 0;
        }
        common::CallTM *call = _curFrame->mCalls[_curCallIndexInFrame];
        _curCallIndexInFrame++;
        if (call)
            return new PATCall(call);
        else
            return NULL;
    }

    virtual unsigned int header_version() const
    {
        return _fileTM->mpInFileRA->getHeaderVersion() - common::HEADER_VERSION_1 + 1;
    }

    virtual const std::string json_header() const
    {
        return _fileTM->mpInFileRA->getJSONHeaderAsString();
    }

private:
    common::TraceFileTM *_fileTM;
    const char *_filepath;
    mutable common::FrameTM *_curFrame;
    mutable UInt32 _curFrameIndex;
    mutable UInt32 _curCallIndexInFrame;
};

class PATOutputFile : public OutputFileInterface
{
public:
    PATOutputFile() : _outfile(new common::OutFile)
    {
    }
    ~PATOutputFile()
    {
        close();
        delete _outfile;
    }

    virtual bool open(const char *filepath)
    {
        return _outfile->Open(filepath);
    }

    virtual void write_json_header(const std::string &content)
    {
        _outfile->mHeader.jsonLength = content.size();
        _outfile->WriteHeader(content.c_str(), content.size());
    }

    virtual bool close()
    {
        _outfile->Close();
        return true;
    }

    virtual bool write(CallInterface *call)
    {
        const unsigned int WRITE_BUF_LEN = 150*1024*1024;
        static char buffer[WRITE_BUF_LEN];

        PATCall *pCall = dynamic_cast<PATCall*>(call);
        if (pCall)
        {
            char *dest = buffer;
            dest = pCall->_call->Serialize(dest);
            _outfile->Write(buffer, dest-buffer);
        }

        return true;
    }

private:
    common::OutFile *_outfile;
};

CallInterface *GenerateCall()
{
    return new PATCall;
}

InputFileInterface *GenerateInputFile()
{
    return new PATInputFile;
}

OutputFileInterface *GenerateOutputFile()
{
    return new PATOutputFile;
}
