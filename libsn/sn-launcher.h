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


#ifndef __SN_LAUNCHER_H__
#define __SN_LAUNCHER_H__

#include <libsn/sn-common.h>

SN_BEGIN_DECLS

typedef struct SnLauncherContext SnLauncherContext;
typedef struct SnLauncherEvent   SnLauncherEvent;

typedef void (* SnLauncherEventFunc) (SnLauncherEvent *event,
                                      void            *user_data);

typedef enum
{
  SN_LAUNCHER_EVENT_CANCELED,
  SN_LAUNCHER_EVENT_COMPLETED,
  SN_LAUNCHER_EVENT_PULSE
} SnLauncherEventType;

SnLauncherContext* sn_launcher_context_new   (SnDisplay           *display,
                                              SnLauncherEventFunc  event_func,
                                              void                *event_func_data,
                                              SnFreeFunc           free_data_func);
void        sn_launcher_context_ref               (SnLauncherContext *context);
void        sn_launcher_context_unref             (SnLauncherContext *context);


void        sn_launcher_context_initiate          (SnLauncherContext *context,
                                                   const char        *launcher_name,
                                                   const char        *launchee_name,
                                                   Time               timestamp);
Window      sn_launcher_context_get_launch_window (SnLauncherContext *context);
const char* sn_launcher_context_get_launch_id     (SnLauncherContext *context);
sn_bool_t   sn_launcher_context_get_initiated     (SnLauncherContext *context);
sn_bool_t   sn_launcher_context_get_canceled      (SnLauncherContext *context);
sn_bool_t   sn_launcher_context_get_completed     (SnLauncherContext *context);
void        sn_launcher_context_cancel            (SnLauncherContext *context);
void        sn_launcher_context_complete          (SnLauncherContext *context);

void        sn_launcher_context_setup_child_process (SnLauncherContext *context);

void sn_launcher_context_set_launch_type           (SnLauncherContext *context,
                                                    SnLaunchType       type);
void sn_launcher_context_set_geometry_window       (SnLauncherContext *context,
                                                    Window             xwindow);
void sn_launcher_context_set_supports_cancel       (SnLauncherContext *context,
                                                    sn_bool_t          supports_cancel);
void sn_launcher_context_set_launch_name           (SnLauncherContext *context,
                                                    const char        *name);
void sn_launcher_context_set_launch_description    (SnLauncherContext *context,
                                                    const char        *description);
void sn_launcher_context_set_launch_workspace      (SnLauncherContext *context,
                                                    int                workspace);
void sn_launcher_context_set_legacy_resource_class (SnLauncherContext *context,
                                                    const char        *klass);
void sn_launcher_context_set_legacy_resource_name  (SnLauncherContext *context,
                                                    const char        *name);
void sn_launcher_context_set_legacy_window_title   (SnLauncherContext *context,
                                                    const char        *title);
void sn_launcher_context_set_binary_name           (SnLauncherContext *context,
                                                    const char        *name);
void sn_launcher_context_set_pid                   (SnLauncherContext *context,
                                                    int                pid);
void sn_launcher_context_set_icon_name             (SnLauncherContext *context,
                                                    const char        *name);

SnLauncherEvent*    sn_launcher_event_copy        (SnLauncherEvent *event);
void                sn_launcher_event_ref         (SnLauncherEvent *event);
void                sn_launcher_event_unref       (SnLauncherEvent *event);
SnLauncherEventType sn_launcher_event_get_type    (SnLauncherEvent *event);
SnLauncherContext*  sn_launcher_event_get_context (SnLauncherEvent *event);
Time                sn_launcher_event_get_time    (SnLauncherEvent *event);

SN_END_DECLS

#endif /* __SN_LAUNCHER_H__ */