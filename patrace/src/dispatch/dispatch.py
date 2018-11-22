##########################################################################
#
# Copyright 2010 VMware, Inc.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
##########################################################################/


"""Generate DLL/SO dispatching functions.
"""

# Adjust path
import os.path
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import specs.stdapi as stdapi

# Map commonly used extension functions to core functions.
synonyms = {
    'glTexSubImage1DOES' : 'glTexSubImage1D',
    'glTexSubImage2DOES' : 'glTexSubImage2D',
    'glTexSubImage3DOES' : 'glTexSubImage3D',
    'glTexStorage1DEXT'  : 'glTexStorage1D',
    'glTexStorage2DEXT'  : 'glTexStorage2D',
    'glTexStorage3DEXT'  : 'glTexStorage3D',
    'glDebugMessageCallbackKHR' : 'glDebugMessageCallback',
    'glDebugMessageControlKHR' : 'glDebugMessageControl',
    'glDebugMessageInsertKHR' : 'glDebugMessageInsert',
    'glPushDebugGroupKHR' : 'glPushDebugGroup',
    'glPopDebugGroupKHR' : 'glPopDebugGroup',
    'glObjectLabelKHR' : 'glObjectLabel',
    'glGetObjectLabelKHR' : 'glGetObjectLabel',
    'glObjectPtrLabelKHR' : 'glObjectPtrLabel',
    'glGetDebugMessageLogKHR' : 'glGetDebugMessageLog',
    'glBindVertexArrayOES' : 'glBindVertexArray',
    'glGenVertexArraysOES' : 'glGenVertexArrays',
    'glDeleteVertexArraysOES' : 'glDeleteVertexArrays',
    'glIsVertexArrayOES' : 'glIsVertexArray',
    'glBlendEquationSeparateOES' : 'glBlendEquationSeparate',
    'glBlendFuncSeparateOES' : 'glBlendFuncSeparate',
    'glBlendEquationOES' : 'glBlendEquation',
    'glInsertEventMarkerEXT' : 'glInsertEventMarker',
    'glPushGroupMarkerEXT' : 'glPushGroupMarker',
    'glPopGroupMarkerEXT' : 'glPopGroupMarker',
    'glBlendBarrierKHR' : 'glBlendBarrier',
    'glMinSampleShadingOES' : 'glMinSampleShading',
    'glPrimitiveBoundingBoxEXT' : 'glPrimitiveBoundingBox',
    'glDrawElementsBaseVertexOES' : 'glDrawElementsBaseVertex',
    'glDrawElementsInstancedBaseVertexOES' : 'glDrawElementsInstancedBaseVertex',
    'glMultiDrawElementsBaseVertexOES' : 'glMultiDrawElementsBaseVertex',
    'glCopyImageSubDataOES' : 'glCopyImageSubData',
    'glBlendEquationEXT' : 'glBlendEquation',
    'glReadnPixelsEXT' : 'glReadnPixels',
}

# Map commonly used core functions to extensions, in case core
# version not high enough but functions supported through extensions.
inverse_mapping = dict((v, k) for k, v in synonyms.items())

def function_pointer_type(function):
    return 'PFN_' + function.name.upper()


def function_pointer_value(function):
    return '_' + function.name + '_ptr'


class Dispatcher:

    def dispatchApi(self, api):
        for function in api.functions:
            self.dispatchFunction(api, function)

        # define standard name aliases for convenience, but only when not
        # tracing, as that would cause symbol clashing with the tracing
        # functions
        print '#ifdef RETRACE'
        for function in api.functions:
            print '#define %s _%s' % (function.name, function.name)
        print '#endif /* RETRACE */'
        print

    def dispatchFunction(self, api, function):
        ptype = function_pointer_type(function)
        pvalue = function_pointer_value(function)
        print 'typedef ' + function.prototype('* %s' % ptype) + ';'
        print 'extern %s %s;' % (ptype, pvalue)
        print
        print 'static inline ' + function.prototype('_' + function.name) + ' {'
        print '    const char *_name = "%s";' % function.name
        if function.type is stdapi.Void:
            ret = ''
        else:
            ret = 'return '
        self.invokeGetProcAddress(api, function)
        print '    %s%s(%s);' % (ret, pvalue, ', '.join([str(arg.name) for arg in function.args]))
        print '}'
        print

    def defineFptrs(self, api):
        for function in api.functions:
            ptype = function_pointer_type(function)
            pvalue = function_pointer_value(function)
            print '%s %s = NULL;' % (ptype, pvalue)

    def getProcAddressName(self, api, function):
        return '_getProcAddress'

    def invokeGetProcAddress(self, api, function):
        ptype = function_pointer_type(function)
        pvalue = function_pointer_value(function)
        getProcAddressName = self.getProcAddressName(api, function)
        print '    if (!%s) {' % (pvalue,)
        print '        static bool has_warned = false;'
        print '        bool has_warned_cached = has_warned;'
        print '        %s = (%s)%s(_name);' % (pvalue, ptype, getProcAddressName)
        print '        if (!%s) {' % (pvalue,)
        if function.name in synonyms: # try different version of same function
            print '            const char *_name2 = "%s";' % synonyms[function.name]
            print r'            if (!has_warned_cached) DBG_LOG("Warning: %s unavailable - trying %s\n", _name, _name2);'
            print '            has_warned = true;'
            print '            %s = (%s)%s(_name2);' % (pvalue, ptype, getProcAddressName)
            print '        }'
            print '        if (!%s) {' % (pvalue,)
        if function.name in inverse_mapping: # try different version of same function, inverse of above
            print '            const char *_name2 = "%s";' % inverse_mapping[function.name]
            print r'            if (!has_warned_cached) DBG_LOG("Warning: %s unavailable - trying %s\n", _name, _name2);'
            print '            has_warned = true;'
            print '            %s = (%s)%s(_name2);' % (pvalue, ptype, getProcAddressName)
            print '        }'
            print '        if (!%s) {' % (pvalue,)
        print r'            if (!has_warned_cached) DBG_LOG("Warning: Ignoring call to unavailable function %s\n", _name);'
        print '            has_warned = true;'
        if function.type is not stdapi.Void:
            print '            %s ret = 0;' % function.type

        if function.type is stdapi.Void:
            print '            return;'
        else:
            print '            return ret;'
        print '        }'
        print '    }'
