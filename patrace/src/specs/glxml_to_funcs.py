#!/usr/bin/env python2

import os
import re
import xml.etree.ElementTree as ET
import gltypes
import sys
from gltypes import *

script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/opengl-registry/xml/gl.xml'))
root = tree.getroot()

commands = {}

# Parse gl.xml
for command in root.find('commands'):
    d = {}
    proto = command.find('proto')
    d['return_type_str'] = ''.join([text for text in proto.itertext()][:-1]).strip()
    d['function_name'] = proto.find('name').text
    p = [{"strlist": [text.strip() for text in param.itertext() if text.strip()],
          "length": param.attrib.get('len')}
         for param in command.findall('param')]

    for param in p:
        splitted = []
        for string in param['strlist']:
            for word in string.split():
                splitted.append(word)
        param['strlist'] = splitted

    d['parameters'] = p

    commands[d['function_name']] = d

gles_versions = [
    'GL_ES_VERSION_3_1',
]

extensions = [
    'KHR_blend_equation_advanced', 'OES_sample_shading', 'OES_sample_variables',
    'OES_shader_image_atomic', 'OES_shader_multisample_interpolation', 'OES_texture_stencil8',
    'OES_texture_storage_multisample_2d_array', 'EXT_copy_image', 'EXT_draw_buffers_indexed',
    'EXT_geometry_shader', 'EXT_gpu_shader5', 'EXT_primitive_bounding_box', 'EXT_shader_io_blocks',
    'EXT_tessellation_shader', 'EXT_texture_border_clamp', 'EXT_texture_buffer', 
    'EXT_texture_cube_map_array', 'EXT_texture_sRGB_decode', 'KHR_debug', 'KHR_texture_compression_astc_ldr',
    'AMD_performance_monitor'
]

all_commands = {}
for version in gles_versions:
    # <feature api="gles2" name="GL_ES_VERSION_3_1" number="3.1">
    for command in root.findall("feature[@name='{v}']/require/command".format(v=version)):
        command_name = command.get('name')
        all_commands[command_name] = commands[command_name]

for extension in extensions:
    # <extension name="GL_3DFX_multisample" supported="gl">
    for command in root.findall("extensions/extension[@name='GL_{v}']/require/command".format(v=extension)):
        command_name = command.get('name')
        all_commands[command_name] = commands[command_name]

def GlFunction(*args, **kwargs):
    kwargs.setdefault('call', 'GL_APIENTRY')
    return gltypes.Function(*args, **kwargs)

def print_prototypes():
    # Sort alphabetical on function name
    sorted_commands = [command for name, command in all_commands.iteritems()]
    sorted_commands.sort(key=lambda c: c['function_name'])

    print '#ifndef _EXT_FUNC_HACK_H'
    print '#define _EXT_FUNC_HACK_H'
    print
    print 'typedef void (*GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);'
    print 'typedef void (*GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);'
    for command in sorted_commands:
        param = []
        for p in command['parameters']:
            param.append('%s' % ' '.join(p['strlist']))
        command['param_string'] = ', '.join(param)
        command['function_name_upper'] = command['function_name'].upper()
        print
        print('typedef {return_type_str} (GLAPIENTRY *PA_PFN{function_name_upper}PROC)({param_string});'.format(**command))
        print('extern PA_PFN{function_name_upper}PROC pa_{function_name};'.format(**command))
        print('#define {function_name} pa_{function_name}'.format(**command))
    print
    print '#endif'

def print_functions():
    # Sort alphabetical on function name
    sorted_commands = [command for name, command in all_commands.iteritems()]
    sorted_commands.sort(key=lambda c: c['function_name'])

    print '#include "pa_demo.h"'
    print '#include "pa_gl31.h"'
    for command in sorted_commands:
        param = [' '.join(p['strlist']) for p in command['parameters']]
        call_list = [p['strlist'][-1] for p in command['parameters']]
        command['param_string'] = ', '.join(param)
        command['call_list'] = ', '.join(call_list)
        command['function_name_upper'] = command['function_name'].upper()
        function_template = \
'''
static {return_type_str} dummy_{function_name}({param_string})
{{
    pa_{function_name} = (PA_PFN{function_name_upper}PROC)eglGetProcAddress("{function_name}");
    return pa_{function_name}({call_list});
}}
PA_PFN{function_name_upper}PROC pa_{function_name} = dummy_{function_name};
'''
        print(function_template.format(**command))

if __name__ == '__main__':

    file_path = os.path.join(script_dir, 'pa_gl31.h')
    orig_stdout = sys.stdout
    with open(file_path, 'w') as f:
        sys.stdout = f
        print_prototypes()

    file_path = os.path.join(script_dir, 'pa_gl31.cpp')
    with open(file_path, 'w') as f:
        sys.stdout = f
        print_functions()

    sys.stdout = orig_stdout
