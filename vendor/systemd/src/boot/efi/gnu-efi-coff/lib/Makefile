CC=clang
VECHO = echo "	"
.DEFAULT_GOAL := libefi.a

CFLAGS+= \
	-O2 -Wall -Wextra -Werror -Wno-int-to-pointer-cast \
	-Wno-unused-parameter \
	-D__GNUC__ \
	-fno-strict-aliasing \
	-fno-stack-protector -fno-stack-check \
	-target aarch64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-I../inc/

LIBEFI_SRCS = boxdraw.c console.c data.c dpath.c event.c guid.c hw.c lock.c \
	pause.c smbios.c str.c cmdline.c crc.c debug.c error.c exit.c hand.c \
	init.c misc.c print.c sread.c \
	aarch64/initplat.c aarch64/math.c \
	runtime/efirtlib.c runtime/rtdata.c runtime/rtlock.c runtime/rtstr.c runtime/vm.c


LIBEFI_OBJS=$(LIBEFI_SRCS:.c=.o)

libefi.a: $(LIBEFI_OBJS)
	llvm-ar rcs $@ $(LIBEFI_OBJS)

clean:
	rm -f *.a *.o aarch64/*.o runtime/*.o
