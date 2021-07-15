#
# Deb package dependencies: clang, lld
CC=clang
GNU_EFI=/home/abeato/src/snappy/kernel/core-initrd/vendor/gnu-efi
LIBFDT=./libfdt

CFLAGS+= \
	-DGIT_VERSION=\"FAFAFA\" -DEFI_MACHINE_TYPE_NAME=\"aa64\" \
	-isystem $(GNU_EFI)/inc/ \
        -isystem $(GNU_EFI)/inc/aarch64/ \
        -isystem $(LIBFDT) \
	-target aarch64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone

# lld-link-10 for PE+ binaries
LDFLAGS+= \
	-target aarch64-unknown-windows \
	$(GNU_EFI)/aarch64/lib/libefi.a \
        $(LIBFDT)/libfdt.a \
	-nostdlib \
	-Wl,-entry:efi_main \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link-10

SRCS=disk.c graphics.c measure.c pe.c util.c linux.c splash.c stub.c stdclib.c

OBJS=$(SRCS:.c=.o)

stub: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o stub
