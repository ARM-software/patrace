import os, sys, json
import argparse

try:
    from patrace import InputFile, OutputFile, Call
except ImportError:
    print 'patrace (Python interface of PATrace SDK) is required.'
    sys.exit(1)

def main():
    ap = argparse.ArgumentParser(description="Output the call numbers of a certain GLES function.")
    ap.add_argument('trace_path', help='Trace path')
    ap.add_argument('func_name', help='Function name')
    ap.add_argument('min_call_num', type=int, default=0, help='Min call number')
    ap.add_argument('max_call_num', type=int, default=0, help='Max call number (or 0 to ignore)')
    args = ap.parse_args()

    if not os.path.exists(args.trace_path):
        print("File does not exist: {0}".format(args.trace_path))
        sys.exit(1)

    out = get_call_numbers(args.trace_path, args.func_name, args.min_call_num, args.max_call_num)
    print json.dumps(out)

def get_call_numbers(trace_path, func_name, min_call_num, max_call_num):
    out = list()
    with InputFile(trace_path) as input:
        for call in input.Calls():
            if max_call_num and call.number > max_call_num:
                break
            if min_call_num <= call.number and call.name == func_name:
                out.append((call.number, str(call.name), [a.asUInt for a in call.args]))

    return out

if __name__ == '__main__':
    main()
