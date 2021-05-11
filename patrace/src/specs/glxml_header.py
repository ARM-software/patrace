#!/usr/bin/env python3

from __future__ import print_function

import os
import xml.etree.ElementTree as ET
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from gltypes import *

script_dir = os.path.dirname(os.path.realpath(__file__))
tree = ET.parse(os.path.join(script_dir, '../../../thirdparty/opengl-registry/xml/gl.xml'))
root = tree.getroot()

file_path = os.path.join(script_dir, 'khronos_enums.hpp')
with open(file_path, 'w') as f:
    orig_stdout = sys.stdout
    sys.stdout = f

    print('#ifndef H_ALL_GL_ENUMS')
    print('#define H_ALL_GL_ENUMS')
    print()

    for l in root.findall('enums'):
        print('// %s : %s : %s : %s' % (l.get('namespace'), l.get('group'), l.get('vendor'), l.get('comment')))
        for e in l.findall('enum'):
            if not (e.get('name') == 'GL_ACTIVE_PROGRAM_EXT' and e.get('api') == 'gl'): # GL registry bug
                print('#define PA_%s %s' % (e.get('name'), e.get('value')))
        print()
    print()
    print('#endif')

    sys.stdout = orig_stdout
