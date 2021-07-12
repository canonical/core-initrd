/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

#define SETUP_MAGIC             0x53726448      /* "HdrS" */

struct setup_header {
        UINT8  setup_sects;
        UINT16 root_flags;
        UINT32 syssize;
        UINT16 ram_size;
        UINT16 vid_mode;
        UINT16 root_dev;
        UINT16 boot_flag;
        UINT16 jump;
        UINT32 header;
        UINT16 version;
        UINT32 realmode_swtch;
        UINT16 start_sys_seg;
        UINT16 kernel_version;
        UINT8  type_of_loader;
        UINT8  loadflags;
        UINT16 setup_move_size;
        UINT32 code32_start;
        UINT32 ramdisk_image;
        UINT32 ramdisk_size;
        UINT32 bootsect_kludge;
        UINT16 heap_end_ptr;
        UINT8  ext_loader_ver;
        UINT8  ext_loader_type;
        UINT32 cmd_line_ptr;
        UINT32 initrd_addr_max;
        UINT32 kernel_alignment;
        UINT8  relocatable_kernel;
        UINT8  min_alignment;
        UINT16 xloadflags;
        UINT32 cmdline_size;
        UINT32 hardware_subarch;
        UINT64 hardware_subarch_data;
        UINT32 payload_offset;
        UINT32 payload_length;
        UINT64 setup_data;
        UINT64 pref_address;
        UINT32 init_size;
        UINT32 handover_offset;
} __attribute__((packed));

/* adapted from linux' bootparam.h */
struct boot_params {
        UINT8  screen_info[64];         // was: struct screen_info
        UINT8  apm_bios_info[20];       // was: struct apm_bios_info
        UINT8  _pad2[4];
        UINT64 tboot_addr;
        UINT8  ist_info[16];            // was: struct ist_info
        UINT8  _pad3[16];
        UINT8  hd0_info[16];
        UINT8  hd1_info[16];
        UINT8  sys_desc_table[16];      // was: struct sys_desc_table
        UINT8  olpc_ofw_header[16];     // was: struct olpc_ofw_header
        UINT32 ext_ramdisk_image;
        UINT32 ext_ramdisk_size;
        UINT32 ext_cmd_line_ptr;
        UINT8  _pad4[116];
        UINT8  edid_info[128];          // was: struct edid_info
        UINT8  efi_info[32];            // was: struct efi_info
        UINT32 alt_mem_k;
        UINT32 scratch;
        UINT8  e820_entries;
        UINT8  eddbuf_entries;
        UINT8  edd_mbr_sig_buf_entries;
        UINT8  kbd_status;
        UINT8  secure_boot;
        UINT8  _pad5[2];
        UINT8  sentinel;
        UINT8  _pad6[1];
        struct setup_header hdr;
        UINT8  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
        UINT32 edd_mbr_sig_buffer[16];  // was: edd_mbr_sig_buffer[EDD_MBR_SIG_MAX]
        UINT8  e820_table[20*128];      // was: struct boot_e820_entry e820_table[E820_MAX_ENTRIES_ZEROPAGE]
        UINT8  _pad8[48];
        UINT8  eddbuf[6*82];            // was: struct edd_info eddbuf[EDDMAXNR]
        UINT8  _pad9[276];
} __attribute__((packed));

/**/
struct arm64_kernel_header
{
        UINT32 code0;		/* Executable code */
        UINT32 code1;		/* Executable code */
        UINT64 text_offset;    /* Image load offset */
        UINT64 res0;		/* reserved */
        UINT64 res1;		/* reserved */
        UINT64 res2;		/* reserved */
        UINT64 res3;		/* reserved */
        UINT64 res4;		/* reserved */
        UINT32 magic;		/* Magic number, little endian, "ARM\x64" */
        UINT32 hdr_offset;	/* Offset of PE/COFF header */
} __attribute__((packed));

struct pe32_data_directory
{
        UINT32 rva;
        UINT32 size;
}  __attribute__((packed));

struct pe64_optional_header
{
        UINT16 magic;
        UINT8 major_linker_version;
        UINT8 minor_linker_version;
        UINT32 code_size;
        UINT32 data_size;
        UINT32 bss_size;
        UINT32 entry_addr;
        UINT32 code_base;

        UINT64 image_base;

        UINT32 section_alignment;
        UINT32 file_alignment;
        UINT16 major_os_version;
        UINT16 minor_os_version;
        UINT16 major_image_version;
        UINT16 minor_image_version;
        UINT16 major_subsystem_version;
        UINT16 minor_subsystem_version;
        UINT32 reserved;
        UINT32 image_size;
        UINT32 header_size;
        UINT32 checksum;
        UINT16 subsystem;
        UINT16 dll_characteristics;

        UINT64 stack_reserve_size;
        UINT64 stack_commit_size;
        UINT64 heap_reserve_size;
        UINT64 heap_commit_size;

        UINT32 loader_flags;
        UINT32 num_data_directories;

        /* Data directories.  */
        struct pe32_data_directory export_table;
        struct pe32_data_directory import_table;
        struct pe32_data_directory resource_table;
        struct pe32_data_directory exception_table;
        struct pe32_data_directory certificate_table;
        struct pe32_data_directory base_relocation_table;
        struct pe32_data_directory debug;
        struct pe32_data_directory architecture;
        struct pe32_data_directory global_ptr;
        struct pe32_data_directory tls_table;
        struct pe32_data_directory load_config_table;
        struct pe32_data_directory bound_import;
        struct pe32_data_directory iat;
        struct pe32_data_directory delay_import_descriptor;
        struct pe32_data_directory com_runtime_header;
        struct pe32_data_directory reserved_entry;
} __attribute__((packed));

struct pe32_coff_header
{
        UINT16 machine;
        UINT16 num_sections;
        UINT32 time;
        UINT32 symtab_offset;
        UINT32 num_symbols;
        UINT16 optional_header_size;
        UINT16 characteristics;
} __attribute__((packed));

struct arm64_linux_pe_header
{
        UINT32 magic;
        struct pe32_coff_header coff;
        struct pe64_optional_header opt;
} __attribute__((packed));


EFI_STATUS linux_exec(EFI_HANDLE *image,
                      CHAR8 *cmdline, UINTN cmdline_size,
                      UINTN linux_addr,
                      UINTN initrd_addr, UINTN initrd_size);

EFI_STATUS linux_aarch64_exec(EFI_HANDLE image,
                              CHAR8 *cmdline, UINTN cmdline_size,
                              UINTN linux_addr,
                              UINTN initrd_addr, UINTN initrd_size);
