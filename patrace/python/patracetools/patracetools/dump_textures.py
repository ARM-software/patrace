#!/usr/bin/env python3
# Author: Joakim Simonsson

import argparse
import os
import OpenGL.GL as GL
import OpenGL
from struct import pack
from math import ceil
from PIL import Image
from patrace import (
    InputFile,
    Call,
)


class Arg:
    def __init__(self, type, name, value):
        self.type = type
        self.name = name
        self.value = value

    def get(self):
        arg = self.type(self.value)
        if self.name:
            arg.mName = self.name
        return arg


class Function:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def write(self, output, tid):
        call = Call(self.name)
        call.thread_id = tid

        for arg in self.args[1:]:
            call.args.push_back(arg.get())

        call.return_value = self.args[0].get()

        output.WriteCall(call)


class Buffer:
    def __init__(self, name, data=b''):
        self.name = name
        self.data = data


class Thread:
    def __init__(self, id):
        self.id = id
        self.current_context = 0
        self.current_surface_draw = 0
        self.current_surface_read = 0


class Context:
    def __init__(self):
        # currently bound buffers, target as key, and value as name
        none_buffer = Buffer(0, b'')

        self.bound_buffers = {
            GL.GL_ARRAY_BUFFER: none_buffer,
            GL.GL_ATOMIC_COUNTER_BUFFER: none_buffer,
            GL.GL_COPY_READ_BUFFER: none_buffer,
            GL.GL_COPY_WRITE_BUFFER: none_buffer,
            GL.GL_DRAW_INDIRECT_BUFFER: none_buffer,
            GL.GL_DISPATCH_INDIRECT_BUFFER: none_buffer,
            GL.GL_ELEMENT_ARRAY_BUFFER: none_buffer,
            GL.GL_PIXEL_PACK_BUFFER: none_buffer,
            GL.GL_PIXEL_UNPACK_BUFFER: none_buffer,
            GL.GL_SHADER_STORAGE_BUFFER: none_buffer,
            GL.GL_TRANSFORM_FEEDBACK_BUFFER: none_buffer,
            GL.GL_UNIFORM_BUFFER: none_buffer,
        }

        self.bound_textures = {
            GL.GL_TEXTURE_2D: 0,
        }

        self.pixelstore = {
            GL.GL_UNPACK_ROW_LENGTH: 0,
            GL.GL_UNPACK_SKIP_ROWS: 0,
            GL.GL_UNPACK_SKIP_PIXELS: 0,
            GL.GL_UNPACK_ALIGNMENT: 4,
            GL.GL_UNPACK_IMAGE_HEIGHT: 0,
            GL.GL_UNPACK_SKIP_IMAGES: 0,
        }

        self.client_side_buffers = {}


class GLSize:
    format = {
        GL.GL_ALPHA: 1,
        GL.GL_RGB: 3,
        GL.GL_RGBA: 4,
    }

    type = {
        GL.GL_UNSIGNED_BYTE: 1,
        GL.GL_UNSIGNED_SHORT: 2,
        GL.GL_UNSIGNED_INT: 4,
    }


class TextureExtractor:
    def __init__(self, filename):
        self.filename = filename
        self.enumvals = [getattr(GL, key) for key in dir(GL) if type(getattr(GL, key)) == OpenGL.constant.IntConstant]
        # Maps gl enum value to string
        self.int_to_gl = dict([(e.real, e.name) for e in self.enumvals])
        # Parameter names that will be displayed with their GL enum
        # string
        self.glenums = [
            'target', 'format', 'type', 'internalformat',
        ]
        self.dataparams = [
            'data', 'pixels',
        ]

        self.dirname = 'textures_{}'.format(self.filename[:-4])

        self.glformat_to_imagemode = {
            GL.GL_RGBA: 'RGBA',
            GL.GL_RGB: 'RGB',
            GL.GL_ALPHA: 'L',
        }

        self.texfunc_to_enum = {
            'glTexImage2D': GL.GL_TEXTURE_2D,
            'glTexSubImage2D': GL.GL_TEXTURE_2D,
        }

        self.contexts = {}

    def write_image(self, mode, width, height, data, filename):
        if len(data) == 0:
            print("Empty data. Not writing", filename)
        print("Writing {}...\r".format(filename))
        img = Image.frombytes(mode, (width, height), data)
        img.save(filename, 'PNG')

    def call_to_string(self, call):
        d = call.GetArgumentsDict()
        for k, v in d.items():
            if k in self.glenums:
                d[k] = self.int_to_gl[v]

        for dp in self.dataparams:
            if dp in d:
                d[dp] = '{BLOB}'

        args = ', '.join(['{}={}'.format(arg, val) for arg, val in d.items()])
        return "@{num}: {funcname}({args})".format(
            num=call.number,
            funcname=call.name,
            args=args,
        )

    def glTexImage2D(self, call, thread, ctx):
        self.texImage2D(call, thread, ctx)

    def glTexSubImage2D(self, call, thread, ctx):
        self.texImage2D(call, thread, ctx)

    def write_texture_data(self, call, d, ctx, image_path, imgmode):
        if len(d['pixels']) == 0:
            return

        if d['type'] == GL.GL_UNSIGNED_BYTE:
            if d['format'] not in list(self.glformat_to_imagemode.keys()):
                print("Unhandled format: " + self.call_to_string(call))

            self.write_image(imgmode, d['width'], d['height'], d['pixels'], image_path)
        else:
            print("Unhandled type: " + self.call_to_string(call))

    def write_buffer_data(self, call, d, ctx, image_path, imgmode):
        offset = d['pixels']
        elementsize = GLSize.type[d['type']]
        texelsize = GLSize.format[d['format']] * elementsize
        if ctx.pixelstore[GL.GL_UNPACK_ROW_LENGTH] == 0:
            rowsize = d['width'] * texelsize
        else:
            a = ctx.pixelstore[GL.GL_UNPACK_ROW_LENGTH]
            if elementsize < a:
                rowsize = a / elementsize * int(ceil(float(texelsize) * d['width'] / a))
            else:
                rowsize = GLSize.format[d['format']] * d['width']

        if d['type'] == GL.GL_UNSIGNED_BYTE:
            data = ctx.bound_buffers[GL.GL_PIXEL_UNPACK_BUFFER].data
            subdata = b''
            for row_i in range(d['height']):
                rowoffset = row_i * rowsize
                subdata += data[offset + rowoffset:offset + d['width'] * texelsize + rowoffset]

            self.write_image(imgmode, d['width'], d['height'], subdata, image_path)
        else:
            print("Unhandled type: " + self.call_to_string(call))

    def texImage2D(self, call, thread, ctx):
        d = call.GetArgumentsDict()

        if d['format'] not in self.glformat_to_imagemode:
            print("Format ({}) not handled in {}".format(
                self.int_to_gl[d['format']],
                self.call_to_string(call)),
            )
            return

        imgmode = self.glformat_to_imagemode[d['format']]
        sub = '_sub' if 'Sub' in call.name else ''
        image_filename = 'c{callnum:08d}_i{name:04d}_t{tid}_{format}{sub}.png'.format(
            tid=call.thread_id,
            name=ctx.bound_textures[self.texfunc_to_enum[call.name]],
            callnum=call.number,
            format=self.int_to_gl[d['format']].lower()[3:],
            sub=sub,
        )
        image_path = os.path.join(self.dirname, image_filename)

        if ctx.bound_buffers[GL.GL_PIXEL_UNPACK_BUFFER].name == 0:
            self.write_texture_data(call, d, ctx, image_path, imgmode)
        else:
            self.write_buffer_data(call, d, ctx, image_path, imgmode)

    def glBindBuffer(self, call, thread, ctx):
        d = call.GetArgumentsDict()
        ctx.bound_buffers[d['target']] = Buffer(d['buffer'])

    def glClientSideBufferData(self, call, thread, ctx):
        d = call.GetArgumentsDict()
        ctx.client_side_buffers[d['name']] = d['data']

    def glCopyClientSideBuffer(self, call, thread, ctx):
        d = call.GetArgumentsDict()
        ctx.bound_buffers[d['target']].data = ctx.client_side_buffers[d['name']]

    def glBindTexture(self, call, thread, ctx):
        d = call.GetArgumentsDict()
        ctx.bound_textures[d['target']] = d['texture']

    def glPixelStorei(self, call, thread, ctx):
        d = call.GetArgumentsDict()
        ctx.pixelstore[d['pname']] = d['param']

    def eglCreateContext(self, call, thread, ctx):
        self.contexts[call.GetReturnValue()] = Context()

    def eglDestroyContext(self, call, thread, ctx):
        del self.contexts[call.GetArgumentsDict()['ctx']]

    def eglMakeCurrent(self, call, thread, ctx):
        thread.current_context = call.GetArgumentsDict()['ctx']

    def run(self, input):
        if not os.path.exists(self.dirname):
            os.makedirs(self.dirname)

        threads = {}

        for call in input.Calls():
            if call.thread_id not in threads:
                threads[call.thread_id] = Thread(call.thread_id)

            thread = threads[call.thread_id]

            ctx = self.contexts.get(thread.current_context)

            if hasattr(self, call.name):
                getattr(self, call.name)(call, thread, ctx)

    def generate_checkerboard(self):
        width = 16
        height = 16

        bright_pixel = pack('BBB', 144, 144, 144)
        dark_pixel = pack('BBB', 100, 100, 100)

        pixels = b''
        for y in range(height):
            for x in range(width):
                if ((x * 2 / width) + (y * 2 / height)) % 2:
                    pixels += bright_pixel
                else:
                    pixels += dark_pixel
        filename = os.path.join(self.dirname, 'checkerboard.png')
        self.write_image('RGB', width, height, pixels, filename)

    def generate_html(self):
        html_template = """
<table style="width:100%;border-collapse:collapse;border=1px;" border=1px>
    <thead>
        <tr>
            <th>CallNumber</th>
            <th>TextureId</th>
            <th>ThreadId</th>
            <th>Format</th>
            <th>Sub Texture</th>
            <th>Image</th>
        </tr>
    </thead>
    <tbody>
{rows}
    </tbody>
</table>
"""

        row_template = """
        <tr>
            <td>{callnum}</td>
            <td>{texid}</td>
            <td>{tid}</td>
            <td>{format}</td>
            <td>{sub}</td>
            <td style="background-image:url(checkerboard.png)"><img src="{filename}" alt="{filename}"></td>
        </tr>"""

        files = [
            f
            for f in os.listdir(self.dirname)
            if (
                os.path.isfile(os.path.join(self.dirname, f)) and
                f.endswith('.png') and
                f != 'checkerboard.png'
            )
        ]

        rows = ''.join([
            row_template.format(
                callnum=int(f.split('_')[0][1:]),
                texid=int(f.split('_')[1][1:]),
                tid=int(f.split('_')[2][1:]),
                format=f.split('_')[3].split('.')[0],
                sub='Yes' if '_sub' in f else 'No',
                filename=f,
            )
            for f in sorted(files)
        ])

        html = html_template.format(rows=rows)

        indexhtml = os.path.join(self.dirname, 'index.html')
        print("Writing {}".format(indexhtml))
        with open(indexhtml, 'w') as f:
            f.write(html)


def extract(file):
    extractor = TextureExtractor(file)

    if not os.path.exists(file):
        print("File does not exists: {}".format(file))
        return

    with InputFile(file) as input:
        extractor.run(input)
        extractor.generate_checkerboard()
        extractor.generate_html()


def main():
    parser = argparse.ArgumentParser(description='Automatically remap thread ids in a .pat trace. This should be used if an eglContext is used by more threads than the default thread.')
    parser.add_argument('file', help='Path to the .pat trace file')

    args = parser.parse_args()
    extract(args.file)

    print("Done")

if __name__ == '__main__':
    main()
