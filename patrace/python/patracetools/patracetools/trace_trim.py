import argparse
import json
from patrace import InputFile, OutputFile


def main():
    parser = argparse.ArgumentParser(
        description='trim a tracefile',
        epilog='Remember that you wont be able to playback the trace if the '
               'range doesn\'t include resources (texture,shader,vbo) needed '
               'for selected range.'
    )
    parser.add_argument('-f', '--frame_range', type=int, nargs=2, required=True,
                        help='Specify the frame range to record drawcalls')
    parser.add_argument('input_trace',
                        help='Specify the PAT trace to parse')
    parser.add_argument('output_trace',
                        help='Specify name of trace to create')
    args = parser.parse_args()

    if not args.output_trace.endswith('.pat'):
        args.output_trace += '.pat'

    JSON_HEADER_FRAME_COUNT_KEY = 'frameCnt'
    JSON_HEADER_CALL_COUNT_KEY = 'callCnt'

    with InputFile(args.input_trace) as input:
        with OutputFile(args.output_trace) as output:

            jsonHeader = json.loads(input.jsonHeader)

            call_count = 0
            for index in range(args.frame_range[0], args.frame_range[1] + 1):
                for call in input.FrameCalls(index):
                    output.WriteCall(call)
                    call_count += 1

            if JSON_HEADER_FRAME_COUNT_KEY in jsonHeader:
                jsonHeader[JSON_HEADER_FRAME_COUNT_KEY] = args.frame_range[1] - args.frame_range[0] + 1
            if JSON_HEADER_CALL_COUNT_KEY in jsonHeader:
                jsonHeader[JSON_HEADER_CALL_COUNT_KEY] = call_count
            output.jsonHeader = json.dumps(jsonHeader, indent=4)

if __name__ == "__main__":
    main()
