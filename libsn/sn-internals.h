/* 
 * Copyright (C) 2002 Red Hat, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef __LF_INTERNALS_H__
#define __LF_INTERNALS_H__

#include <liblf/lf-common.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <liblf/lf-list.h>
#include <liblf/lf-xutils.h>

LF_BEGIN_DECLS

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*) 0)
#endif

/* --- From lf-launcher.c --- */
lf_bool_t lf_internal_launcher_process_event (LfDisplay *display,
                                              XEvent    *xevent);

/* --- From lf-monitor.c --- */
lf_bool_t lf_internal_monitor_process_event (LfDisplay *display,
                                             XEvent    *xevent);

/* --- From lf-util.c --- */
lf_bool_t lf_internal_utf8_validate (const char *str,
                                     int         max_len);
char*     lf_internal_strdup        (const char *str);
char*     lf_internal_strndup       (const char *str,
                                     int         n);
void      lf_internal_strfreev      (char      **strings);

unsigned long lf_internal_string_to_ulong (const char* str);

/* --- From lf-xmessages.c --- */
lf_bool_t lf_internal_xmessage_process_event (LfDisplay *display,
                                              XEvent    *xevent);

LF_END_DECLS

#endif /* __LF_INTERNALS_H__ */
