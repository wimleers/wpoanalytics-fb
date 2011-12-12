#!/usr/local/bin/python2.6

import os
import sys
from subprocess import Popen
from subprocess import PIPE

from optparse import OptionParser

moc_path = "/home/engshare/third-party/gcc-4.6.2-glibc-2.13/qt/qt-4.7.4/bin/moc"
project_name = "patternminer"

def getopts():
    parser = OptionParser()
    parser.add_option("--fbcode_dir", default=None,
                      help="the fbcode base directory (automatic)"),
    parser.add_option("--install_dir", default=None,
                      help="the fbcode build install dir (automatic)"),
    parser.add_option("--file", default=None,
                      help="header file that inherits QObject")

    (options, args) = parser.parse_args()

    if not options.fbcode_dir:
        print "Error: this should be called from fbmake"
        sys.exit(1)
    if not options.file:
        print "Error: --file is required"
        sys.exit(1)

    return (options, args)


def moc(options):
    filename = os.path.basename(options.file)
    basename = os.path.splitext(filename)[0]
    target = "moc_%s.cpp" % (basename)

    fbconfig_mess_args = "-f%(header_file)s -p%(fbcode_dir)s/%(project_name)s"
    C_format = "%(MOC)s -o %(install_dir)s/%(TARGET)s " + fbconfig_mess_args + " %(SOURCE)s"
    formatter_values = {
       # The simple stuff.
       "MOC"    : moc_path,
       "TARGET" : target,
       "SOURCE" : options.file,
       # The mess `fbconfig` forces us in.
       "header_file"  : options.file, # Necessary because this moc file is
                                      # generated in an isolated environment,
                                      # separate from the target environment.
       "fbcode_dir"   : options.fbcode_dir,
       "install_dir"  : options.install_dir,
       "project_name" : project_name,
    }

    C = C_format % formatter_values

    print "Running command:"
    print C
    p = Popen(C, shell=True, stdout=PIPE)
    print p.communicate()[0]    

if __name__ == "__main__":
    (options, args) = getopts()
    moc(options)
