CC=clang

CFLAGS+= \
	-DGIT_VERSION=\"FAFAFA\" -DEFI_MACHINE_TYPE_NAME=\"aa64\" \
	-isystem /home/ubuntu/src/gnu-efi/inc/ -isystem /home/ubuntu/src/gnu-efi/inc/aarch64/ \
	-target aarch64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone

# lld-link-10 for PE+ binaries
LDFLAGS+= \
	-target aarch64-unknown-windows \
	/home/ubuntu/src/gnu-efi/aarch64/lib/libefi.a \
	-nostdlib \
	-Wl,-entry:efi_main \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link-10

SRCS=disk.c graphics.c measure.c pe.c util.c linux.c splash.c stub.c

OBJS=$(SRCS:.c=.o)
EFI_OBJS=boxdraw.o smbios.o console.o crc.o data.o debug.o dpath.o error.o event.o exit.o guid.o hand.o hw.o init.o lock.o misc.o pause.o print.o sread.o str.o cmdline.o rtlock.o efirtlib.o rtstr.o vm.o rtdata.o initplat.o math.o setjmp.o

stub: $(OBJS)
	$(CC) -o stub $(OBJS) $(EFI_OBJS) $(LDFLAGS)
