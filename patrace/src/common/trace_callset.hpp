/**************************************************************************
 *
 * Copyright 2012 VMware, Inc.
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
 * Representation of call sets.
 *
 * Grammar:
 *
 *     set = '@' filename
 *         | range ( ',' ? range ) *
 *
 *     range = interval ( '/' frequency )
 *
 *     interval = '*'
 *              | number
 *              | start_number '-' end_number
 *
 *     frequency = divisor
 *               | "frame"
 *               | "rendertarget" | "fbo"
 *               | "render | "draw"
 *
 */

#ifndef _TRACE_CALLSET_HPP_
#define _TRACE_CALLSET_HPP_

#include <cstring> // for strcmp
#include <list>

namespace common {


    // Should match Call::no
    typedef unsigned CallNo;


    // Aliases for call flags
    enum CallFlags {
        FREQUENCY_NONE         = 1 << 0,
        FREQUENCY_FRAME        = 1 << 1,
        FREQUENCY_RENDERTARGET = 1 << 2,
        FREQUENCY_RENDER       = 1 << 3,
        FREQUENCY_ALL          = 0xffffffff,
    };

    inline CallFlags GetCallFlags(const char *funcName)
    {
        if (strncmp(funcName, "eglSwapBuffers", strlen("eglSwapBuffers")) == 0)
            return FREQUENCY_FRAME;
        else if (strcmp(funcName, "glBindFramebuffer") == 0)
            return FREQUENCY_RENDERTARGET;
        else if (strcmp(funcName, "glDrawElements") == 0 ||
                 strcmp(funcName, "glDrawArrays") == 0 ||
                 strcmp(funcName, "glDrawArraysInstanced") == 0 ||
                 strcmp(funcName, "glDrawElementsInstanced") == 0 ||
                 strcmp(funcName, "glDrawArraysIndirect") == 0 ||
                 strcmp(funcName, "glDrawElementsIndirect") == 0 ||
                 strcmp(funcName, "glDrawRangeElements") == 0 ||
                 strcmp(funcName, "glBlitFramebuffer") == 0)
            return FREQUENCY_RENDER;
        else
            return FREQUENCY_NONE;
    }

    // A linear range of calls
    class CallRange
    {
    public:
        CallNo start;
        CallNo stop;
        CallNo step;
        CallFlags freq;

        CallRange(CallNo callNo) :
            start(callNo),
            stop(callNo),
            step(1),
            freq(FREQUENCY_ALL)
        {}

        CallRange(CallNo _start, CallNo _stop, CallNo _step = 1, CallFlags _freq = FREQUENCY_ALL) :
            start(_start),
            stop(_stop),
            step(_step),
            freq(_freq)
        {}

        bool contains(CallNo callNo, const char *funcName) const {
            if (callNo >= start && callNo <= stop &&
                ((callNo - start) % step) == 0)
            {
                if (freq == FREQUENCY_ALL)
                    return true;
                else
                {
                    CallFlags flag = GetCallFlags(funcName);
                    return (flag & freq) != 0;
                }
            }

            return false;
        }
    };


    // A collection of call ranges
    class CallSet
    {
    public:
        CallSet() {}

        CallSet(CallFlags freq);

        CallSet(const char *str);

        // Not empty set
        inline bool
        empty() const {
            return ranges.empty();
        }

        void
        addRange(const CallRange & range) {
            if (range.start <= range.stop &&
                range.freq != FREQUENCY_NONE) {

                RangeList::iterator it = ranges.begin();
                while (it != ranges.end() && it->start < range.start) {
                    ++it;
                }

                ranges.insert(it, range);
            }
        }

        inline bool
        contains(CallNo callNo, const char *funcName) const {
            if (empty()) {
                return false;
            }
            RangeList::const_iterator it;
            for (it = ranges.begin(); it != ranges.end() && it->start <= callNo; ++it) {
                if (it->contains(callNo, funcName)) {
                    return true;
                }
            }
            return false;
        }

    private:
        // TODO: use binary tree to speed up lookups
        typedef std::list< CallRange > RangeList;
        RangeList ranges;
    };

    CallSet parse(const char *string);

} /* namespace trace */


#endif /* _TRACE_CALLSET_HPP_ */
