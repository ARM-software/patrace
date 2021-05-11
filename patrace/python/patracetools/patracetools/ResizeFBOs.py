import os, sys, json

try:
    from patrace import InputFile, OutputFile, Call
except ImportError:
    print('patrace (Python interface of PATrace SDK) is required.')

# This script converts an input trace into another trace ('output.pat') in which all FBOs are used with size 16x16.

# It modifies all glViewport calls to change the resolution to 16x16.
# This means it also sets the onscreen FBO (FBO0) resolution to 16x16.

def OutputCall(output, call):

    # create dummy textues
    if call.name == 'glViewport':
        # fail if x or y are set to other than 0
        if call.args[0].asUInt != 0 or call.args[1].asUInt != 0:
            print('Note, had x,y:', call.args[0].asUInt, call.args[1].asUInt)
        c = Call('glViewport')
        c.thread_id = call.thread_id
        c.argCount = 4
        c.args[0].asUInt = 0
        c.args[1].asUInt = 0
        c.args[2].asUInt = 16
        c.args[3].asUInt = 16
        output.WriteCall(c)
    else:
        output.WriteCall(call)


def main():
    trace_filepath = sys.argv[1]
    if not os.path.exists(trace_filepath):
        print("File does not exists: {0}".format(trace_filepath))
        sys.exit(1)

    with InputFile(trace_filepath) as input:
        with OutputFile('output.pat') as output:
            output.jsonHeader = input.jsonHeader

            for call in input.Calls():
                OutputCall(output, call)


if __name__ == '__main__':
    main()
