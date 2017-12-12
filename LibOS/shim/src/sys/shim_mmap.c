/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

/* Copyright (C) 2014 Stony Brook University
   This file is part of Graphene Library OS.

   Graphene Library OS is free software: you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Graphene Library OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/*
 * shim_mmap.c
 *
 * Implementation of system call "mmap", "munmap" and "mprotect".
 */

#include <shim_internal.h>
#include <shim_table.h>
#include <shim_handle.h>
#include <shim_vma.h>
#include <shim_fs.h>
#include <shim_profile.h>

#include <pal.h>
#include <pal_error.h>

#include <sys/mman.h>
#include <errno.h>

DEFINE_PROFILE_OCCURENCE(mmap, memory);

void * shim_do_mmap (void * addr, size_t length, int prot, int flags, int fd,
                     off_t offset)
{
    struct shim_handle * hdl = NULL;
    long ret = -ENOMEM;

    if (addr + length < addr) {
        return (void *) -EINVAL;
    }

    assert(!(flags & (VMA_UNMAPPED|VMA_TAINTED)));

    if (flags & MAP_32BIT)
        return (void *) -ENOSYS;

    if ((flags & MAP_FIXED) || addr) {
        struct shim_vma * tmp = NULL;

        if (!lookup_overlap_vma(addr, length, &tmp)) {
            debug("mmap: allowing overlapping MAP_FIXED allocation at %p with length %lu\n",
                  addr, length);

            if (!(flags & MAP_FIXED))
                addr = NULL;
        }
    }

    void * mapped = ALIGN_DOWN((void *) addr);
    void * mapped_end = ALIGN_UP((void *) addr + length);

    addr = mapped;
    length = mapped_end - mapped;

    if (flags & MAP_ANONYMOUS) {
        ret = alloc_anon_vma(&addr, length, prot, flags, NULL);
        if (ret < 0)
            goto err;

        ADD_PROFILE_OCCURENCE(mmap, length);
    } else {
        if (fd < 0) {
            ret = -EINVAL;
            goto err;
        }

        hdl = get_fd_handle(fd, NULL, NULL);
        if (!hdl) {
            ret = -EBADF;
            goto err;
        }

        if (!hdl->fs || !hdl->fs->fs_ops || !hdl->fs->fs_ops->mmap) {
            ret = -ENODEV;
            goto err;
        }

        if ((ret = hdl->fs->fs_ops->mmap(hdl, &addr, length, prot, flags,
                                         offset)) < 0)
            goto err;
    }

    if (hdl)
        put_handle(hdl);
    return addr;

err:
    if (hdl)
        put_handle(hdl);
    return (void *) ret;
}

int shim_do_mprotect (void * addr, size_t len, int prot)
{
    void * mapped = ALIGN_DOWN(addr);
    void * mapped_end = ALIGN_UP(addr + len);

    return protect_vma(mapped, mapped_end - mapped, prot, 0);
}

int shim_do_munmap (void * addr, size_t len)
{
    struct shim_vma * tmp = NULL;

    if (lookup_overlap_vma(addr, len, &tmp) < 0) {
        debug("can't find addr %p - %p in map, quit unmapping\n",
              addr, addr + len);

        /* Really not an error */
        return -EFAULT;
    }

    put_vma(tmp);

    void * mapped = ALIGN_DOWN(addr);
    void * mapped_end = ALIGN_UP(addr + len);

    return free_vma((void *) mapped, mapped_end - mapped, 0);
}
