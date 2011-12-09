###############################################################################
# .pro file for building on Facebook servers                                  #
###############################################################################

FB_OS = "centos5.2-native"
FB_PLATFORM = "gcc-4.6.2-glibc-2.13"

###############################################################################
 
include ("patternminer-CLI.pro")

# Name the target binary.
TARGET = patternminer

# Use static linking.
CONFIG += static link_prl

# Use the the third-party repository compiler & linker.
QMAKE_CXX  = /home/engshare/third-party/$$FB_OS/gcc/$$FB_PLATFORM/bin/g++
QMAKE_LINK = /home/engshare/third-party/$$FB_OS/gcc/$$FB_PLATFORM/bin/g++

# Necessary to enforce a statically linked binary, because g++ defaults to a
# dynamically linked one whenever at least one library or plug-in uses
# dynamic linking.
# Qt contains references to dlopen() and thus it would not result in a static
# build. Now it does.
QMAKE_LFLAGS += -static
