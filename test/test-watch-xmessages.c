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
#include <liblf/lf.h>
#include <liblf/lf-xmessages.h>
#include <liblf/lf-internals.h>

#include "test-boilerplate.h"

static void
message_func (LfDisplay       *display,
              const char      *message_type,
              const char      *message,
              void            *user_data)
{
  char *prefix;
  char **names;
  char **values;
  int i;

#if 0
  printf ("raw %s: %s\n",
          message_type, message);
#endif
  
  prefix = NULL;
  names = NULL;
  values = NULL;

  if (lf_internal_unserialize_message (message,
                                       &prefix, &names, &values))
    {
      printf (" %s:\n", prefix);

      i = 0;
      while (names && names[i])
        {
          printf ("   '%s' = '%s'\n", names[i], values[i]);
          
          ++i;
        }

      lf_internal_strfreev (names);
      lf_internal_strfreev (values);
    }
}

int
main (int argc, char **argv)
{
  Display *xdisplay;
  LfDisplay *display;

  if (argc != 2)
    {
      fprintf (stderr, "argument must be type of events to watch\n");
      return 1;
    }
  
  xdisplay = XOpenDisplay (NULL);
  if (xdisplay == NULL)
    {
      fprintf (stderr, "Could not open display\n");
      return 1;
    }

  if (getenv ("LIBLF_SYNC") != NULL)
    XSynchronize (xdisplay, True);
  
  XSetErrorHandler (x_error_handler);

  /* We have to select for property events on one root window
   */
  XSelectInput (xdisplay, DefaultRootWindow (xdisplay),
                PropertyChangeMask);
  
  display = lf_display_new (xdisplay,
                            error_trap_push,
                            error_trap_pop);

  lf_internal_add_xmessage_func (display,
                                 argv[1],
                                 message_func,
                                 NULL, NULL);
  
  while (TRUE)
    {
      XEvent xevent;

      XNextEvent (xdisplay, &xevent);

      lf_display_process_event (display, &xevent);
    }
  
  return 0;
}
