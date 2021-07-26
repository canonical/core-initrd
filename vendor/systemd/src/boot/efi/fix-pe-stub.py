#!/usr/bin/python3
import argparse
import subprocess

# Sizes of different parts of th PE+ file
sz_signature = 4
sz_coff_fhead = 20
sz_opt_head = 112
sz_sect_head = 40

# File offset to the PE header pointer
off_pe_pointer = 0x3c
# Offsets inside file header
off_fhead_NumberOfSections = 2
off_fhead_NumberOfSymbols = 12
# Offsets inside optional headers
off_opth_SizeOfInitializedData = 8
off_opth_SectionAlignment = 32
off_opth_FileAlignment = 36
off_opth_SizeOfImage = 56
off_opth_NumberOfRvaAndSizes = 108
# Offsets inside section header
off_secth_Name = 0
off_secth_VirtualSize = 8
off_secth_VirtualAddress = 12
off_secth_SizeOfRawData = 16


def align_to_size(address, size):
    rest = address % size
    if rest:
        address -= rest
        address += size
    return address


def fix_aarch64_pe_stub(stub, kernel, initramfs, out_file):
    with open(stub, "r+b") as stub_f:

        stub_f.seek(off_pe_pointer)
        pe_off = int.from_bytes(stub_f.read(4), 'little')

        stub_f.seek(pe_off + sz_signature + off_fhead_NumberOfSections)
        num_sect = int.from_bytes(stub_f.read(2), 'little')

        # Set number of symbols to 0 (gnu-efi sets it wrongly to 1) as required
        # by the PE specification for images, so llvm-objcopy does not complain
        stub_f.seek(pe_off + sz_signature + off_fhead_NumberOfSymbols)
        stub_f.write(bytearray(4))

    # Copy now kernel/initramfs to the ouput file
    subprocess.check_call(
        [
            'llvm-objcopy',
            '--add-section',
            '.linux={}'.format(kernel),
            '--add-section',
            '.initrd={}'.format(initramfs),
            stub,
            out_file,
        ]
    )

    # We need to fill properly some values in the PE/COFF headers due to llvm
    # bugs.
    with open(out_file, "r+b") as out_f:
        out_f.seek(off_pe_pointer)
        pe_off = int.from_bytes(out_f.read(4), 'little')

        out_f.seek(pe_off + sz_signature + off_fhead_NumberOfSections)
        num_sect = int.from_bytes(out_f.read(2), 'little')

        # Get alignment values
        out_f.seek(pe_off + sz_signature + sz_coff_fhead +
                   off_opth_SectionAlignment)
        section_align = int.from_bytes(out_f.read(4), 'little')
        file_align = int.from_bytes(out_f.read(4), 'little')

        out_f.seek(pe_off + sz_signature + sz_coff_fhead +
                   off_opth_NumberOfRvaAndSizes)
        num_dirs = int.from_bytes(out_f.read(4), 'little')
        sect_off = (pe_off + sz_signature + sz_coff_fhead + sz_opt_head +
                    8*num_dirs)

        # Set load size and VMA for .linux and .initrd
        # We assume the order of sections is
        # .test, .data, .linux, .initrd
        data_sz = 0
        data_va = 0
        disk_data = 0
        for s in range(num_sect):
            out_f.seek(sect_off)
            sect_name = out_f.read(8)

            if s == 0:
                out_f.seek(sect_off + off_secth_VirtualAddress)
                vma_start = int.from_bytes(out_f.read(4), 'little')

            if sect_name == b'.data\x00\x00\x00':
                out_f.seek(sect_off + off_secth_VirtualSize)
                data_sz = int.from_bytes(out_f.read(4), 'little')
                data_va = int.from_bytes(out_f.read(4), 'little')
                disk_data += data_sz
                next_va = data_va + align_to_size(data_sz, section_align)

            if sect_name == b'.linux\x00\x00' or sect_name == b'.initrd\x00':
                out_f.seek(sect_off + off_secth_SizeOfRawData)
                raw_sect_sz = int.from_bytes(out_f.read(4), 'little')
                out_f.seek(sect_off + off_secth_VirtualSize)
                out_f.write(raw_sect_sz.to_bytes(4, 'little'))
                out_f.write(next_va.to_bytes(4, 'little'))
                # Now rewrite SizeOfRawData with an aligned value
                disk_size = align_to_size(raw_sect_sz, file_align)
                out_f.write(disk_size.to_bytes(4, 'little'))
                disk_data += disk_size

                next_va += align_to_size(raw_sect_sz, section_align)

            sect_off += sz_sect_head

        # Set size of data sections
        out_f.seek(pe_off + sz_signature + sz_coff_fhead +
                   off_opth_SizeOfInitializedData)
        out_f.write(disk_data.to_bytes(4, 'little'))

        # Size in memory (sect_off points now at end of headers)
        image_sz = next_va - vma_start + align_to_size(sect_off, section_align)
        out_f.seek(pe_off + sz_signature + sz_coff_fhead +
                   off_opth_SizeOfImage)
        out_f.write(image_sz.to_bytes(4, 'little'))


def main():
    parser = argparse.ArgumentParser(usage='Create kernel.efi for aarch64')
    parser.add_argument('stub', help='Stub with systemd EFI stub')
    parser.add_argument('kernel', help='Linux kernel image')
    parser.add_argument('initramfs', help='Initramfs')
    parser.add_argument('out_file', help='Out EFI executable')
    args = parser.parse_args()

    fix_aarch64_pe_stub(args.stub, args.kernel, args.initramfs, args.out_file)


if __name__ == "__main__":
    main()
