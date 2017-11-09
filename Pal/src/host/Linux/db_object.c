/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

/* Copyright (C) 2014 OSCAR lab, Stony Brook University
   This file is part of Graphene Library OS.

   Graphene Library OS is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Graphene Library OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/*
 * db_object.c
 *
 * This file contains APIs for closing or polling PAL handles.
 */

#include "pal_defs.h"
#include "pal_linux_defs.h"
#include "pal.h"
#include "pal_internal.h"
#include "pal_linux.h"
#include "pal_error.h"
#include "pal_debug.h"
#include "api.h"

#include <linux/time.h>
#include <linux/wait.h>
#include <atomic.h>
#include <cmpxchg.h>
#include <asm/errno.h>
#include <select.h>

#define DEFAULT_QUANTUM 500

#ifndef FIONREAD
# define FIONREAD 0x541B
#endif

/* internally to wait for one object. Also used as a shortcut to wait
 *  on events and semaphores.
 *
 *  Returns 0 on success, negative value on failure (e.g., -PAL_ERROR_TRYAGAIN)
 */
static int _DkObjectWaitOne (PAL_HANDLE handle, uint64_t timeout)
{
    /* only for all these handle which has a file descriptor, or
       a eventfd. events and semaphores will skip this part */
    if (HANDLE_HDR(handle)->flags & HAS_FDS) {
        struct timespec timeout_ts;

        if (timeout >= 0) {
            long sec = (unsigned long) timeout / 1000000;
            long microsec = (unsigned long) timeout - (sec * 1000000);

            timeout_ts.tv_sec = sec;
            timeout_ts.tv_nsec = microsec * 1000;
        }

        __kernel_fd_set rfds, wfds, efds;
        __FD_ZERO(&rfds);
        __FD_ZERO(&wfds);
        __FD_ZERO(&efds);
        int nfds = 0;
        for (int i = 0 ; i < MAX_FDS ; i++) {
            PAL_FLG flags = HANDLE_HDR(handle)->flags >> i;

            if (!(flags & 00011))
                continue;

            PAL_IDX fd = HANDLE_HDR(handle)->fds[i];

            if (fd == PAL_IDX_POISON)
                continue;

            if ((flags & 01001) == 00001) {
                __FD_SET(fd, &rfds);
                __FD_SET(fd, &efds);
            }

            if ((flags & 01110) == 00010) {
                __FD_SET(fd, &wfds);
                __FD_SET(fd, &efds);
            }

            if (__FD_ISSET(fd, &efds)) {
                if (nfds <= fd)
                    nfds = fd + 1;
            }
        }

        if (!nfds)
            return -PAL_ERROR_TRYAGAIN;

        int ret = INLINE_SYSCALL(pselect6, 6, nfds, &rfds, &wfds, &efds,
                                 timeout >= 0 ? &timeout_ts : NULL, NULL);

        if (IS_ERR(ret))
            switch (ERRNO(ret)) {
                case EINTR:
                case ERESTART:
                    return -PAL_ERROR_INTERRUPTED;
                default:
                    return unix_to_pal_error(ERRNO(ret));
            }

        if (!ret)
            return -PAL_ERROR_TRYAGAIN;

        for (int i = 0 ; i < MAX_FDS ; i++) {
            PAL_FLG flags = HANDLE_HDR(handle)->flags >> i;

            if (!(flags & 00011))
                continue;

            PAL_IDX fd = HANDLE_HDR(handle)->fds[i];

            if (fd == PAL_IDX_POISON)
                continue;

            if (!__FD_ISSET(fd, &rfds) &&
                !__FD_ISSET(fd, &wfds) &&
                !__FD_ISSET(fd, &efds))
                continue;

            if (__FD_ISSET(fd, &rfds)) {
                if (IS_HANDLE_TYPE(handle, pipe) ||
                    IS_HANDLE_TYPE(handle, pipeprv) ||
                    IS_HANDLE_TYPE(handle, tcp) ||
                    IS_HANDLE_TYPE(handle, udp) ||
                    IS_HANDLE_TYPE(handle, process)) {

                    /* check if closed */
                    uint64_t val = 0;
                    ret = INLINE_SYSCALL(ioctl, 3, fd, FIONREAD, &val);
                    if (IS_ERR(ret) || !val) {
                        HANDLE_HDR(handle)->flags |= ERROR(i);
                    }
                }
            }

            if (__FD_ISSET(fd, &wfds))
                HANDLE_HDR(handle)->flags |= WRITEABLE(i);
            if (__FD_ISSET(fd, &efds))
                HANDLE_HDR(handle)->flags |= ERROR(i);
        }

        return 0;
    }

    const struct handle_ops * ops = HANDLE_OPS(handle);

    if (!ops->wait)
        return -PAL_ERROR_NOTSUPPORT;

    return ops->wait(handle, timeout);
}

/* _DkObjectsWaitAny for internal use. The function wait for any of the handle
   in the handle array. timeout can be set for the wait. */
int _DkObjectsWaitAny (int count, PAL_HANDLE * handleArray, uint64_t timeout,
                       PAL_HANDLE * polled)
{
    if (count <= 0)
        return 0;

    if (count == 1) {
        int rv = _DkObjectWaitOne(handleArray[0], timeout);
        if (rv == 0)
            *polled = handleArray[0];
        return rv;
    }

    __kernel_fd_set rfds, wfds, efds;
    __FD_ZERO(&rfds);
    __FD_ZERO(&wfds);
    __FD_ZERO(&efds);
    int n, i, ret, nfds = 0;

    for (n = 0 ; n < count ; n++) {
        PAL_HANDLE handle = handleArray[n];
        if (!handle)
            continue;

        for (i = 0 ; i < MAX_FDS ; i++) {
            PAL_FLG flags = HANDLE_HDR(handle)->flags >> i;

            if (!(flags & 00011))
                continue;

            PAL_IDX fd = HANDLE_HDR(handle)->fds[i];

            if (fd == PAL_IDX_POISON)
                continue;

            if ((flags & 01001) == 00001) {
                __FD_SET(fd, &rfds);
                __FD_SET(fd, &efds);
            }

            if ((flags & 01110) == 00010) {
                __FD_SET(fd, &wfds);
                __FD_SET(fd, &efds);
            }

            if (__FD_ISSET(fd, &efds)) {
                if (nfds <= fd)
                    nfds = fd + 1;
            }
        }
    }

    if (!nfds)
        return -PAL_ERROR_TRYAGAIN;

    struct timespec timeout_ts;

    if (timeout >= 0) {
        long sec = (unsigned long) timeout / 1000000;
        long microsec = (unsigned long) timeout - (sec * 1000000);
        timeout_ts.tv_sec = sec;
        timeout_ts.tv_nsec = microsec * 1000;
    }

    ret = INLINE_SYSCALL(pselect6, 6, nfds, &rfds, &wfds, &efds,
                         timeout >= 0 ? &timeout_ts : NULL, NULL);

    if (IS_ERR(ret))
        switch (ERRNO(ret)) {
            case EINTR:
            case ERESTART:
                return -PAL_ERROR_INTERRUPTED;
            default:
                return unix_to_pal_error(ERRNO(ret));
        }

    if (!ret)
        return -PAL_ERROR_TRYAGAIN;

    PAL_HANDLE polled_hdl = NULL;

    for (n = 0 ; n < count ; n++) {
        PAL_HANDLE handle = handleArray[n];
        if (!handle)
            continue;

        for (i = 0 ; i < MAX_FDS ; i++) {
            PAL_FLG flags = HANDLE_HDR(handle)->flags >> i;

            if (!(flags & 00011))
                continue;

            PAL_IDX fd = HANDLE_HDR(handle)->fds[i];

            if (fd == PAL_IDX_POISON)
                continue;

            if (!__FD_ISSET(fd, &rfds) &&
                !__FD_ISSET(fd, &wfds) &&
                !__FD_ISSET(fd, &efds))
                continue;

            if (__FD_ISSET(fd, &rfds)) {
                if (IS_HANDLE_TYPE(handle, pipe) ||
                    IS_HANDLE_TYPE(handle, pipeprv) ||
                    IS_HANDLE_TYPE(handle, tcp) ||
                    IS_HANDLE_TYPE(handle, udp) ||
                    IS_HANDLE_TYPE(handle, process)) {

                    /* check if closed */
                    uint64_t val = 0;
                    ret = INLINE_SYSCALL(ioctl, 3, fd, FIONREAD, &val);
                    if (IS_ERR(ret) || !val) {
                        HANDLE_HDR(handle)->flags |= ERROR(i);
                    }
                }
            }

            polled_hdl = polled_hdl ? : handle;

            if (__FD_ISSET(fd, &wfds))
                HANDLE_HDR(handle)->flags |= WRITEABLE(i);
            if (__FD_ISSET(fd, &efds))
                HANDLE_HDR(handle)->flags |= ERROR(i);
        }
    }

    *polled = polled_hdl;
    return polled_hdl ? 0 : -PAL_ERROR_TRYAGAIN;
}

#if TRACE_HEAP_LEAK == 1

PAL_HANDLE heap_alloc_head;
PAL_LOCK   heap_alloc_trace_lock = LOCK_INIT;

HEAP_ALLOC_RECORD * collect_heap_alloc_records (PAL_NUM max_records)
{
    HEAP_ALLOC_RECORD * records =
            malloc(sizeof(HEAP_ALLOC_RECORD) * max_records);

    if (!records)
        return NULL;

    memset(records, 0, sizeof(HEAP_ALLOC_RECORD) * max_records);

    _DkInternalLock(&heap_alloc_trace_lock);

    PAL_HANDLE ptr = heap_alloc_head;
    int nrecords = 0, i;

    for (; ptr ; ptr = ptr->hdr.heap_trace.next) {
        assert(!ptr->hdr.heap_trace.next ||
               ptr->hdr.heap_trace.next->hdr.heap_trace.pprev ==
               &ptr->hdr.heap_trace.next);

        for (i = 0 ; i < nrecords ; i++)
            if (ptr->hdr.heap_trace.caller == records[i].caller) {
                records[i].count++;
                break;
            }

        if (i == nrecords) {
            if (nrecords == max_records) break;
            records[nrecords].caller = ptr->hdr.heap_trace.caller;
            records[nrecords].count = 1;
            nrecords++;
        }
    }

    _DkInternalUnlock(&heap_alloc_trace_lock);

    return records;
}

#endif /* TRACE_HEAP_LEAK == 0 */
