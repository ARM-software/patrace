#!/usr/bin/env python2

import os
import re
import xml.etree.ElementTree as ET
import sys

# Parse our spec file
handled = []
fd = open('gles12api.py', 'r')
for line in fd:
    if '#' in line:
        try:
            ext = re.search('# (GL_.*)', line).group(1).strip()
            handled.append(ext)
        except:
            pass
fd.close()
fd = open('eglapi.py', 'r')
for line in fd:
    if '#' in line:
        try:
            ext = re.search('# (GL_.*)', line).group(1).strip()
            handled.append(ext)
        except:
            pass
fd.close()

# Parse gl.xml
script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/opengl-registry/xml/gl.xml'))
root = tree.getroot()
for extension in root.findall('extensions/extension'):
    name = extension.get('name')
    supported = extension.get('supported')
    funcs = []
    for command in extension.findall('require/command'):
        funcs.append(command.get('name'))
    if supported == 'gl':
        continue
    if len(funcs) == 0:
        continue # no functions, not relevant
    if name in handled:
        continue
    if 'GL_ARB' in name: # these are already supported through latest version
        continue
    if not 'GL_OES' in name and not 'GL_EXT' in name and not 'GL_KHR' in name and not 'GL_ARM' in name: # only want official ones
        continue
    print '%s' % name
