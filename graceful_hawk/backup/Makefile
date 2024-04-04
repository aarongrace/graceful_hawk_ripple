# make a Hawk emulator
# Author:  Douglas W. Jones
#
# Instructions:
#
#   1, configure your emulator by editing this file, as directed below
#   2, make compiler=cc
#   3, make clean to remove compilation temporaries

##########################################################################
#
# Machine Configuration Section:
#
#   In effect, the skeleton of the emulator is an bus, into which you
#   plug option boards for I/O devices and other machine components.
#
#   RULE:  To select an option, follow the directions.
#          If, on real hardware, you'd have to jumper something on the
#          circuit board, then you must edit the source file for the
#          option and select the jumperable feature there.

#---- exactly one of the following definitions must be uncommented

# the Hawk cpu
cpu = cpu.o float.o
cpulib = -lm

#---- The following may be uncommented to select the Sparrowhawk CPU subset
# subset = -DSPARROWHAWK

#---- exactly one of the following definition pairs must be uncommented

# the Hawk console
console = console.o showop.o
conslib = -lcurses -ltermcap

#---- exactly one of the following definition pairs must be uncommented

# the Hawk ROM initializer
powerup = powerup.o

#---- Memory; on a real machine, the amount of memory can be selected
#     as any multiple of 0x10000 up to 0xFFFF0000 (a highly unlikely upper
#     limit).  This emulator does not allow for non-contiguous memory fields,
#     nor does it allow ROM to start anywhere but location zero.
#     The memory and ROM size is specified in bytes!

#---- exactly one of the following definitions must be uncommented

MEMORY = -DMAXMEM=0x20000 -DMAXROM=0x10000

##########################################################################
#
# Patch together the list of object files and the list of compiler
# options from the above

options =                                     $(MEMORY) $(subset) -O
objects =    $(cpu)    $(console) $(powerup)
libraries =  $(cpulib) $(conslib)


##########################################################################
#
# Override make's complex understanding of file suffixes and make sure
# that all c compiles use the options we've set up.

.SUFFIXES: .o .c

.c.o:
	$(compiler) -c $(options) $*.c

##########################################################################
#
# Actual makefile dependencies for the emulator
#
# Note that, since this makefile contains the option settings,
# it references itself!

hawk: $(objects)
	$(compiler) -o hawk $(objects) $(libraries)

$(objects): bus.h Makefile
cpu.o: irfields.h float.h powerup.h console.h
float.o: float.h
powerup.o: powerup.h
console.o: console.h showop.h float.h
showop.o: showop.h irfields.h

##########################################################################
#
# Secondary utilities

# make clean to delete the object files, saving disk space
clean:
	rm -f *.o
