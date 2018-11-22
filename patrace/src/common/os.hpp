/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Simple OS abstraction layer.
 */

#ifndef _OS_HPP_
#define _OS_HPP_

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif /* !_WIN32 */

#ifdef __GCC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       x
#define unlikely(x)     x
#endif

#if defined __GCC__ || defined __clang__
#define NORETURN __attribute__ ((noreturn))
#else
#define NORETURN
#endif

namespace os {

void log(const char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format (printf, 1, 2)))
#endif
;

#ifdef ANDROID
void setAndroidAppName(const char* n);
#endif

#if defined _WIN32 || defined __CYGWIN__
  #define SHORT_FILE (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
  /* We always use .def files on windows for now */
  #if 0
  #define PUBLIC __declspec(dllexport)
  #else
  #define PUBLIC
  #endif
  #define PRIVATE
#else
  #define SHORT_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #if __GNUC__ >= 4
    #define PUBLIC __attribute__ ((visibility("default")))
    #define PRIVATE __attribute__ ((visibility("hidden")))
  #else
    #define PUBLIC
    #define PRIVATE
  #endif
#endif

// More convenient debug support
#define DBG_LOG(format, ...) do { os::log("%s,%d: " format, SHORT_FILE, __LINE__, ##__VA_ARGS__); } while(0)

void abort(void);

void setExceptionCallback(void (*callback)(void));
void resetExceptionCallback(void);

} /* namespace os */

// Hack to workaround strange missing support for std::to_string in Android
#ifdef __ANDROID__
#include <string>
#include <sstream>

template <typename T>
std::string _to_string(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}
#else
#define _to_string(_x) std::to_string(_x)
#endif

#endif /* _OS_HPP_ */
