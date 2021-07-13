#
# Deb package dependencies: clang, lld
CC=clang
GNU_EFI=/home/abeato/src/snappy/kernel/core-initrd/vendor/gnu-efi
LIBFDT=/home/abeato/src/snappy/kernel/core-initrd/vendor/libfdt

CFLAGS+= \
	-DGIT_VERSION=\"FAFAFA\" -DEFI_MACHINE_TYPE_NAME=\"aa64\" \
	-isystem $(GNU_EFI)/inc/ \
        -isystem $(GNU_EFI)/inc/aarch64/ \
        -isystem $(LIBFDT)/lib/contrib/ \
	-target aarch64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone

# lld-link-10 for PE+ binaries
LDFLAGS+= \
	-target aarch64-unknown-windows \
	$(GNU_EFI)/aarch64/lib/libefi.a \
        $(LIBFDT)/lib/contrib/libfdt.a \
	-nostdlib \
	-Wl,-entry:efi_main \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link-10

SRCS=disk.c graphics.c measure.c pe.c util.c linux.c splash.c stub.c

OBJS=$(SRCS:.c=.o)

stub: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
