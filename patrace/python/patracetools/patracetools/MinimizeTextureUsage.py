#!/usr/bin/env python2
import os, sys, json

try:
    from patrace import InputFile, OutputFile, Call
except ImportError:
    print 'patrace (Python interface of PATrace SDK) is required.'
    sys.exit()

try:
    import OpenGL.GL as GL
except ImportError:
    print 'PyOpenGL (the Python OpenGL binding) is required;'
    print 'You could install it with "pip install PyOpenGL".'
    sys.exit()

# This script convert an input trace into another trace ('output.pat') which minimizes the texture bandwidth.

# It creates a 1x1 2D texture (with name 1000) and another 1x1 cubemap texture (with name 1001);
# So it assumes that the count of textures created by the original trace is less than 1000;
# Then for each drawcall, it forces every texture unit (up to 8) to just use the trivial textures.

hasCreatedTextures = False

def OutputCall(output, call, tid):

    global hasCreatedTextures
    # create dummy textues
    if call.name == 'glBindTexture' and not hasCreatedTextures:
        c = Call('glBindTexture')
        c.tid = tid
        c.argCount = 2
        c.args[0].asUInt = GL.GL_TEXTURE_2D
        c.args[1].asUInt = 1000
        output.WriteCall(c)

        c = Call('glTexImage2D')
        c.tid = tid
        c.argCount = 9
        c.args[0].asUInt = GL.GL_TEXTURE_2D
        c.args[1].asUInt = 0
        c.args[2].asUInt = GL.GL_RGBA
        c.args[3].asUInt = 1
        c.args[4].asUInt = 1
        c.args[5].asUInt = 0
        c.args[6].asUInt = GL.GL_RGBA
        c.args[7].asUInt = GL.GL_UNSIGNED_BYTE
        c.args[8].asString = '\0xFF\0xFF\0xFF\0xFF'
        output.WriteCall(c)

        c = Call('glBindTexture')
        c.tid = tid
        c.argCount = 2
        c.args[0].asUInt = GL.GL_TEXTURE_CUBE_MAP
        c.args[1].asUInt = 1001
        output.WriteCall(c)

        for target in range(GL.GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL.GL_TEXTURE_CUBE_MAP_NEGATIVE_Z + 1):
            c = Call('glTexImage2D')
            c.tid = tid
            c.argCount = 9
            c.args[0].asUInt = target
            c.args[1].asUInt = 0
            c.args[2].asUInt = GL.GL_RGBA
            c.args[3].asUInt = 1
            c.args[4].asUInt = 1
            c.args[5].asUInt = 0
            c.args[6].asUInt = GL.GL_RGBA
            c.args[7].asUInt = GL.GL_UNSIGNED_BYTE
            c.args[8].asString = '\0xFF\0xFF\0xFF\0xFF'
            output.WriteCall(c)

        hasCreatedTextures = True

    if call.name.startswith('glDraw'):
        for unit in range(GL.GL_TEXTURE0, GL.GL_TEXTURE0 + 8):
            c = Call('glActiveTexture')
            c.tid = tid
            c.argCount = 1
            c.args[0].asUInt = unit
            output.WriteCall(c)

            c = Call('glBindTexture')
            c.tid = tid
            c.argCount = 2
            c.args[0].asUInt = GL.GL_TEXTURE_2D
            c.args[1].asUInt = 1000
            output.WriteCall(c)

            c = Call('glBindTexture')
            c.tid = tid
            c.argCount = 2
            c.args[0].asUInt = GL.GL_TEXTURE_CUBE_MAP
            c.args[1].asUInt = 1001
            output.WriteCall(c)

    output.WriteCall(call)


def main():
    trace_filepath = sys.argv[1]
    if not os.path.exists(trace_filepath):
        print("File does not exists: {}".format(trace_filepath))
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
