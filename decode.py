#!/usr/bin/env python

from bdec import DecodeError
from bdec.spec import load_specs
from bdec.output.instance import encode
from bdec.output.xmlout import to_file
import getopt
import os.path
import sys

def usage(program):
    print 'Decode aivdm encoded ais files.'
    print 'Usage: %s [options] <filename.aivdm> ...'
    print
    print 'Options:'
    print '  -h    Show this help.'

class Decoder:
    def __init__(self):
        spec_dir = os.path.join(os.path.split(__file__)[0], 'spec')
        self._aivdm = load_specs([os.path.join(spec_dir, 'aivdm.xml')])
        self._ais = load_specs([os.path.join(spec_dir, 'ais.xml')])

    def decode(self, data):
        # Encode aivdm data back into binary
        try:
            bits = encode(self._aivdm[0], list(ord(c) for c in data.strip()))
        except DecodeError, ex:
            filename, line_number, column_number = self._aivdm[2][ex.entry]
            sys.exit('%s[%i] - %s' %  (filename, line_number, ex))

        # Decode the ais binary
        try:
            to_file(self._ais[0].decode(bits), sys.stdout)
        except DecodeError, ex:
            filename, line_number, column_number = self._ais[2][ex.entry]
            sys.exit('%s[%i] - %s' %  (filename, line_number, ex))

def main(args):
    try:
        opts, args = getopt.getopt(args[1:], 'h', [])
    except getopt.GetOptError, ex:
        sys.exit(ex)

    for opt, value in opts:
        if opt == '-h':
            usage(args[0])
        else:
            raise NotImplementedError(opt)
    
    if not args:
        sys.exit("No files to decode. Run '%s -h' for more details.", args[0])

    decoder = Decoder()
    for filename in args:
        decoder.decode(file(filename, 'rb').read())

if __name__ == '__main__':
    main(sys.argv)
