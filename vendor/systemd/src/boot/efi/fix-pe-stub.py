#!/usr/bin/python3

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
off_opth_NumberOfRvaAndSizes = 108
# Offsets inside section header
off_secth_Name = 0
off_secth_VirtualSize = 8
off_secth_VirtualAddress = 12


def fix_aarch64_pe_stub():
    with open("stub", "r+b") as stub:
        stub.seek(off_pe_pointer)
        pe_off = int.from_bytes(stub.read(4), byteorder='little')
        print('Offset to PE header: 0x{:x}'.format(pe_off))

        stub.seek(pe_off + sz_signature + off_fhead_NumberOfSections)
        num_sect = int.from_bytes(stub.read(2), byteorder='little')
        print('Num sect {}'.format(num_sect))

        # Set number of symbols to 0 (gnu-efi bug?)
        stub.seek(pe_off + sz_signature + off_fhead_NumberOfSymbols)
        stub.write(bytearray(4))

        # Set load size and VMA for .linux and .initrd
        stub.seek(pe_off + sz_signature + sz_coff_fhead +
                  off_opth_NumberOfRvaAndSizes)
        num_dirs = int.from_bytes(stub.read(4), byteorder='little')
        sect_off = (pe_off + sz_signature + sz_coff_fhead + sz_opt_head +
                    8*num_dirs)

        for s in range(num_sect):
            stub.seek(sect_off + s*sz_sect_head)
            print(stub.read(8))


fix_aarch64_pe_stub()
