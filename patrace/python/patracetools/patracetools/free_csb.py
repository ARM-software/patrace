#!/usr/bin/env python2

import os
import json
import argparse
from patrace import (
    InputFile,
    OutputFile,
    Call,
    CreateInt32Value,
)


class CSB(object):
    def __init__(self, name):
        self.name = name
        self.last_used = None
        self.thread_id = None


class CallWriter(object):
    def __init__(self, output_file):
        self.output_file = output_file
        self.thread_id = None

    def glDeleteClientSideBuffer(self, name):
        injcall = Call('glDeleteClientSideBuffer')
        injcall.thread_id = self.thread_id
        injcall.args.push_back(CreateInt32Value(name))
        self.output_file.WriteCall(injcall)


class Injector(object):
    def __init__(self):
        self.num_calls_injected = 0
        self.csbs = {}

    def _find_csbs(self, input):
        print 'Finding ClientSideBuffer objects...'

        csb_usage_calls = [
            'glClientSideBufferData',
            'glClientSideBufferSubData',
            'glCopyClientSideBuffer',
            'glPatchClientSideBuffer',
        ]

        for call in input.Calls():
            if call.name == 'glCreateClientSideBuffer':
                csb_name = call.GetReturnValue()
                self.csbs[csb_name] = CSB(name=csb_name)
            elif call.name in csb_usage_calls:
                csb_name = call.GetArgumentsDict()['name']
                csb = self.csbs[csb_name]
                csb.last_used = call.number
                csb.thread_id = call.thread_id

        return {
            csb.last_used: csb
            for csb in self.csbs.values()
        }

    def run(self, input, output):
        # Modify header, if we are remaping the default tid
        header = json.loads(input.jsonHeader)
        output.jsonHeader = json.dumps(header)

        callinjectnum_to_csb = self._find_csbs(input)

        call_writer = CallWriter(output)

        print("Injecting glDeleteClientSideBuffer calls...")
        for call in input.Calls():
            output.WriteCall(call)

            if call.number in callinjectnum_to_csb:
                csb = callinjectnum_to_csb[call.number]
                # Inject call:
                call_writer.thread_id = csb.thread_id
                call_writer.glDeleteClientSideBuffer(name=csb.name)

        self.num_calls_injected = len(self.csbs)


def inject(oldfile, newfile):
    if not os.path.exists(oldfile):
        print("File does not exists: {}".format(oldfile))
        return

    injector = Injector()
    with InputFile(oldfile) as input:
        with OutputFile(newfile) as output:
            injector.run(input, output)

    return injector.num_calls_injected


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Inject glDeleteClientSideBuffer calls to reduce memory footprint '
            'while retracing.'
        )
    )
    parser.add_argument('oldfile', help='Path to the .pat trace file')
    parser.add_argument('newfile', help='New .pat file to create')

    args = parser.parse_args()
    num_calls_injected = inject(args.oldfile, args.newfile)
    print "Injected {} glDeleteClientSideBuffer calls".format(num_calls_injected)


if __name__ == '__main__':
    main()
