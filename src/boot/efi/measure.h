/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <efi.h>

/* This TPM PCR is where we extend the kernel command line and any passed credentials here. */
#define TPM_PCR_INDEX_KERNEL_PARAMETERS 12U

/* We used to write the kernel command line/credentials into PCR 8, in systemd <= 250. Let's provide for
 * some compatibility. (Remove in 2023!) */
#if EFI_TPM_PCR_COMPAT
#define TPM_PCR_INDEX_KERNEL_PARAMETERS_COMPAT 8U
#else
#define TPM_PCR_INDEX_KERNEL_PARAMETERS_COMPAT UINT32_MAX
#endif

/* This TPM PCR is where most Linux infrastructure extends the initrd binary images into, and so do we. */
#define TPM_PCR_INDEX_INITRD 4U

#if ENABLE_TPM

BOOLEAN tpm_present(void);
EFI_STATUS tpm_log_event(UINT32 pcrindex, EFI_PHYSICAL_ADDRESS buffer, UINTN buffer_size, const CHAR16 *description);
EFI_STATUS tpm_log_load_options(const CHAR16 *cmdline);

#else

static inline BOOLEAN tpm_present(void) {
        return FALSE;
}

static inline EFI_STATUS tpm_log_event(UINT32 pcrindex, EFI_PHYSICAL_ADDRESS buffer, UINTN buffer_size, const CHAR16 *description) {
        return EFI_SUCCESS;
}

static inline EFI_STATUS tpm_log_load_options(const CHAR16 *cmdline) {
        return EFI_SUCCESS;
}

#endif
