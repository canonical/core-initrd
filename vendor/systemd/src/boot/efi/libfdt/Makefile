# SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)
# Makefile.libfdt
#

include Makefile.libfdt

CC=clang
VECHO = echo "	"
.DEFAULT_GOAL := libfdt.a

CFLAGS+= \
	-target aarch64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-I.

libfdt.a: $(LIBFDT_OBJS)
	llvm-ar rcs $@ $(LIBFDT_OBJS)

clean:
	rm -f *.a *.o
