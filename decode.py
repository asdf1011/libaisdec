#!/usr/bin/env python

from bdec import DecodeError
from bdec.data import Data
from bdec.field import Field
from bdec.spec import load_specs, LoadError
from bdec.output.instance import encode
from bdec.output.xmlout import to_file
import getopt
from glob import glob
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
        self._fake_6_bit_text_field = Field('6 bit ascii', length=0,
                format=Field.TEXT)

        try:
            self._aivdm = load_specs([os.path.join(spec_dir, 'aivdm.xml')])
            self._ais = load_specs(glob(os.path.join(spec_dir, 'ais*.xml')))
        except LoadError, ex:
            sys.exit(str(ex))

    def _filter_6_bit_ascii(self, items):
        """Convert 6-bit ascii decode items into a single text field.
        
        This doesn't change the decoding, but hacks the decoded output
        so it displays the data in a nicer fashion.

        items -- An iterable list of decoded entries.
        return -- A iterable list of decoded entries, 6-bit ascii items
            collapsed into a single field."""
        iterable = iter(items)
        for is_starting, name, entry, data, value in iterable:
            if is_starting and entry.name == '6 bit ascii':
                # We found a 6-bit ascii string. Get the data and values for
                # the child entries, and collapse them into a single result.
                yield True, name, self._fake_6_bit_text_field, None, None
                text = []
                d = Data()
                for is_starting, name, entry, data, value in iterable:
                    if entry.name == '6 bit ascii':
                        break
                    elif value is not None and name=='character':
                        d += data
                        text += chr(value)
                yield False, name, self._fake_6_bit_text_field, d, ''.join(text)
            else:
                yield is_starting, name, entry, data, value

    def decode(self, data):
        # Encode aivdm data back into binary
        try:
            bits = encode(self._aivdm[0], list(ord(c) for c in data.strip()))
        except DecodeError, ex:
            filename, line_number, column_number = self._aivdm[2][ex.entry]
            sys.exit('%s[%i] - %s' %  (filename, line_number, ex))

        # Decode the ais binary
        try:
            to_file(self._filter_6_bit_ascii(self._ais[0].decode(bits)), sys.stdout)
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
