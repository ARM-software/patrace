#!/usr/bin/env python

import argparse
import os
import json
import subprocess
import copy
import getpass
import datetime
import sys
import regex

glslang = '/usr/local/bin/glslangValidator'

# Try to upgrade shader to modern version
def rewrite_shader(filename, newfilename, vertex):
	code = ''
	version = '310'
	must_add_out = False
	with open(filename) as fp:
		for line in fp:
			if '#version' in line:
				if '320' in line: version = '320'
				continue

			if vertex: line = line.replace('varying ', 'out ')
			else: line = line.replace('varying ', 'in ')

			line = line.replace('attribute ', 'in ')
			def repl(x):
				return 'texture('
			line = regex.sub('texture.+?\(', repl, line)

			if 'gl_FragColor' in line:
				must_add_out = True
				line = line.replace('gl_FragColor', 'myFragColor')

			if len(line.strip()) > 0:
				code = code + line
	if must_add_out:
		code = 'out vec4 myFragColor;\n' + code
	code = '#version ' + version + ' es\n' + code
	print 'Writing %s' % newfilename
	with open(newfilename, 'w') as fp:
		fp.write(code)

def renderpass_directory(name):
	jsonname = name + "/renderpass.json"
	with open(jsonname, "r") as fp:
		if not fp:
			print 'Failed to open %s' % jsonname
			return False
		data = json.load(fp)

	for x in data['resources']['programs']:
		newentry = copy.deepcopy(x['shaders'])
		for k,v in x['shaders'].iteritems():
			if not '_glsl_preprocessed' in k: continue
			print '%s : %s' % (k, v)
			newfilename = v + '.spv'
			last = v.rindex('.')
			ext = v[last:]
			tmpfilename = v[:last] + '.rewrite' + ext
			rewrite_shader(name + '/' + v, name + '/' + tmpfilename, '.vert' in v)
			cmd = [ glslang, '--aml', '--amb', '-r', '-G', '-g', '-o', name + "/" + newfilename, name + "/" + tmpfilename ]
			print cmd
			retval = subprocess.call(cmd)
			if retval != 0: sys.exit(retval)
			newkey = k.replace('_glsl_preprocessed', '_spirv')
			newentry[newkey] = newfilename
		x['shaders'] = newentry

	ourselves = {}
	ourselves['tool'] = 'spirvify'
	ourselves['user'] = getpass.getuser()
	ourselves['time'] = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M%:%S')
	data['content_info']['generation'].append(ourselves)

	with open(jsonname, "w") as fp:
		fp.write(json.dumps(data, sort_keys=True, indent=4))

def parse_args():
	ap = argparse.ArgumentParser(description='Add SPIRV variants of GLSL shaders to renderpass JSON')
	ap.add_argument('directory', help='Name of renderpass directory to SPIRVify')
	return ap.parse_args()

def main():
	args = parse_args()
	renderpass_directory(args.directory)

if __name__ == '__main__':
	main()
