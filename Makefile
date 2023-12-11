# Use this Makefile as a starting point for your A4.

# Keep the first rule with the target "all", and change the name of the
# dependencies to whatever you like. These represent the four main programs
# that a complete assignment should produce. If you haven't completed them
# all, remove any that don't exist.

# Then, change the remaining rules to build each of those main programs,
# based on the names of your source and header files. For the targets that
# link the programs, make sure the target name matches the first object
# file name, without the ".o". And for the targets that compile the object
# files, make sure that the .o target and the first .c dependency match.

# Use of variables is optional but it will make things easier!

CC=clang
CFLAGS=-Wall -g -DNDEBUG

# The default goal is to build all four programs

all: a5_main a5_test_mm

# Targets to link all four programs

a5_main: a5_main.o a5_imffs.o a5_multimap.o

a5_test_mm: a5_test_mm.o a4_tests.o a5_multimap.o

a5_test_imffs: a5_test_imffs.o a4_tests.o a5_imffs.o a5_multimap.o

# Targets to compile all object files
a5_imffs.o: a5_imffs.h a5_imffs.c a5_multimap.h

a5_test_mm.o: a5_test_mm.c a5_multimap.h a4_tests.h

a4_tests.o: a4_tests.c a4_tests.h

a5_main.o: a5_main.c a5_imffs.h a5_multimap.h

a5_test_imffs.o: a5_test_imffs.c a4_tests.h a5_imffs.h a5_multimap.h

# Remove build products

clean:
	rm -f *.o a5_main a5_test_mm
