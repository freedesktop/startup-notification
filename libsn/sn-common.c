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
#include "lf-common.h"
#include "lf-internals.h"

struct LfDisplay
{
  int refcount;
  Display *xdisplay;
  int n_screens;
  Screen **screens;
  LfDisplayErrorTrapPush push_trap_func;
  LfDisplayErrorTrapPop  pop_trap_func;
};

/**
 * lf_display_new:
 * @xdisplay: an X window system display
 * @push_trap_func: function to push an X error trap
 * @pop_trap_func: function to pop an X error trap
 * 
 * Creates a new #LfDisplay object, containing
 * data that liblf associates with an X display.
 *
 * @push_trap_func should be a function that causes X errors to be
 * ignored until @pop_trap_func is called as many times as
 * @push_trap_func has been called. (Nested push/pop pairs must be
 * supported.) The outermost @pop_trap_func in a set of nested pairs
 * must call XSync() to ensure that all errors that will occur have in
 * fact occurred. These functions are used to avoid X errors due to
 * BadWindow and such.
 * 
 * Return value: the new #LfDisplay
 **/
LfDisplay*
lf_display_new (Display                *xdisplay,
                LfDisplayErrorTrapPush  push_trap_func,
                LfDisplayErrorTrapPop   pop_trap_func)
{
  LfDisplay *display;
  int i;
  
  display = lf_new0 (LfDisplay, 1);

  display->xdisplay = xdisplay;
  display->n_screens = ScreenCount (xdisplay);
  display->screens = lf_new (Screen*, display->n_screens);
  display->refcount = 1;

  display->push_trap_func = push_trap_func;
  display->pop_trap_func = pop_trap_func;
  
  for (i = 0; i < display->n_screens; ++i)
    display->screens[i] = ScreenOfDisplay (display->xdisplay, i);

  return display;
}

/**
 * lf_display_ref:
 * @display: an #LfDisplay
 * 
 * Increment the reference count for @display
 **/
void
lf_display_ref (LfDisplay *display)
{
  display->refcount += 1;
}

/**
 * lf_display_unref:
 * @display: an #LfDisplay
 * 
 * Decrement the reference count for @display, freeing
 * display if the reference count reaches zero.
 **/
void
lf_display_unref (LfDisplay *display)
{
  display->refcount -= 1;
  if (display->refcount == 0)
    {
      lf_free (display->screens);
      lf_free (display);
    }
}

/**
 * lf_display_get_x_display:
 * @display: an #LfDisplay
 * 
 * 
 * 
 * Return value: X display for this #LfDisplay
 **/
Display*
lf_display_get_x_display (LfDisplay *display)
{

  return display->xdisplay;
}

/**
 * lf_display_get_x_screen:
 * @display: an #LfDisplay
 * @number: screen number to get
 * 
 * Gets a screen by number; if the screen number
 * does not exist, returns %NULL.
 * 
 * Return value: X screen or %NULL
 **/
Screen*
lf_display_get_x_screen (LfDisplay *display,
                         int        number)
{
  if (number < 0 || number >= display->n_screens)
    return NULL;
  else
    return display->screens[number];
}

/**
 * lf_display_process_event:
 * @display: a display
 * @xevent: X event
 * 
 * liblf should be given a chance to see all X events by passing them
 * to this function. If the event was a property notify or client
 * message related to the launch feedback protocol, the
 * lf_display_process_event() returns true. Calling
 * lf_display_process_event() is not currently required for launchees,
 * only launchers and launch feedback displayers. The function returns
 * false for mapping, unmapping, window destruction, and selection
 * events even if they were involved in launch feedback.
 * 
 * Return value: true if the event was a property notify or client message involved in launch feedback
 **/
lf_bool_t
lf_display_process_event (LfDisplay *display,
                          XEvent    *xevent)
{
  lf_bool_t retval;

  retval = FALSE;

  if (lf_internal_launcher_process_event (display, xevent))
    retval = TRUE;

  if (lf_internal_monitor_process_event (display, xevent))
    retval = TRUE;

  if (lf_internal_xmessage_process_event (display, xevent))
    retval = TRUE;
  
  return retval;
}

/**
 * lf_display_error_trap_push:
 * @display: a display
 *
 *  Calls the push_trap_func from lf_display_new() if non-NULL.
 **/
void
lf_display_error_trap_push (LfDisplay *display)
{
  if (display->push_trap_func)
    (* display->push_trap_func) (display, display->xdisplay);
}

/**
 * lf_display_error_trap_pop:
 * @display: a display
 *
 *  Calls the pop_trap_func from lf_display_new() if non-NULL.
 **/
void
lf_display_error_trap_pop  (LfDisplay *display)
{
  if (display->pop_trap_func)
    (* display->pop_trap_func) (display, display->xdisplay);
}

