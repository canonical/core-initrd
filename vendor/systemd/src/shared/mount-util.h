/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <mntent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alloc-util.h"
#include "dissect-image.h"
#include "errno-util.h"
#include "macro.h"

/* 4MB for contents of regular files, 1m inodes for directories, symbolic links and device nodes, using
 * large storage array systems as a baseline */
#define TMPFS_LIMITS_DEV             ",size=4m,nr_inodes=1m"

/* Very little, if any use expected */
#define TMPFS_LIMITS_EMPTY_OR_ALMOST ",size=4m,nr_inodes=1k"
#define TMPFS_LIMITS_SYS             TMPFS_LIMITS_EMPTY_OR_ALMOST
#define TMPFS_LIMITS_SYS_FS_CGROUP   TMPFS_LIMITS_EMPTY_OR_ALMOST

/* On an extremely small device with only 256MB of RAM, 20% of RAM should be enough for the re-execution of
 * PID1 because 16MB of free space is required. */
#define TMPFS_LIMITS_RUN             ",size=20%,nr_inodes=800k"

/* The limit used for various nested tmpfs mounts, in particular for guests started by systemd-nspawn.
 * 10% of RAM (using 16GB of RAM as a baseline) translates to 400k inodes (assuming 4k each) and 25%
 * translates to 1M inodes.
 * (On the host, /tmp is configured through a .mount unit file.) */
#define NESTED_TMPFS_LIMITS          ",size=10%,nr_inodes=400k"

/* More space for volatile root and /var */
#define TMPFS_LIMITS_VAR             ",size=25%,nr_inodes=1m"
#define TMPFS_LIMITS_ROOTFS          TMPFS_LIMITS_VAR
#define TMPFS_LIMITS_VOLATILE_STATE  TMPFS_LIMITS_VAR

int mount_fd(const char *source, int target_fd, const char *filesystemtype, unsigned long mountflags, const void *data);
int mount_nofollow(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);

int repeat_unmount(const char *path, int flags);
int umount_recursive(const char *target, int flags);

int bind_remount_recursive_with_mountinfo(const char *prefix, unsigned long new_flags, unsigned long flags_mask, char **deny_list, FILE *proc_self_mountinfo);
static inline int bind_remount_recursive(const char *prefix, unsigned long new_flags, unsigned long flags_mask, char **deny_list) {
        return bind_remount_recursive_with_mountinfo(prefix, new_flags, flags_mask, deny_list, NULL);
}

int bind_remount_one_with_mountinfo(const char *path, unsigned long new_flags, unsigned long flags_mask, FILE *proc_self_mountinfo);

int mount_move_root(const char *path);

DEFINE_TRIVIAL_CLEANUP_FUNC_FULL(FILE*, endmntent, NULL);
#define _cleanup_endmntent_ _cleanup_(endmntentp)

int mount_verbose_full(
                int error_log_level,
                const char *what,
                const char *where,
                const char *type,
                unsigned long flags,
                const char *options,
                bool follow_symlink);

static inline int mount_follow_verbose(
                int error_log_level,
                const char *what,
                const char *where,
                const char *type,
                unsigned long flags,
                const char *options) {
        return mount_verbose_full(error_log_level, what, where, type, flags, options, true);
}

static inline int mount_nofollow_verbose(
                int error_log_level,
                const char *what,
                const char *where,
                const char *type,
                unsigned long flags,
                const char *options) {
        return mount_verbose_full(error_log_level, what, where, type, flags, options, false);
}

int umount_verbose(
                int error_log_level,
                const char *where,
                int flags);

int mount_option_mangle(
                const char *options,
                unsigned long mount_flags,
                unsigned long *ret_mount_flags,
                char **ret_remaining_options);

int mode_to_inaccessible_node(const char *runtime_dir, mode_t mode, char **dest);
int mount_flags_to_string(unsigned long flags, char **ret);

/* Useful for usage with _cleanup_(), unmounts, removes a directory and frees the pointer */
static inline char* umount_and_rmdir_and_free(char *p) {
        PROTECT_ERRNO;
        if (p) {
                (void) umount_recursive(p, 0);
                (void) rmdir(p);
        }
        return mfree(p);
}
DEFINE_TRIVIAL_CLEANUP_FUNC(char*, umount_and_rmdir_and_free);

int bind_mount_in_namespace(pid_t target, const char *propagate_path, const char *incoming_path, const char *src, const char *dest, bool read_only, bool make_file_or_directory);
int mount_image_in_namespace(pid_t target, const char *propagate_path, const char *incoming_path, const char *src, const char *dest, bool read_only, bool make_file_or_directory, const MountOptions *options);

int make_mount_point(const char *path);

typedef enum RemountIdmapFlags {
        /* Include a mapping from UID_MAPPED_ROOT (i.e. UID 2^31-2) on the backing fs to UID 0 on the
         * uidmapped fs. This is useful to ensure that the host root user can safely add inodes to the
         * uidmapped fs (which otherwise wouldn't work as the host root user is not defined on the uidmapped
         * mount and any attempts to create inodes will then be refused with EOVERFLOW). The idea is that
         * these inodes are quickly re-chown()ed to more suitable UIDs/GIDs. Any code that intends to be able
         * to add inodes to file systems mapped this way should set this flag, but given it comes with
         * certain security implications defaults to off, and requires explicit opt-in. */
        REMOUNT_IDMAP_HOST_ROOT = 1 << 0,
} RemountIdmapFlags;

int remount_idmap(const char *p, uid_t uid_shift, uid_t uid_range, RemountIdmapFlags flags);

/* Creates a mount point (not parents) based on the source path or stat - ie, a file or a directory */
int make_mount_point_inode_from_stat(const struct stat *st, const char *dest, mode_t mode);
int make_mount_point_inode_from_path(const char *source, const char *dest, mode_t mode);
