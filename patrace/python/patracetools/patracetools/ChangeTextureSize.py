import os, sys, json

from patrace import InputFile, OutputFile

try:
    import OpenGL.GL as GL
except ImportError:
    print 'PyOpenGL (the Python OpenGL binding) is required;'
    print 'You could install it with "pip install PyOpenGL".'
    sys.exit()

# This script just modifies the dimension of a 2D texture with specified names and create a new trace ('output.pat')

# Usage: python ChangeTextureSize.py input_trace

active_texture_unit = GL.GL_TEXTURE0
bound_2d_texture_object = {
    GL.GL_TEXTURE0          : 0,
    GL.GL_TEXTURE1          : 0,
    GL.GL_TEXTURE2          : 0,
    GL.GL_TEXTURE3          : 0,
    GL.GL_TEXTURE4          : 0,
    GL.GL_TEXTURE5          : 0,
    GL.GL_TEXTURE6          : 0,
    GL.GL_TEXTURE7          : 0,
}

# new dimensions for textures
new_sizes = {
    1 : (1280, 752),
    2 : (1280, 752),
}

def OutputCall(output, call, tid):

    global active_texture_unit, bound_2d_texture_object, new_sizes

    if call.name == 'glActiveTexture':
        active_texture_unit = call.args[0].asUInt
    elif call.name == 'glBindTexture':
        target = call.args[0].asUInt
        texture = call.args[1].asUInt
        if target == GL.GL_TEXTURE_2D:
            bound_2d_texture_object[active_texture_unit] = texture
    elif call.name == 'glTexImage2D':
        target = call.args[0].asUInt
        texname = bound_2d_texture_object[active_texture_unit]
        if target == GL.GL_TEXTURE_2D and texname in new_sizes:
            oldwidth = call.args[3].asUInt
            oldheight = call.args[4].asUInt
            width, height = new_sizes[texname]
            call.args[3].asUInt = width
            call.args[4].asUInt = height
            print('Log : Call no.{5} for texture object {0}, resize from {1}x{2} to {3}x{4}'.format(texname, oldwidth, oldheight, width, height, call.number))

    output.WriteCall(call)


def main():
    trace_filepath = sys.argv[1]
    if not os.path.exists(trace_filepath):
        print("File does not exists: {0}".format(trace_filepath))
        sys.exit(1)

    with InputFile(trace_filepath) as input:
        header = json.loads(input.jsonHeader)
        if 'defaultTid' in header:
            thread_id = header['defaultTid']

        with OutputFile('output.pat') as output:
            output.jsonHeader = input.jsonHeader

            for call in input.Calls():
                if call.thread_id == thread_id:
                    OutputCall(output, call, thread_id)
                else:
                    output.WriteCall(call)


if __name__ == '__main__':
    main()
