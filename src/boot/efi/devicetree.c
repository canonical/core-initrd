/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <efi.h>

#include "devicetree.h"
#include "missing_efi.h"
#include "util.h"

#define FDT_V1_SIZE (7*4)

static EFI_STATUS devicetree_allocate(struct devicetree_state *state, UINTN size) {
        UINTN pages = DIV_ROUND_UP(size, EFI_PAGE_SIZE);
        EFI_STATUS err;

        assert(state);

        err = BS->AllocatePages(AllocateAnyPages, EfiACPIReclaimMemory, pages, &state->addr);
        if (EFI_ERROR(err))
                return err;

        state->pages = pages;
        return err;
}

static UINTN devicetree_allocated(const struct devicetree_state *state) {
        assert(state);
        return state->pages * EFI_PAGE_SIZE;
}

static EFI_STATUS devicetree_fixup(struct devicetree_state *state, UINTN len) {
        EFI_DT_FIXUP_PROTOCOL *fixup;
        UINTN size;
        EFI_STATUS err;

        assert(state);

        err = LibLocateProtocol(&EfiDtFixupProtocol, (void **)&fixup);
        if (EFI_ERROR(err))
                return log_error_status_stall(EFI_SUCCESS,
                                              L"Could not locate device tree fixup protocol, skipping.");

        size = devicetree_allocated(state);
        err = fixup->Fixup(fixup, PHYSICAL_ADDRESS_TO_POINTER(state->addr), &size,
                           EFI_DT_APPLY_FIXUPS | EFI_DT_RESERVE_MEMORY);
        if (err == EFI_BUFFER_TOO_SMALL) {
                EFI_PHYSICAL_ADDRESS oldaddr = state->addr;
                UINTN oldpages = state->pages;
                void *oldptr = PHYSICAL_ADDRESS_TO_POINTER(state->addr);

                err = devicetree_allocate(state, size);
                if (EFI_ERROR(err))
                        return err;

                CopyMem(PHYSICAL_ADDRESS_TO_POINTER(state->addr), oldptr, len);
                err = BS->FreePages(oldaddr, oldpages);
                if (EFI_ERROR(err))
                        return err;

                size = devicetree_allocated(state);
                err = fixup->Fixup(fixup, PHYSICAL_ADDRESS_TO_POINTER(state->addr), &size,
                                   EFI_DT_APPLY_FIXUPS | EFI_DT_RESERVE_MEMORY);
        }

        return err;
}

EFI_STATUS devicetree_install(struct devicetree_state *state, EFI_FILE *root_dir, CHAR16 *name) {
        _cleanup_(file_closep) EFI_FILE *handle = NULL;
        _cleanup_freepool_ EFI_FILE_INFO *info = NULL;
        UINTN len;
        EFI_STATUS err;

        assert(state);
        assert(root_dir);
        assert(name);

        err = LibGetSystemConfigurationTable(&EfiDtbTableGuid, &state->orig);
        if (EFI_ERROR(err))
                return EFI_UNSUPPORTED;

        err = root_dir->Open(root_dir, &handle, name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
        if (EFI_ERROR(err))
                return err;

        err = get_file_info_harder(handle, &info, NULL);
        if (EFI_ERROR(err))
                return err;
        if (info->FileSize < FDT_V1_SIZE || info->FileSize > 32 * 1024 * 1024)
                /* 32MB device tree blob doesn't seem right */
                return EFI_INVALID_PARAMETER;

        len = info->FileSize;

        err = devicetree_allocate(state, len);
        if (EFI_ERROR(err))
                return err;

        err = handle->Read(handle, &len, PHYSICAL_ADDRESS_TO_POINTER(state->addr));
        if (EFI_ERROR(err))
                return err;

        err = devicetree_fixup(state, len);
        if (EFI_ERROR(err))
                return err;

        return BS->InstallConfigurationTable(&EfiDtbTableGuid, PHYSICAL_ADDRESS_TO_POINTER(state->addr));
}

EFI_STATUS devicetree_install_from_memory(struct devicetree_state *state,
                const void *dtb_buffer, UINTN dtb_length) {

        EFI_STATUS err;

        assert(state);
        assert(dtb_buffer && dtb_length > 0);

        err = LibGetSystemConfigurationTable(&EfiDtbTableGuid, &state->orig);
        if (EFI_ERROR(err))
                return EFI_UNSUPPORTED;

        err = devicetree_allocate(state, dtb_length);
        if (EFI_ERROR(err))
                return err;

        CopyMem(PHYSICAL_ADDRESS_TO_POINTER(state->addr), dtb_buffer, dtb_length);

        err = devicetree_fixup(state, dtb_length);
        if (EFI_ERROR(err))
                return err;

        return BS->InstallConfigurationTable(&EfiDtbTableGuid, PHYSICAL_ADDRESS_TO_POINTER(state->addr));
}

void devicetree_cleanup(struct devicetree_state *state) {
        EFI_STATUS err;

        if (!state->pages)
                return;

        err = BS->InstallConfigurationTable(&EfiDtbTableGuid, state->orig);
        /* don't free the current device tree if we can't reinstate the old one */
        if (EFI_ERROR(err))
                return;

        BS->FreePages(state->addr, state->pages);
        state->pages = 0;
}
