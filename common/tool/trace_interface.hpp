#ifndef _COMMON_TOOL_TRACE_INTERFACE_HPP_
#define _COMMON_TOOL_TRACE_INTERFACE_HPP_

#include "base/base.hpp"

class CallInterface
{
public:
    virtual ~CallInterface() {}

    virtual const char *GetName() const = 0;
    virtual void SetName(const char *) = 0;
    virtual unsigned int GetNumber() const = 0;
    virtual void SetNumber(unsigned int) = 0;
    virtual unsigned int GetThreadID() const = 0;
    virtual void SetThreadID(unsigned int tid) = 0;

    virtual UInt32 ret_to_uint() const = 0;

    virtual UInt32 arg_num() const = 0;

    virtual bool arg_to_bool(UInt32 index) const = 0;
    virtual UInt32 arg_to_uint(UInt32 index) const = 0;
    virtual UInt8 *arg_to_pointer(UInt32 index, bool* bufferObject) const = 0;
    virtual UInt32 array_size(UInt32 index) const = 0;
    virtual UInt32 array_arg_to_uint(UInt32 index, UInt32 array_index) const = 0;
    virtual const char *array_arg_to_str(UInt32 index, UInt32 array_index) const = 0;

    virtual bool arg_is_client_side_buffer(UInt32 index) const = 0;
    virtual UInt32 arg_to_client_side_buffer(UInt32 index) const = 0;

    virtual bool set_signature(const char *fun_name) = 0;
    // resize the arguments to specific num and don't touch the remaining arguments
    virtual bool clear_args(UInt32 num) = 0;
    virtual void push_back_null_arg() = 0;
    virtual void push_back_sint_arg(SInt32 value) = 0;
    virtual void push_back_enum_arg(SInt32 value) = 0;
    virtual void push_back_str_arg(const std::string &str) = 0;
    virtual void push_back_pointer_arg(UInt32 size, const UInt8 *value) = 0;
    virtual void push_back_array_arg(UInt32 size) = 0;

    virtual void set_array_item(UInt32 arg_index, UInt32 array_index,
        const std::string &str) = 0;
};

class InputFileInterface
{
public:
    virtual ~InputFileInterface() {}

    virtual bool open(const char *) = 0;
    virtual bool close() = 0;
    virtual bool reset() = 0;
    virtual CallInterface * next_call() const = 0;

    virtual unsigned int header_version() const { return 0; }
    // For JSON header, which is introduced just for
    // version 3 or newer of PATrace
    virtual const std::string json_header() const { return std::string(); }
};

class OutputFileInterface
{
public:
    virtual ~OutputFileInterface() {}

    virtual bool open(const char *) = 0;
    virtual bool close() = 0;
    virtual bool write(CallInterface *) = 0;
    virtual void write_json_header(const std::string &content) {}
};

CallInterface *GenerateCall();
InputFileInterface *GenerateInputFile();
OutputFileInterface *GenerateOutputFile();

#endif
