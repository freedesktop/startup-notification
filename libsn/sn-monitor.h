/* Monitor API - if you are a program that monitors launch sequences */
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


#ifndef __LF_MONITOR_H__
#define __LF_MONITOR_H__

#include <liblf/lf-common.h>

LF_BEGIN_DECLS

typedef struct LfMonitorContext LfMonitorContext;
typedef struct LfMonitorEvent   LfMonitorEvent;
typedef struct LfLaunchSequence LfLaunchSequence;

typedef void (* LfMonitorEventFunc) (LfMonitorEvent *event,
                                     void           *user_data);

typedef enum
{
  LF_MONITOR_EVENT_INITIATED,
  LF_MONITOR_EVENT_COMPLETED,
  LF_MONITOR_EVENT_CANCELED,
  LF_MONITOR_EVENT_PULSE,
  LF_MONITOR_EVENT_GEOMETRY_CHANGED,
  LF_MONITOR_EVENT_PID_CHANGED,
  /* only allowed with xmessages protocol */
  LF_MONITOR_EVENT_WORKSPACE_CHANGED
} LfMonitorEventType;

LfMonitorContext*  lf_monitor_context_new                  (LfDisplay           *display,
                                                            LfMonitorEventFunc   event_func,
                                                            void                *event_func_data,
                                                            LfFreeFunc           free_data_func);
void               lf_monitor_context_ref                  (LfMonitorContext *context);
void               lf_monitor_context_unref                (LfMonitorContext *context);


void               lf_monitor_event_ref                 (LfMonitorEvent *event);
void               lf_monitor_event_unref               (LfMonitorEvent *event);
LfMonitorEvent*    lf_monitor_event_copy                (LfMonitorEvent *event);
LfMonitorEventType lf_monitor_event_get_type            (LfMonitorEvent *event);
LfLaunchSequence*  lf_monitor_event_get_launch_sequence (LfMonitorEvent *event);
LfMonitorContext*  lf_monitor_event_get_context         (LfMonitorEvent *event);
Time               lf_monitor_event_get_time            (LfMonitorEvent *event);

void        lf_launch_sequence_ref                       (LfLaunchSequence *sequence);
void        lf_launch_sequence_unref                     (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_id                    (LfLaunchSequence *sequence);
Window      lf_launch_sequence_get_window                (LfLaunchSequence *sequence);
lf_bool_t   lf_launch_sequence_get_geometry              (LfLaunchSequence *sequence,
                                                          int              *x,
                                                          int              *y,
                                                          int              *width,
                                                          int              *height);
Window      lf_launch_sequence_get_geometry_window       (LfLaunchSequence *sequence);
lf_bool_t   lf_launch_sequence_get_completed             (LfLaunchSequence *sequence);
lf_bool_t   lf_launch_sequence_get_canceled              (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_name                  (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_description           (LfLaunchSequence *sequence);
int         lf_launch_sequence_get_workspace             (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_legacy_resource_class (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_legacy_resource_name  (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_legacy_window_title   (LfLaunchSequence *sequence);
lf_bool_t   lf_launch_sequence_get_supports_cancel       (LfLaunchSequence *sequence);
int         lf_launch_sequence_get_pid                   (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_binary_name           (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_hostname              (LfLaunchSequence *sequence);
const char* lf_launch_sequence_get_icon_name             (LfLaunchSequence *sequence);

void        lf_launch_sequence_cancel                    (LfLaunchSequence *sequence);

LF_END_DECLS

#endif /* __LF_MONITOR_H__ */
