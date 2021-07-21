/* SPDX-License-Identifier: LGPL-2.1+ */

#include <efi.h>
#include <efilib.h>
#include <libfdt.h>

#include "linux.h"
#include "util.h"

#ifdef __i386__
#define __regparm0__ __attribute__((regparm(0)))
#else
#define __regparm0__
#endif

typedef VOID(*handover_f)(VOID *image, EFI_SYSTEM_TABLE *table, struct boot_params *params) __regparm0__;
static VOID linux_efi_handover(EFI_HANDLE image, struct boot_params *params) {
        handover_f handover;
        UINTN start = (UINTN)params->hdr.code32_start;

#ifdef __x86_64__
        asm volatile ("cli");
        start += 512;
#endif
        handover = (handover_f)(start + params->hdr.handover_offset);
        handover(image, ST, params);
}

EFI_STATUS linux_exec(EFI_HANDLE *image,
                      CHAR8 *cmdline, UINTN cmdline_len,
                      UINTN linux_addr,
                      UINTN initrd_addr, UINTN initrd_size) {
        struct boot_params *image_params;
        struct boot_params *boot_params;
        UINT8 setup_sectors;
        EFI_PHYSICAL_ADDRESS addr;
        EFI_STATUS err;

        image_params = (struct boot_params *) linux_addr;

        if (image_params->hdr.boot_flag != 0xAA55 ||
            image_params->hdr.header != SETUP_MAGIC ||
            image_params->hdr.version < 0x20b ||
            !image_params->hdr.relocatable_kernel)
                return EFI_LOAD_ERROR;

        boot_params = (struct boot_params *) 0xFFFFFFFF;
        err = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
                                EFI_SIZE_TO_PAGES(0x4000), (EFI_PHYSICAL_ADDRESS*) &boot_params);
        if (EFI_ERROR(err))
                return err;

        ZeroMem(boot_params, 0x4000);
        CopyMem(&boot_params->hdr, &image_params->hdr, sizeof(struct setup_header));
        boot_params->hdr.type_of_loader = 0xff;
        setup_sectors = image_params->hdr.setup_sects > 0 ? image_params->hdr.setup_sects : 4;
        boot_params->hdr.code32_start = (UINT32)linux_addr + (setup_sectors + 1) * 512;

        if (cmdline) {
                addr = 0xA0000;
                err = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiLoaderData,
                                        EFI_SIZE_TO_PAGES(cmdline_len + 1), &addr);
                if (EFI_ERROR(err))
                        return err;
                CopyMem((VOID *)(UINTN)addr, cmdline, cmdline_len);
                ((CHAR8 *)(UINTN)addr)[cmdline_len] = 0;
                boot_params->hdr.cmd_line_ptr = (UINT32)addr;
        }

        boot_params->hdr.ramdisk_image = (UINT32)initrd_addr;
        boot_params->hdr.ramdisk_size = (UINT32)initrd_size;

        linux_efi_handover(image, boot_params);
        return EFI_LOAD_ERROR;
}

/* Create new fdt, either empty or with the content of old_fdt if not null */
static void *create_new_fdt(void *old_fdt, int fdt_sz) {
        EFI_STATUS err;
        void *fdt = (void *) 0xFFFFFFFFUL;
        int ret;

        err = uefi_call_wrapper(BS->AllocatePages, 4,
                                AllocateMaxAddress,
                                EfiACPIReclaimMemory,
                                EFI_SIZE_TO_PAGES(fdt_sz),
                                (EFI_PHYSICAL_ADDRESS*) &fdt);
        if (EFI_ERROR(err)) {
                Print(L"Cannot allocate when creating fdt\n");
                return 0;
        }

        if (old_fdt) {
                ret = fdt_open_into(old_fdt, fdt, fdt_sz);
                if (ret != 0) {
                        Print(L"Error %d when copying fdt\n", ret);
                        return 0;
                }
        } else {
                ret = fdt_create_empty_tree(fdt, fdt_sz);
                if (ret != 0) {
                        Print(L"Error %d when creating empty fdt\n", ret);
                        return 0;
                }
        }

        /* Set in EFI configuration table */
        err = uefi_call_wrapper(BS->InstallConfigurationTable, 2,
                                &EfiDtbTableGuid, fdt);
        if (EFI_ERROR(err)) {
                Print(L"Cannot set fdt in EFI configuration\n");
                return 0;
        }

        return fdt;
}

static void *open_fdt(void) {
        EFI_STATUS status;
        void *fdt;
        unsigned long fdt_size;

        /* Look for a device tree configuration table entry. */
        status = LibGetSystemConfigurationTable(&EfiDtbTableGuid,
                                                (VOID**) &fdt);
        if (EFI_ERROR(status)) {
                Print(L"DTB table not found, create new one\n");
                fdt = create_new_fdt(NULL, 2048);
                if (!fdt)
                        return 0;
        }

        if (fdt_check_header(fdt) != 0) {
		Print(L"Invalid header detected on UEFI supplied FDT\n");
		return 0;
	}
	fdt_size = fdt_totalsize(fdt);
        Print(L"Size of fdt is %lu\n", fdt_size);

        return fdt;
}

static int update_chosen(void *fdt, UINTN initrd_addr, UINTN initrd_size) {
        uint64_t initrd_start, initrd_end;
	int ret, node;

        node = fdt_subnode_offset(fdt, 0, "chosen");
	if (node < 0) {
		node = fdt_add_subnode(fdt, 0, "chosen");
		if (node < 0) {
			/* 'node' is an error code when negative: */
			ret = node;
                        Print(L"Error creating chosen\n");
			return ret;
		}
	}

        initrd_start = cpu_to_fdt64(initrd_addr);
        initrd_end = cpu_to_fdt64(initrd_addr + initrd_size);

        ret = fdt_setprop(fdt, node, "linux,initrd-start",
                          &initrd_start, sizeof initrd_start);
        if (ret) {
                Print(L"Cannot create initrd-start property\n");
                return ret;
        }

        ret = fdt_setprop(fdt, node, "linux,initrd-end",
                          &initrd_end, sizeof initrd_end);
        if (ret) {
                Print(L"Cannot create initrd-end property\n");
                return ret;
        }

        return 0;
}

#define FDT_EXTRA_SIZE 0x400

// Update fdt /chosen node with initrd address and size
static void update_fdt(UINTN initrd_addr, UINTN initrd_size) {
        void *fdt;

        fdt = open_fdt();
        if (fdt == 0)
                return;

        if (update_chosen(fdt, initrd_addr, initrd_size) == -FDT_ERR_NOSPACE) {
                /* Copy to new tree and re-try */
                Print(L"Not enough space, creating a new fdt\n");
                fdt = create_new_fdt(NULL, fdt_totalsize(fdt) + FDT_EXTRA_SIZE);
                if (!fdt)
                        return;
                update_chosen(fdt, initrd_addr, initrd_size);
        }
}

// linux_addr: .linux section address
EFI_STATUS linux_aarch64_exec(EFI_HANDLE image,
                              CHAR8 *cmdline, UINTN cmdline_len,
                              UINTN linux_addr,
                              UINTN initrd_addr, UINTN initrd_size) {
        struct arm64_kernel_header *hdr;
        struct arm64_linux_pe_header *pe;
        handover_f handover;

        if (initrd_size != 0)
                update_fdt(initrd_addr, initrd_size);

        hdr = (struct arm64_kernel_header *) linux_addr;

        pe = (void *)((UINTN)linux_addr + hdr->hdr_offset);
        handover = (handover_f)((UINTN)linux_addr + pe->opt.entry_addr);

        Print(L"Calling now EFI kernel stub\n");

        handover(image, ST, image);

        return EFI_LOAD_ERROR;
}
