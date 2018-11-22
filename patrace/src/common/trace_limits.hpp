#ifndef _TRACE_LIMITS_H
#define _TRACE_LIMITS_H

namespace common
{
    /// Legacy thread limit used by old header formats
    const static int MAX_RETRACE_THREADS = 8;
}

/// Maximum number of threads
const static int PATRACE_THREAD_LIMIT = 255;

#endif
