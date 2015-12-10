#
# Makefile for Advances in Computer Architecture coursework
#              
# Author(s): Michiel W. van Tol
# Copyright (C) 2009 by Computer Systems Architecture group, 
#                       University of Amsterdam
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Note: This Makefile requires GNU Make 3.81 or newer
#/home/arun/SystemC
# Location of the SystemC library files
#SYSTEMC_PATH    = /afs/tu-berlin.de/units/Fak_IV/aes/tools/mca/systemc-2.3.0
SYSTEMC_PATH    = /home/arun/SystemC/systemc-2.3.0
# SYSTEMC_PATH    = /usr/local/systemc-2.3.0
SYSTEMC_INCLUDE = $(SYSTEMC_PATH)/include

# Figure out on what processor/architecture we compile
MACHINE_ARCH = $(shell uname -m)
KERNEL = $(shell uname -s)

# Then pick the right path for the precompiled SystemC Library
ifeq ($(KERNEL),Darwin)
    SYSTEMC_LIBDIR     = $(SYSTEMC_PATH)/lib-macosx
endif
ifeq ($(MACHINE_ARCH),sun4u)
    SYSTEMC_LIBDIR     = $(SYSTEMC_PATH)/lib-gccsparcOS5
endif
ifeq ($(MACHINE_ARCH),i686)
    SYSTEMC_LIBDIR     = $(SYSTEMC_PATH)/lib-linux
endif
ifeq ($(MACHINE_ARCH),x86_64)
    SYSTEMC_LIBDIR     = $(SYSTEMC_PATH)/lib-linux64
endif

# Path settings
SOURCE_PATH     = src

# ACA2009 lib
ACALIB_DIR    = acalib/
ACALIB        = $(ACALIB_DIR)aca2009.cpp


# Compiler settings
CC              = g++
CFLAGS          = -Wall -O2
INCLUDES        = -I $(SYSTEMC_INCLUDE) -I $(ACALIB_DIR)
LIBS            = -lsystemc -lm -Wl,-rpath $(SYSTEMC_LIBDIR)
LIBDIR          = -L$(SYSTEMC_LIBDIR)

# Find all targets
TARGETS         := $(patsubst $(SOURCE_PATH)/%,%,$(shell find $(SOURCE_PATH)/* -type d))

# Find all .cpp and .h files in target, 
# using find so it throws a warning when no .cpp is found
CPP_FILES       = $(shell find $(SOURCE_PATH)/$*/*.cpp)

# Same as above, but for dependency rule for .cpp and .h files
D_CPP_FILES     = $$(wildcard $(SOURCE_PATH)/$$*/*.cpp)
D_H_FILES       = $$(wildcard $(SOURCE_PATH)/$$*/*.h)

.SECONDEXPANSION:
.PHONY: all targets clean $(TARGETS)

all: $(TARGETS)
	
$(TARGETS): $$@.bin

%.bin: $(D_CPP_FILES) $(D_H_FILES) $(SYSTEMC_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(CPP_FILES) $(ACALIB) $(LIBDIR) $(LIBS)
	
targets:
	@echo List of found targets:
	@echo $(TARGETS)
	@echo
	@echo List of found cpp files:
	@echo $(foreach sourcedir,$(TARGETS),$(wildcard $(SOURCE_PATH)/$(sourcedir)/*.cpp))
	@echo
	@echo SystemC installation used in:
	@echo $(SYSTEMC_LIBDIR)        

clean:
	rm -f $(TARGETS:%=%.bin)

