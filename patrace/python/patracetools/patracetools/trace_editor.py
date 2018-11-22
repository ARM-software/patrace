import os
import sys
from optparse import OptionParser

from patrace import InputFile

import json


def main():
    # try to load PATrace module
    if not os.curdir in sys.path:
        sys.path.append(os.curdir)

    # parse the argments
    parser = OptionParser()
    parser.add_option('-j', '--display_json', default = False, action = 'store_true',
        dest = 'display_json',
        help = 'Just display the version and the json string in the header and exit.')
    (options, args) = parser.parse_args()

    if len(args) == 0 or not os.path.exists(args[0]):
        print("The input trace doesn't exist, exit.")
        sys.exit()

    if options.display_json:
        with InputFile(args[0], True) as trace:
            header_version = trace.version
            json_header = trace.jsonHeader
            print
            print 'Trace version : %s' % header_version
            print 'Trace JSON string :'
            print json.dumps(json.loads(json_header), indent=4, sort_keys=True)
        sys.exit()

    try:
        from PyQt4.QtGui import *
    except ImportError:
        print 'PyQt4 (a set of Python bindings for Qt 4 that exposes much of the functionality of Qt 4 to Python) is required.'
        print 'You can install it with "apt-get install python-qt4" under Ubuntu.'
        sys.exit()

    from trace_editor_ui import MainWindow

    app = QApplication(sys.argv)
    window = MainWindow(args[0])

    window.setGeometry(0, 0, 800, 600)
    window.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
