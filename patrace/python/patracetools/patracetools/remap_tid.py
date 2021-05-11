#!/usr/bin/env python3
import argparse
import json

try:
    from patrace import InputFile, OutputFile
except ImportError:
    print('patrace (Python interface of PATrace SDK) is required.')


class Remapper:
    def __init__(self, args):
        self.num_calls_remapped = 0
        self.skip_counter = 0
        self.args = args

    def run(self, input, output):
        # Modify header, if we are remaping the default tid
        header = json.loads(input.jsonHeader)
        if self.args.oldtid in [-1, header['defaultTid']]:
            print("Setting new default tid: {0}".format(self.args.newtid))
            header['defaultTid'] = self.args.newtid
        output.jsonHeader = json.dumps(header)

        for call in input.Calls():
            if self.args.calls:
                if call.number in self.args.calls:
                    self.remap(call, self.args.newtid)
            elif self.included(call):
                if (
                    self.relevant_thread(call) and
                    self.skip_counter >= self.args.begin and
                    not self.exceed_count()
                ):
                    self.remap(call, self.args.newtid)
                self.skip_counter += 1

            output.WriteCall(call)

    def remap(self, call, newtid):
        call.thread_id = newtid
        self.num_calls_remapped += 1

    def relevant_thread(self, call):
        return self.args.oldtid in [-1, call.thread_id]

    def included(self, call):
        return self.args.include == '' or call.name == self.args.include

    def exceed_count(self):
        return self.args.count > -1 and self.num_calls_remapped >= self.args.count


def main():
    parser = argparse.ArgumentParser(description='Remap thread ids in a .pat trace')
    parser.add_argument('file', help='Path to the .pat trace file')
    parser.add_argument('oldtid', type=int, help='The thread id to remap. If set to -1, all thread ids will be remapped.')
    parser.add_argument('newtid', type=int, help='The new thread id')
    parser.add_argument('--newfile', default='trace_out.pat', help="Specifies the path to the output trace file")
    parser.add_argument('--include', default='', help="Only functions with this name will be remapped")
    parser.add_argument('--begin', type=int, default=0, help="Skip this many calls before remapping starts")
    parser.add_argument('--count', type=int, default=-1, help="Remap this many calls. -1 means remap all.")
    parser.add_argument('--calls', metavar='N', type=int, nargs='+', help="List of call numbers to remap. If used, all other options are ignored.")

    args = parser.parse_args()
    remapper = Remapper(args)

    with InputFile(args.file) as input:
        with OutputFile(args.newfile) as output:
            remapper.run(input, output)

    print("Number of calls remapped {num}".format(num=remapper.num_calls_remapped))

if __name__ == '__main__':
    main()
