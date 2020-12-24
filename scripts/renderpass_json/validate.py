#!/usr/bin/python

import argparse
import zipfile
import os
import json

# Python 2/3 compatibility hack
try:
	basestring
except NameError:
	basestring = str

def one_of(data, key, mylist):
	assert key in data, 'key %s is missing' % key
	assert isinstance(data[key], basestring), 'key %s is wrong type' % key
	assert data[key] in mylist, 'key %s value %s is not in allowed list' % (key, data[key])

def has(data, key, type):
	assert key in data, 'key %s is missing' % key
	assert isinstance(data[key], type), 'key %s is wrong type' % key

def may(data, key, type):
	if key in data:
		assert isinstance(data[key], type), 'key %s is wrong type' % key

def filename(dir, data):
	has(data, 'filename', basestring)
	filename = '%s/%s' % (dir, data['filename'])
	assert os.path.exists(filename), '%s does not exist!' % filename

def validate_renderpass_directory(name):
	gles = False
	vulkan = False
	jsonname = name + "/renderpass.json"
	fp = open(jsonname, "r")
	if not fp:
		print 'Failed to open %s' % jsonname
		return False
	data = json.load(fp)

	has(data, 'content_info', dict)
	has(data, 'renderpass', dict)
	has(data, 'resources', dict)
	has(data, 'statevectors', dict)
	has(data['content_info'], 'content_name', basestring)
	has(data['content_info'], 'content_frame', int)
        for x in data['content_info']['generation']:
		has(x, 'gpu', basestring)
		has(x, 'user', basestring)
		has(x, 'time', basestring)
		one_of(x, 'tool', ['analyze_trace'])
	one_of(data['content_info'], 'source_api', ['GLES', 'VULKAN'])
	may(data['content_info'], 'source_api_version', float)
	has(data, 'renderdump_version', float)
	has(data['renderpass'], 'commands', list)
	has(data['resources'], 'geometry', list)
	has(data['resources'], 'programs', list)
	for x in data['renderpass']['commands']:
		has(x, 'blend_state', int)
		has(x, 'depthstencil_state', int)
		has(x, 'geometry', int)
		has(x, 'indexed', bool)
		has(x, 'program', int)
		has(x, 'rasterization_state', int)
		has(x, 'scissor_state', int)
		has(x, 'viewport_state', int)
		has(x, 'metadata', dict) # free form
		has(x, 'draw_params', dict)
		has(x['draw_params'], 'element_count', int)
		may(x['draw_params'], 'instance_count', int)
		may(x['draw_params'], 'first_instance', int)
		may(x['draw_params'], 'base_vertex', int)
		one_of(x, 'topology', ['TRIANGLES', 'POINTS', 'TRIANGLE_FAN', 'LINES', 'TRIANGLE_STRIP', 'LINE_LOOP', 'LINE_STRIP'])
	for x in data['resources']['geometry']:
		if 'index_buffer' in x:
			has(x['index_buffer'], 'index_byte_width', int)
			filename(name, x['index_buffer'])
		for y in x.get('uniforms', []):
			has(y, 'value', list)
			has(y, 'location', int)
			has(y, 'size', int)
			has(y, 'name', basestring)
			has(y, 'type', basestring) # TBD, check against list of allowed types
		for y in x.get('vertex_buffers', []):
			has(y, 'components', int)
			has(y, 'slot', int)
			has(y, 'stride', int)
			has(y, 'binding', basestring)
			one_of(y, 'type', ['FLOAT', 'INT8', 'UINT8', 'INT16', 'UINT16', 'INT32', 'UINT32', 'HALF', 'FIXED', 'INT_2_10_10_10_REV', 'UNSIGNED_INT_2_10_10_10_REV'])
			filename(name, y)
	for x in data['resources']['programs']:
		has(x, 'shaders', dict)
		if gles: has(x['shaders'], 'fragment_glsl', basestring)
		if gles: has(x['shaders'], 'vertex_glsl', basestring)
	for x in data['statevectors'].get('blend_states', []):
		has(x, 'blend_factor', list)
		assert len(x['blend_factor']) == 4
	for x in data['statevectors'].get('depthstencil_states', []):
		has(x, 'back_face', dict)
		has(x, 'front_face', dict)
		has(x, 'depth_enable', bool)
		has(x, 'stencil_enable', bool)
		has(x, 'depth_writes', bool)
		one_of(x, 'depth_function', ['NEVER', 'LESS', 'EQUAL', 'LEQUAL', 'GREATER', 'NOTEQUAL', 'GEQUAL', 'ALWAYS'])
		for f in ['back_face', 'front_face']:
			face = x[f]
			one_of(face, 'function', ['NEVER', 'LESS', 'EQUAL', 'LEQUAL', 'GREATER', 'NOTEQUAL', 'GEQUAL', 'ALWAYS'])
			has(face, 'compare_mask', int)
			has(face, 'reference', int)
			has(face, 'write_mask', int)
	for x in data['statevectors'].get('rasterization_states', []):
		one_of(x, 'cull_mode', ['NONE', 'BACK', 'FRONT', 'FRONT_AND_BACK'])

	return True

def parse_args():
	ap = argparse.ArgumentParser(description='Validate a renderpass JSON directory')
	ap.add_argument('directory', help='Name of renderpass directory to validate')
	return ap.parse_args()

def main():
	args = parse_args()
	validate_renderpass_directory(args.directory)
	print "Validation of {} OK".format(args.directory)

if __name__ == '__main__':
	main()
