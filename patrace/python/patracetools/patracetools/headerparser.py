import json
import struct
from collections import namedtuple


@classmethod
def make(cls, file):
    data = file.read(cls.size())
    return cls._make(struct.unpack(cls.fmt, data))


@classmethod
def size(cls):
    return struct.calcsize(cls.fmt)


HeaderHead = namedtuple('HeaderHead', 'toNext magicNo version')
HeaderHead.fmt = 'iii'
HeaderHead.size = size
HeaderHead.make = make


HeaderV3 = namedtuple('HeaderV3', 'toNext magicNo version jsonLength jsonFileBegin jsonFileEnd')
HeaderV3.fmt = 'iiiiQQ'
HeaderV3.size = size
HeaderV3.make = make


def read_json_header(filename):
    with open(filename) as f:
        hh = HeaderHead.make(f)

        if hh.version < 5:
            raise Exception("This script only supports header format V3 and above")

        f.seek(0)
        header = HeaderV3.make(f)

        f.seek(header.jsonFileBegin)

        data = f.read(header.jsonLength)

    return json.loads(data)


def read_json_header_as_string(trace_file):
    data_dict = read_json_header(trace_file)
    return json.dumps(data_dict, sort_keys=True, indent=2)


def write_json_header(filename, header):
    jsonstr = json.dumps(header, separators=(',', ':'))

    with open(filename, 'r+') as f:
        hh = HeaderHead.make(f)

        if hh.version < 5:
            raise Exception("This script only supports header format V3 and above")

        f.seek(0)
        header = HeaderV3.make(f)
        headerdict = header._asdict()
        headerdict['jsonLength'] = len(jsonstr)

        if headerdict['jsonLength'] > (header.jsonFileEnd - header.jsonFileBegin):
            raise Exception("JSON data to be written exceeds the resereved area for JSON data.")

        f.seek(0)
        f.write(struct.pack(header.fmt, *headerdict.values()))
        f.seek(header.jsonFileBegin)
        f.write(jsonstr)
        f.write('\x00' * (header.jsonFileEnd - f.tell()))
