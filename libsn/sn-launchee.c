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
#include "lf-launchee.h"
#include "lf-internals.h"
#include <errno.h>

struct LfLauncheeContext
{
  int refcount;
  LfDisplay *display;
  char *launch_id;
  Window launch_window;
};

/**
 * lf_launchee_context_new:
 * @display: an #LfDisplay
 * @launch_id: launch ID as in DESKTOP_LAUNCH_ID
 * @launch_window: launch window as in DESKTOP_LAUNCH_WINDOW
 * 
 * Creates a new launchee-side context for the launch feedback
 * protocol.
 * 
 * Return value: a new launchee context
 **/
LfLauncheeContext*
lf_launchee_context_new (LfDisplay         *display,
                         const char        *launch_id,
                         Window             launch_window)
{
  LfLauncheeContext *context;

  context = lf_new0 (LfLauncheeContext, 1);
  
  context->refcount = 1;
  
  context->display = display;
  lf_display_ref (context->display);

  context->launch_id = lf_malloc (strlen (launch_id) + 1);
  strcpy (context->launch_id, launch_id);

  context->launch_window = launch_window;

  return context;
}

/**
 * lf_launchee_context_new_from_environment:
 * @display: an #LfDisplay
 * 
 * Tries to create an #LfLauncheeContext given information in
 * the program's environment (DESKTOP_LAUNCH_ID and DESKTOP_LAUNCH_WINDOW
 * environment variables). Returns %NULL if the env variables are not
 * available or can't be parsed.
 * 
 * Return value: a new #LfLauncheeContext or %NULL
 **/
LfLauncheeContext*
lf_launchee_context_new_from_environment (LfDisplay *display)
{
  const char *id_str;
  const char *window_str;
  unsigned long window;
  
  id_str = getenv ("DESKTOP_LAUNCH_ID");
  window_str = getenv ("DESKTOP_LAUNCH_WINDOW");
  
  if (id_str == NULL || window_str == NULL)
    return NULL;
  
  window = lf_internal_string_to_ulong (window_str);
  
  if (window == None)
    return NULL;
  
  return lf_launchee_context_new (display, id_str, window);
}

void
lf_launchee_context_ref (LfLauncheeContext *context)
{
  context->refcount += 1;
}

void
lf_launchee_context_unref (LfLauncheeContext *context)
{
  context->refcount -= 1;
  if (context->refcount == 0)
    {
      lf_free (context->launch_id);
      
      lf_free (context);
    }
}

Window
lf_launchee_context_get_launch_window (LfLauncheeContext *context)
{
  return context->launch_window;
}

const char*
lf_launchee_context_get_launch_id (LfLauncheeContext *context)
{
  return context->launch_id;
}

/**
 * lf_launchee_context_pulse:
 * @context: an #LfLauncheeContext
 *
 * Notifies the launcher that progress is being made.  Should be
 * called regularly during a long launch operation.
 * 
 **/
void
lf_launchee_context_pulse (LfLauncheeContext *context)
{
  XEvent xev;
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.display = lf_display_get_x_display (context->display);
  xev.xclient.window = context->launch_window;
  xev.xclient.message_type = lf_internal_atom_get (context->display,
                                                   "_NET_LAUNCH_PULSE");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 0;
  xev.xclient.data.l[1] = 0;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;

  lf_display_error_trap_push (context->display);
  XSendEvent (lf_display_get_x_display (context->display),
              context->launch_window,
              False,
              PropertyChangeMask,
              &xev);
  XFlush (lf_display_get_x_display (context->display));
  lf_display_error_trap_pop (context->display);
}

/**
 * lf_launchee_context_cancel:
 * @context: an #LfLauncheeContext
 *
 * Called by the launchee application to cancel a launch (will
 * probably cause the launcher to kill the launchee).
 * 
 **/
void
lf_launchee_context_cancel (LfLauncheeContext *context)
{
  lf_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_CANCELED",
                            0);
}

/**
 * lf_launchee_context_complete:
 * @context: an #LfLauncheeContext
 *
 * Called by the launchee application when it is fully started up
 * and launch feedback should end.
 * 
 **/
void
lf_launchee_context_complete (LfLauncheeContext *context)
{
  lf_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_COMPLETE",
                            0);
}

/**
 * lf_launchee_context_setup_window:
 * @context: a #LfLauncheeContext
 * @xwindow: window to be set up
 * 
 * Sets up @xwindow, marking it as launched by the launch sequence
 * represented by @context. For example sets the _NET_LAUNCH_ID
 * property. Only the group leader windows of an application
 * MUST be set up with this function, though if
 * any other windows come from a separate launch sequence,
 * they can be setup up separately.
 * 
 **/
void
lf_launchee_context_setup_window (LfLauncheeContext *context,
                                  Window             xwindow)
{
  lf_internal_set_string (context->display,
                          xwindow,
                          "_NET_LAUNCH_ID",
                          context->launch_id);
}
