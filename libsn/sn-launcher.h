/* Launcher API - if you are a program that starts other programs */
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


#ifndef __LF_LAUNCHER_H__
#define __LF_LAUNCHER_H__

#include <liblf/lf-common.h>

LF_BEGIN_DECLS

typedef struct LfLauncherContext LfLauncherContext;
typedef struct LfLauncherEvent   LfLauncherEvent;

typedef void (* LfLauncherEventFunc) (LfLauncherEvent *event,
                                      void            *user_data);

typedef enum
{
  LF_LAUNCHER_EVENT_CANCELED,
  LF_LAUNCHER_EVENT_COMPLETED,
  LF_LAUNCHER_EVENT_PULSE
} LfLauncherEventType;

LfLauncherContext* lf_launcher_context_new   (LfDisplay           *display,
                                              LfLauncherEventFunc  event_func,
                                              void                *event_func_data,
                                              LfFreeFunc           free_data_func);
void        lf_launcher_context_ref               (LfLauncherContext *context);
void        lf_launcher_context_unref             (LfLauncherContext *context);


void        lf_launcher_context_initiate          (LfLauncherContext *context,
                                                   const char        *launcher_name,
                                                   const char        *launchee_name,
                                                   Time               timestamp);
Window      lf_launcher_context_get_launch_window (LfLauncherContext *context);
const char* lf_launcher_context_get_launch_id     (LfLauncherContext *context);
lf_bool_t   lf_launcher_context_get_initiated     (LfLauncherContext *context);
lf_bool_t   lf_launcher_context_get_canceled      (LfLauncherContext *context);
lf_bool_t   lf_launcher_context_get_completed     (LfLauncherContext *context);
void        lf_launcher_context_cancel            (LfLauncherContext *context);
void        lf_launcher_context_complete          (LfLauncherContext *context);

void        lf_launcher_context_setup_child_process (LfLauncherContext *context);

void lf_launcher_context_set_launch_type           (LfLauncherContext *context,
                                                    LfLaunchType       type);
void lf_launcher_context_set_geometry_window       (LfLauncherContext *context,
                                                    Window             xwindow);
void lf_launcher_context_set_supports_cancel       (LfLauncherContext *context,
                                                    lf_bool_t          supports_cancel);
void lf_launcher_context_set_launch_name           (LfLauncherContext *context,
                                                    const char        *name);
void lf_launcher_context_set_launch_description    (LfLauncherContext *context,
                                                    const char        *description);
void lf_launcher_context_set_launch_workspace      (LfLauncherContext *context,
                                                    int                workspace);
void lf_launcher_context_set_legacy_resource_class (LfLauncherContext *context,
                                                    const char        *klass);
void lf_launcher_context_set_legacy_resource_name  (LfLauncherContext *context,
                                                    const char        *name);
void lf_launcher_context_set_legacy_window_title   (LfLauncherContext *context,
                                                    const char        *title);
void lf_launcher_context_set_binary_name           (LfLauncherContext *context,
                                                    const char        *name);
void lf_launcher_context_set_pid                   (LfLauncherContext *context,
                                                    int                pid);
void lf_launcher_context_set_icon_name             (LfLauncherContext *context,
                                                    const char        *name);

LfLauncherEvent*    lf_launcher_event_copy        (LfLauncherEvent *event);
void                lf_launcher_event_ref         (LfLauncherEvent *event);
void                lf_launcher_event_unref       (LfLauncherEvent *event);
LfLauncherEventType lf_launcher_event_get_type    (LfLauncherEvent *event);
LfLauncherContext*  lf_launcher_event_get_context (LfLauncherEvent *event);
Time                lf_launcher_event_get_time    (LfLauncherEvent *event);

LF_END_DECLS

#endif /* __LF_LAUNCHER_H__ */
