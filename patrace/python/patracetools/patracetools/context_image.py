#!/usr/bin/env python3
import argparse
import struct
import svgwrite
from patrace import InputFile


class DrawValues:
    threadrow_height = 100
    contextrow_height = 20
    surfacerow_height = 50
    swapbuffers_height = 20

    contextrow_ypos = 5
    surfacerow_ypos = contextrow_ypos + contextrow_height
    swapbuffers_ypos = surfacerow_ypos + surfacerow_height


def svg_template(svgdoc, num_calls, num_threads):
    group = svgdoc.add(svgdoc.g(id='template', stroke_width=1))
    for t in range(num_threads):
        group.add(
            svgdoc.line(
                start=(0, t * DrawValues.threadrow_height),
                end=(num_calls, t * DrawValues.threadrow_height),
                stroke='silver',
            )
        )

        group.add(
            svgdoc.text(
                "Thread {}".format(t),
                insert=(0, t * DrawValues.threadrow_height + DrawValues.threadrow_height * 0.5),
            )
        )


class Remapper:
    def __init__(self):
        self.num_calls_remapped = 0

        self.color_map = [
            (0xff, 0xff, 0xff),
            (0xa6, 0xce, 0xe3),
            (0x1f, 0x78, 0xb4),
            (0xb2, 0xdf, 0x8a),
            (0x33, 0xa0, 0x2c),
            (0xfb, 0x9a, 0x99),
            (0xe3, 0x1a, 0x1c),
            (0xfd, 0xbf, 0x6f),
            (0xff, 0x7f, 0x00),
            (0xca, 0xb2, 0xd6),
            (0x6a, 0x3d, 0x9a),
            (0xff, 0xff, 0x99),
            (0xb1, 0x59, 0x28),
            (0xff, 0xff, 0xff),
            (0xff, 0x40, 0x40),
            (0x40, 0xff, 0x40),
            (0x40, 0x40, 0xff),
            (0xff, 0xff, 0x40),
            (0xff, 0x40, 0xff),
            (0x40, 0xff, 0xff),
            (0xff, 0x80, 0xff),
            (0xff, 0x40, 0x80),
            (0x2f, 0x40, 0x40),
            (0x40, 0x2f, 0x40),
            (0x40, 0x40, 0x2f),
            (0x2f, 0x2f, 0x40),
            (0x2f, 0x40, 0x2f),
            (0x40, 0x2f, 0x2f),
            (0x2f, 0x80, 0x2f),
            (0x2f, 0x40, 0x80),
        ]

    def write_pixels(self, file, color, num):
        for x in range(num):
            file.write(struct.pack('BBB', *color))

    def run(self, filename, input):

        print('Searching for relevant calls...')
        call_lists = {
            'eglMakeCurrent': [],
            'eglCreateContext': [],
            'eglDestroyContext': [],
            'eglCreateWindowSurface': [],
            'eglCreateWindowSurface2': [],
            'eglCreatePbufferSurface': [],
            'eglDestroySurface': [],
            'eglSwapBuffers': [],
        }

        context_calls = []

        num_calls = 0
        for call in input.Calls():
            num_calls += 1
            if call.name in list(call_lists.keys()):
                context_calls.append({
                    'name': call.name,
                    'tid': call.thread_id,
                    'params': call.GetArgumentsDict().copy(),
                    'retval': call.GetReturnValue(),
                    'number': call.number,
                })

        print("Number of calls: {}".format(num_calls))
        # Sometimes, contexts can get the same pointer values
        # Hence, the contexts pointers will not be unique. Therefor,
        # we create an unique, sequential id.
        context_sequential_id = 1
        # Maps original context id with sequential context id.
        contexts_idmap = {0: 0}

        # Do the same for surfaces
        surface_sequential_id = 1
        surface_idmap = {0: 0}

        highest_thread_id = -1
        for call in context_calls:
            highest_thread_id = max(call['tid'], highest_thread_id)

            if call['name'] == 'eglCreateContext':
                contexts_idmap[call['retval']] = context_sequential_id
                call['retval'] = context_sequential_id
                context_sequential_id += 1
            elif call['name'] == 'eglDestroyContext':
                old_id = call['params']['ctx']
                seq_id = contexts_idmap[old_id]
                del contexts_idmap[old_id]
                call['params']['ctx'] = seq_id
            elif call['name'] == 'eglMakeCurrent':
                # Change ctx parameter to our new sequential id
                call['params']['ctx'] = contexts_idmap[call['params']['ctx']]
                call['params']['draw'] = surface_idmap[call['params']['draw']]
                call['params']['read'] = surface_idmap[call['params']['read']]
            elif call['name'] in ['eglCreateWindowSurface', 'eglCreateWindowSurface2', 'eglCreatePbufferSurface']:
                surface_idmap[call['retval']] = surface_sequential_id
                call['retval'] = surface_sequential_id
                surface_sequential_id += 1
            elif call['name'] == 'eglDestroySurface':
                old_id = call['params']['surface']
                seq_id = surface_idmap[old_id]
                del surface_idmap[old_id]
                call['params']['surface'] = seq_id
            elif call['name'] == 'eglSwapBuffers':
                old_id = call['params']['surface']
                seq_id = surface_idmap[old_id]
                call['params']['surface'] = seq_id

        num_threads = highest_thread_id + 1

        class States:
            NONE = 0
            CREATE = 1
            DESTROY = 2
            CURRENT = 3

        class Colors:
            BLACK = [0] * 3
            WHITE = [0xff] * 3
            SILVER = [0xaf] * 3

        class Thread:
            def __init__(self):
                self.state = States.NONE
                self.current_ctx = 0
                self.current_surface = 0
                self.context = 0
                self.context_boxes = []
                self.surface_boxes = []
                self.swapbuffer_ticks = []

        class Box:
            def __init__(self, lower_left, color, text):
                self.lower_left = lower_left
                self.upper_right = (0, 0)
                self.color = color
                self.text = text

            def __repr__(self):
                return "{} -> {}".format(self.lower_left, self.upper_right)

        class Line:
            def __init__(self, start, end, color):
                self.start = start
                self.end = end
                self.color = color

            def __repr__(self):
                return "{} -> {}".format(self.start, self.end)

        # Threads represent the state of a thread.
        threads = [Thread() for i in range(num_threads)]

        for i in range(num_calls):
            if not context_calls:
                break

            thread = threads[context_calls[0]['tid']]

            if context_calls[0]['name'] == 'eglCreateContext':
                thread.state = States.CREATE
                thread.context = context_calls[0]['retval']
            elif context_calls[0]['name'] == 'eglDestroyContext':
                thread.state = States.DESTROY
                thread.context = context_calls[0]['params']['ctx']
            elif (
                context_calls[0]['number'] == i and
                context_calls[0]['name'] == 'eglMakeCurrent'
            ):
                thread.state = States.CURRENT
                thread.current_ctx = context_calls[0]['params']['ctx']
                thread.current_surface = context_calls[0]['params']['draw']
                if thread.context_boxes:
                    thread.context_boxes[-1].upper_right = (
                        context_calls[0]['number'] - 1,
                        context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.contextrow_ypos + DrawValues.contextrow_height
                    )
                thread.context_boxes.append(
                    Box(
                        lower_left=(
                            context_calls[0]['number'],
                            context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.contextrow_ypos
                        ),
                        color='rgb({}, {}, {})'.format(*self.color_map[thread.current_ctx]),
                        text='Context #{}'.format(thread.current_ctx),
                    )
                )

                if thread.surface_boxes:
                    thread.surface_boxes[-1].upper_right = (
                        context_calls[0]['number'] - 1,
                        context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.surfacerow_ypos + DrawValues.surfacerow_height
                    )

                thread.surface_boxes.append(
                    Box(
                        lower_left=(
                            context_calls[0]['number'],
                            context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.surfacerow_ypos
                        ),
                        color='rgb({}, {}, {})'.format(*self.color_map[thread.current_surface]),
                        text='Surface #{}'.format(thread.current_surface),
                    )
                )

                context_calls = context_calls[1:]
            elif (
                context_calls[0]['number'] == i and
                context_calls[0]['name'] == 'eglSwapBuffers'
            ):
                thread.swapbuffer_ticks.append(
                    Line(
                        start=(i, context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.swapbuffers_ypos),
                        end=(i, context_calls[0]['tid'] * DrawValues.threadrow_height + DrawValues.swapbuffers_ypos + DrawValues.swapbuffers_height),
                        color='rgb({}, {}, {})'.format(*self.color_map[thread.current_surface])
                    )
                )
                context_calls = context_calls[1:]

            if context_calls and context_calls[0]['name'] in list(call_lists.keys()):
                if context_calls[0]['name'] not in ['eglMakeCurrent', 'eglSwapBuffers']:
                    context_calls = context_calls[1:]

        for i, thread in enumerate(threads):
            # Cap last box
            if thread.context_boxes:
                thread.context_boxes[-1].upper_right = (
                    num_calls,
                    i * DrawValues.threadrow_height + DrawValues.contextrow_ypos + DrawValues.contextrow_height
                )

            if thread.surface_boxes:
                thread.surface_boxes[-1].upper_right = (
                    num_calls,
                    i * DrawValues.threadrow_height + DrawValues.surfacerow_ypos + DrawValues.surfacerow_height
                )

        # Write boxes
        svgdoc = svgwrite.Drawing(filename='{}.svg'.format(filename),
                                  size=(num_calls, 100 * num_threads + 100))

        svg_template(svgdoc, num_calls, num_threads)

        for i, thread in enumerate(threads):
            for box in thread.context_boxes + thread.surface_boxes:
                boxsize = (box.upper_right[0] - box.lower_left[0] + 1, box.upper_right[1] - box.lower_left[1])
                start = (box.lower_left[0], box.lower_left[1])
                g = svgdoc.g()
                g.add(
                    svgdoc.rect(
                        insert=start,
                        size=boxsize,
                        stroke_width=0,
                        stroke='black',
                        fill=box.color,
                    )
                )
                g.add(
                    svgdoc.text(
                        box.text,
                        insert=(start[0], start[1] + 15),
                        fill='black',
                    )
                )
                svgdoc.add(g)

            swapbuffer_group = svgdoc.add(svgdoc.g(id='eglSwapBuffer_{}'.format(i), stroke_width=5))
            for line in thread.swapbuffer_ticks:
                swapbuffer_group.add(
                    svgdoc.line(
                        start=line.start,
                        end=line.end,
                        stroke=line.color,
                    )
                )

        svgdoc.add(
            svgdoc.text(
                filename,
                insert=(50, num_threads * 100),
                style="font-size:2em",
            )
        )

        svgdoc.save()


def remap(trace):
    remapper = Remapper()

    with InputFile(trace) as input:
        remapper.run(trace, input)

    return remapper.num_calls_remapped


def main():
    parser = argparse.ArgumentParser(description='Visualise contexts on different thread in a trace. Outputs out.ppm.')
    parser.add_argument('trace', help='Path to the .pat file')

    args = parser.parse_args()
    remap(args.trace)

    print("Done")


if __name__ == '__main__':
    main()
