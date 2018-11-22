#!/usr/bin/env python2

import os
import re
import xml.etree.ElementTree as ET
import gltypes
import sys
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
for version in gles_versions.keys():
    for command in root.findall("feature[@name='{v}']/require/command".format(v=version)):
        command_name = command.get('name')
        if command_name in funcs_to_versions:
            print '%s already in %s -- cannot add for %s!' % (command_name, funcs_to_versions[command_name], version)
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
    print >> f, '#pragma once'
    print >> f
    print >> f, '#include <string>'
    print >> f, '#include <unordered_map>'
    print >> f
    print >> f, 'extern std::unordered_map<std::string, int> map_func_to_version;'
    print >> f, 'extern std::unordered_multimap<std::string, std::string> map_func_to_extension;'
file_path = os.path.join(script_dir, 'pa_func_to_version.cpp')
with open(file_path, 'w') as f:
    print >> f, '#include "pa_func_to_version.h"'
    print >> f
    print >> f, 'std::unordered_map<std::string, int> map_func_to_version = {'
    for k,v in funcs_to_versions.iteritems():
        print >> f, '\t{"%s", %.1f},' % (k, v)
    print >> f, '};'
    print >> f
    print >> f, 'std::unordered_multimap<std::string, std::string> map_func_to_extension = {'
    for k,v in funcs_to_extension.iteritems():
        for i in v:
            print >> f, '\t{"%s", "%s"},' % (k, i)
    print >> f, '};'
