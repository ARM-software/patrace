#ifndef _STATE_PROGRAM_HPP_
#define _STATE_PROGRAM_HPP_

#include "common.hpp"

namespace pat
{

class ProgramObject
{
public:
    void SetType(unsigned int) {}
};

struct ShaderObject
{
    UInt32 type;
    std::string source;
};
typedef std::shared_ptr<ShaderObject> ShaderObjectPtr;

} // namespace state

#endif // _STATE_PROGRAM_HPP_
