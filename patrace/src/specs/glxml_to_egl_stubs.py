#!/usr/bin/env python3

from __future__ import print_function
import os
import xml.etree.ElementTree as ET
from specs import gltypes
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/egl-registry/api/egl.xml'))
root = tree.getroot()

commands = {}

# Parse egl.xml
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

egl_versions = [
    'EGL_VERSION_1_0',
    'EGL_VERSION_1_1',
    'EGL_VERSION_1_2',
    'EGL_VERSION_1_3',
    'EGL_VERSION_1_4',
    'EGL_VERSION_1_5'
]

extensions = [
]

all_commands = {}
for version in egl_versions:
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


def print_gl_functions():
    # Sort alphabetical on function name
    sorted_commands = [command for name, command in all_commands.items()]
    sorted_commands.sort(key=lambda c: c['function_name'])

    print('#include <EGL/egl.h>')
    print()
    print('#define EXPORT extern "C" __attribute__ ((visibility ("default")))')
    print()
    for command in sorted_commands:
        param = [' '.join(p['strlist']) for p in command['parameters']]
        call_list = [p['strlist'][-1] for p in command['parameters']]
        command['param_string'] = ', '.join(param)
        command['call_list'] = ', '.join(call_list)
        command['function_name_upper'] = command['function_name'].upper()
        function_template = \
            '''
EXPORT {return_type_str} {function_name}({param_string});
{return_type_str} {function_name}({param_string})
{{
    return ({return_type_str})0;
}}
'''
        print(function_template.format(**command))


if __name__ == '__main__':
    orig_stdout = sys.stdout

    file_path = os.path.join(script_dir, 'stub_egl.cpp')
    with open(file_path, 'w') as f:
        sys.stdout = f
        print_gl_functions()

    sys.stdout = orig_stdout
