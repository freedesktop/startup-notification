/* Launchee API - if you are a program started by other programs */
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


#ifndef __LF_LAUNCHEE_H__
#define __LF_LAUNCHEE_H__

#include <liblf/lf-common.h>

LF_BEGIN_DECLS

typedef struct LfLauncheeContext LfLauncheeContext;

LfLauncheeContext* lf_launchee_context_new                  (LfDisplay         *display,
                                                             const char        *launch_id,
                                                             Window             launch_window);
LfLauncheeContext* lf_launchee_context_new_from_environment (LfDisplay         *display);
void               lf_launchee_context_ref                  (LfLauncheeContext *context);
void               lf_launchee_context_unref                (LfLauncheeContext *context);
Window             lf_launchee_context_get_launch_window    (LfLauncheeContext *context);
const char*        lf_launchee_context_get_launch_id        (LfLauncheeContext *context);
void               lf_launchee_context_pulse                (LfLauncheeContext *context);
void               lf_launchee_context_cancel               (LfLauncheeContext *context);
void               lf_launchee_context_complete             (LfLauncheeContext *context);
void               lf_launchee_context_setup_window         (LfLauncheeContext *context,
                                                             Window             xwindow);

LF_END_DECLS

#endif /* __LF_LAUNCHEE_H__ */
