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


#ifndef __SN_MONITOR_H__
#define __SN_MONITOR_H__

#include <libsn/sn-common.h>

SN_BEGIN_DECLS

typedef struct SnMonitorContext SnMonitorContext;
typedef struct SnMonitorEvent   SnMonitorEvent;
typedef struct SnLaunchSequence SnLaunchSequence;

typedef void (* SnMonitorEventFunc) (SnMonitorEvent *event,
                                     void           *user_data);

typedef enum
{
  SN_MONITOR_EVENT_INITIATED,
  SN_MONITOR_EVENT_COMPLETED,
  SN_MONITOR_EVENT_CANCELED,
  SN_MONITOR_EVENT_PULSE,
  SN_MONITOR_EVENT_GEOMETRY_CHANGED,
  SN_MONITOR_EVENT_PID_CHANGED,
  /* only allowed with xmessages protocol */
  SN_MONITOR_EVENT_WORKSPACE_CHANGED
} SnMonitorEventType;

SnMonitorContext*  sn_monitor_context_new                  (SnDisplay           *display,
                                                            SnMonitorEventFunc   event_func,
                                                            void                *event_func_data,
                                                            SnFreeFunc           free_data_func);
void               sn_monitor_context_ref                  (SnMonitorContext *context);
void               sn_monitor_context_unref                (SnMonitorContext *context);


void               sn_monitor_event_ref                 (SnMonitorEvent *event);
void               sn_monitor_event_unref               (SnMonitorEvent *event);
SnMonitorEvent*    sn_monitor_event_copy                (SnMonitorEvent *event);
SnMonitorEventType sn_monitor_event_get_type            (SnMonitorEvent *event);
SnLaunchSequence*  sn_monitor_event_get_launch_sequence (SnMonitorEvent *event);
SnMonitorContext*  sn_monitor_event_get_context         (SnMonitorEvent *event);
Time               sn_monitor_event_get_time            (SnMonitorEvent *event);

void        sn_launch_sequence_ref                       (SnLaunchSequence *sequence);
void        sn_launch_sequence_unref                     (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_id                    (SnLaunchSequence *sequence);
Window      sn_launch_sequence_get_window                (SnLaunchSequence *sequence);
sn_bool_t   sn_launch_sequence_get_geometry              (SnLaunchSequence *sequence,
                                                          int              *x,
                                                          int              *y,
                                                          int              *width,
                                                          int              *height);
Window      sn_launch_sequence_get_geometry_window       (SnLaunchSequence *sequence);
sn_bool_t   sn_launch_sequence_get_completed             (SnLaunchSequence *sequence);
sn_bool_t   sn_launch_sequence_get_canceled              (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_name                  (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_description           (SnLaunchSequence *sequence);
int         sn_launch_sequence_get_workspace             (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_legacy_resource_class (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_legacy_resource_name  (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_legacy_window_title   (SnLaunchSequence *sequence);
sn_bool_t   sn_launch_sequence_get_supports_cancel       (SnLaunchSequence *sequence);
int         sn_launch_sequence_get_pid                   (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_binary_name           (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_hostname              (SnLaunchSequence *sequence);
const char* sn_launch_sequence_get_icon_name             (SnLaunchSequence *sequence);

void        sn_launch_sequence_cancel                    (SnLaunchSequence *sequence);

SN_END_DECLS

#endif /* __SN_MONITOR_H__ */
