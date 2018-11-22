#!/usr/bin/env python2
import argparse
from sets import Set

try:
    from patrace import InputFile, OutputFile
except ImportError:
    print 'patrace (Python interface of PATrace SDK) is required.'

#
# This script converts an input trace into another trace ('output.pat') in which all calls
#


class Application(object):
    def __init__(self, args):
        self.args = args
        self.current_frame = 0

    def run(self):
        with InputFile(self.args.file) as input:
            with OutputFile(self.args.newfile) as output:
                output.jsonHeader = input.jsonHeader
                for call in input.Calls():
                    if self.store_call(call):
                        output.WriteCall(call)

                    if call.name == 'eglSwapBuffers':
                        self.current_frame += 1

    def store_call(self, call):
        if self.current_frame not in self.args.frames:
            return True

        if call.name == 'eglSwapBuffers':
            if not self.args.removeswaps:
                return True
        elif self.args.removedrawsonly:
            if not call.name.startswith('glDraw'):
                return True

        return False


def frames(string):
    """Takes a comma separate list of frames/frame ranges, and returns a set containing those frames

    Example: "2,3,4-7,10" => Set([2,3,4,5,6,7,10])
    """
    def decode_frames(string):
        print 's', string
        if '-' in string:
            a = string.split('-')
            start, end = int(a[0]), int(a[1])
            for i in range(start, end + 1):
                yield i
        else:
            yield int(string)
    framesList = string.split(',')
    frames = Set()
    for strFrames in framesList:
        for frameNumber in decode_frames(strFrames):
            frames.add(frameNumber)
    return frames


def main():
    parser = argparse.ArgumentParser(description='Remove eglSwapbuffers calls and all draw calls targeting FBO0')
    parser.add_argument('file', help='Path to the .pat trace file')
    parser.add_argument('--frames', type=frames, help='Frames to remove, comma-separated individual frames or frame ranges. Example: \'2,3,4-7,10\'')
    parser.add_argument('--removeswaps', action="store_true", default=False, help='Specify whether to remove eglSwapBuffers as well. Default is False.')
    parser.add_argument('--removedrawsonly', action="store_true", default=False, help='Specify whether to remove only draw calls from the specified frame range. Default is False.')
    parser.add_argument('--newfile', default='trace_out.pat', help="Specifies the path to the output trace file")
    args = parser.parse_args()

    Application(args).run()

if __name__ == '__main__':
    main()
