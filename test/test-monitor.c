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

#include <config.h>
#include <libsn/sn.h>

#include "test-boilerplate.h"

static void
monitor_event_func (SnMonitorEvent *event,
                    void            *user_data)
{
  SnMonitorContext *context;
  SnLaunchSequence *sequence;
  
  context = sn_monitor_event_get_context (event);
  sequence = sn_monitor_event_get_launch_sequence (event);
  
  switch (sn_monitor_event_get_type (event))
    {
    case SN_MONITOR_EVENT_INITIATED:
      {
        int x, y, w, h;
        const char *s;
        
        printf ("Initiated sequence %s\n",
                sn_launch_sequence_get_id (sequence));
        printf (" launch window 0x%lx\n",
                sn_launch_sequence_get_window (sequence));

        s = sn_launch_sequence_get_name (sequence);
        printf (" name %s\n", s ? s : "(unset)");

        s = sn_launch_sequence_get_description (sequence);
        printf (" description %s\n", s ? s : "(unset)");

        printf (" workspace %d\n",
                sn_launch_sequence_get_workspace (sequence));

        printf (" %s cancel\n",
                sn_launch_sequence_get_supports_cancel (sequence) ?
                "supports" : "does not support");
        
        if (sn_launch_sequence_get_geometry (sequence,
                                             &x, &y, &w, &h))
          printf (" geometry %d,%d %d x %d window 0x%lx\n",
                  x, y, w, h,
                  sn_launch_sequence_get_geometry_window (sequence));
        else
          printf (" no geometry set\n");

        printf (" pid %d\n",
                sn_launch_sequence_get_pid (sequence));
        
        s = sn_launch_sequence_get_binary_name (sequence);
        printf (" binary name %s\n", s ? s : "(unset)");
        s = sn_launch_sequence_get_icon_name (sequence);
        printf (" icon name %s\n", s ? s : "(unset)");
        s = sn_launch_sequence_get_hostname (sequence);
        printf (" hostname %s\n", s ? s : "(unset)");
        
        s = sn_launch_sequence_get_legacy_resource_class (sequence);
        printf (" legacy class %s\n", s ? s : "(unset)");
        s = sn_launch_sequence_get_legacy_resource_name (sequence);
        printf (" legacy name %s\n", s ? s : "(unset)");
        s = sn_launch_sequence_get_legacy_window_title (sequence);
        printf (" legacy title %s\n", s ? s : "(unset)");        
      }
      break;

    case SN_MONITOR_EVENT_COMPLETED:
      printf ("Completed sequence %s\n",
              sn_launch_sequence_get_id (sequence));
      break;

    case SN_MONITOR_EVENT_CANCELED:
      printf ("Canceled sequence %s\n",
              sn_launch_sequence_get_id (sequence));
      break;

    case SN_MONITOR_EVENT_PULSE:
      printf ("Pulse for sequence %s\n",
              sn_launch_sequence_get_id (sequence));
      break;
      
    case SN_MONITOR_EVENT_GEOMETRY_CHANGED:
      {
        int x, y, w, h;

        printf ("Geometry changed for sequence %s\n",
                sn_launch_sequence_get_id (sequence));
        if (sn_launch_sequence_get_geometry (sequence,
                                             &x, &y, &w, &h))
          printf (" geometry %d,%d %d x %d window 0x%lx\n",
                  x, y, w, h,
                  sn_launch_sequence_get_geometry_window (sequence));
        else
          printf (" no geometry set\n");
      }
      break;
    case SN_MONITOR_EVENT_PID_CHANGED:
      {
        printf ("PID for sequence %s is now %d\n",
                sn_launch_sequence_get_id (sequence),
                sn_launch_sequence_get_pid (sequence));
      }
      break;
    case SN_MONITOR_EVENT_WORKSPACE_CHANGED:
      {
        printf ("Workspace for sequence %s is now %d\n",
                sn_launch_sequence_get_id (sequence),
                sn_launch_sequence_get_workspace (sequence));
      }
      break;
    }
}

int
main (int argc, char **argv)
{
  Display *xdisplay;
  SnDisplay *display;
  SnMonitorContext *context;
  
  xdisplay = XOpenDisplay (NULL);
  if (xdisplay == NULL)
    {
      fprintf (stderr, "Could not open display\n");
      return 1;
    }

  if (getenv ("LIBSN_SYNC") != NULL)
    XSynchronize (xdisplay, True);
  
  XSetErrorHandler (x_error_handler);

  /* We have to select for property events on at least one
   * root window (but not all as INITIATE messages go to
   * all root windows)
   */
  XSelectInput (xdisplay, DefaultRootWindow (xdisplay),
                PropertyChangeMask);
  
  display = sn_display_new (xdisplay,
                            error_trap_push,
                            error_trap_pop);

  context = sn_monitor_context_new (display,
                                    monitor_event_func,
                                    NULL, NULL);  
  
  while (TRUE)
    {
      XEvent xevent;

      XNextEvent (xdisplay, &xevent);

      sn_display_process_event (display, &xevent);
    }

  sn_monitor_context_unref (context);
  
  return 0;
}