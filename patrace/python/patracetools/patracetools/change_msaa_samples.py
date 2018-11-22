#!/usr/bin/env python2
import argparse
import json

try:
    from patrace import InputFile, OutputFile
except ImportError:
    print 'patrace (Python interface of PATrace SDK) is required.'


def change_msaa_samples(input, output, samples):
    header = json.loads(input.jsonHeader)
    output.jsonHeader = json.dumps(header)
    current_fbo = -1
    modified_fbos = set()

    for call in input.Calls():
        if call.name == 'glBindFramebuffer':
            current_fbo = call.args[1].asInt
        elif call.name == 'glRenderbufferStorageMultisampleEXT':
            original_samples = call.args[1].asInt
            call.args[1].asInt = samples
            modified_fbos.add(current_fbo)

            print 'fbo: ' + str(current_fbo)
            print 'original samples: ' + str(original_samples)
            print 'new samples: ' + str(samples)
            print '---'
        elif call.name == 'glFramebufferTexture2DMultisampleEXT':
            original_samples = call.args[5].asInt
            call.args[5].asInt = samples
            modified_fbos.add(current_fbo)

            print 'fbo: ' + str(current_fbo)
            print 'original samples: ' + str(original_samples)
            print 'new samples: ' + str(samples)
            print '---'

        output.WriteCall(call)

    return modified_fbos


def main():
    parser = argparse.ArgumentParser(description='Remap thread ids in a .pat trace')
    parser.add_argument('file', help='Path to the .pat trace file')
    parser.add_argument('--newfile', default='trace_out.pat', help="Specifies the path to the output trace file")
    parser.add_argument('--samples', type=int,  default=0, help="Only functions with this name will be remapped", choices=[0, 2, 4, 8, 16])

    args = parser.parse_args()

    with InputFile(args.file) as input:
        with OutputFile(args.newfile) as output:
            modified_fbos = change_msaa_samples(input, output, args.samples)
            print 'Modified FBOs: ' + str(modified_fbos)

if __name__ == '__main__':
    main()
