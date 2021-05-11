#!/usr/bin/env python3

from __future__ import print_function
import os
import re
import xml.etree.ElementTree as ET
import collections

script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/opengl-registry/xml/gl.xml'))
root = tree.getroot()

commands = collections.OrderedDict()
gles_versions = {
    'GL_ES_VERSION_3_0' : 30,
    'GL_ES_VERSION_3_1' : 31,
    'GL_ES_VERSION_3_2' : 32,
}
funcs_to_versions = collections.OrderedDict()
funcs_to_extension = collections.defaultdict(list)

all_commands = collections.OrderedDict()
for version in list(gles_versions.keys()):
    for command in root.findall("feature[@name='{v}']/require/command".format(v=version)):
        command_name = command.get('name')
        if command_name in funcs_to_versions:
            print('%s already in %s -- cannot add for %s!' % (command_name, funcs_to_versions[command_name], version))
            continue
        funcs_to_versions[command_name] = gles_versions[version]

for extension in root.findall('extensions/extension'):
    name = extension.get('name')
    supported = extension.get('supported')
    funcs = []
    if not 'gles' in supported:
        continue
    for require in extension.findall('require'):
        ext = require.get('api', None)
        if ext and not 'gles' in ext:
            continue
        comment = require.get('comment', None)
        for command in require.findall('command'):
            command_name = command.get('name')
            if comment and comment == "KHR extensions *mandate* suffixes for ES, unlike for GL":
                command_name += 'KHR'
            funcs_to_extension[command_name].append(name)

file_path = os.path.join(script_dir, 'pa_func_to_version.h')
with open(file_path, 'w') as f:
    print('#pragma once', file=f)
    print(file=f)
    print('#include <string>', file=f)
    print('#include <unordered_map>', file=f)
    print(file=f)
    print('extern std::unordered_map<std::string, int> map_func_to_version;', file=f)
    print('extern std::unordered_multimap<std::string, std::string> map_func_to_extension;', file=f)
file_path = os.path.join(script_dir, 'pa_func_to_version.cpp')
with open(file_path, 'w') as f:
    print('#include "pa_func_to_version.h"', file=f)
    print(file=f)
    print('std::unordered_map<std::string, int> map_func_to_version = {', file=f)
    for k,v in funcs_to_versions.items():
        print('\t{"%s", %.1f},' % (k, v), file=f)
    print('};', file=f)
    print(file=f)
    print('std::unordered_multimap<std::string, std::string> map_func_to_extension = {', file=f)
    for k,v in funcs_to_extension.items():
        for i in v:
            print('\t{"%s", "%s"},' % (k, i), file=f)
    print('};', file=f)
