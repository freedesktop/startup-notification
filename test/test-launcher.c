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

#include "test-boilerplate.h"

static pid_t child_pid = 0;

static void
launcher_event_func (LfLauncherEvent *event,
                     void            *user_data)
{
  LfLauncherContext *context;

  context = lf_launcher_event_get_context (event);
  
  switch (lf_launcher_event_get_type (event))
    {
    case LF_LAUNCHER_EVENT_COMPLETED:
      printf ("Completed!\n");
      break;
    case LF_LAUNCHER_EVENT_CANCELED:
      printf ("Canceled!\n");
      kill (child_pid, SIGTERM);
      lf_launcher_context_complete (context);
      break;
    case LF_LAUNCHER_EVENT_PULSE:
      printf (" pulse.\n");
      break;
    }
}

int
main (int argc, char **argv)
{
  Display *xdisplay;
  LfDisplay *display;
  LfLauncherContext *context;

  if (argc < 2)
    {
      fprintf (stderr, "must specify command line to launch\n");
      exit (1);
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
  
  display = lf_display_new (xdisplay,
                            error_trap_push,
                            error_trap_pop);

  context = lf_launcher_context_new (display,
                                     launcher_event_func,
                                     NULL, NULL);  

  lf_launcher_context_set_launch_name (context, "Test Launch");
  lf_launcher_context_set_launch_description (context, "Launching a test program for liblf");
  lf_launcher_context_set_supports_cancel (context, TRUE);
  lf_launcher_context_set_binary_name (context, argv[1]);
  
  lf_launcher_context_initiate (context,
                                "test-launcher",
                                argv[1],
                                CurrentTime); /* CurrentTime bad */

  switch ((child_pid = fork ()))
    {
    case -1:
      fprintf (stderr, "Fork failed: %s\n", strerror (errno));
      break;
    case 0:
      lf_launcher_context_setup_child_process (context);
      execv (argv[1], argv + 1);
      fprintf (stderr, "Failed to exec %s: %s\n", argv[1], strerror (errno));
      _exit (1);
      break;
    }

  lf_launcher_context_set_pid (context, child_pid);
  
  while (TRUE)
    {
      XEvent xevent;

      XNextEvent (xdisplay, &xevent);

      lf_display_process_event (display, &xevent);
    }

  lf_launcher_context_unref (context);
  
  return 0;
}
