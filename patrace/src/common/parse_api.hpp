#ifndef _COMMON_PARSE_API_HPP_
#define _COMMON_PARSE_API_HPP_

#include <common/api_info.hpp>
#include <common/trace_model.hpp>

namespace common {

class InFileBase;
typedef void (*ParseFunc)(char*, CallTM& callTM, const InFileBase &);

extern const common::EntryMap parse_callbacks;

}

#endif
