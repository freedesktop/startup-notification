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
#include "lf-launcher.h"
#include "lf-internals.h"

#include <sys/types.h>
#include <unistd.h>

static LfList* context_list = NULL;

struct LfLauncherContext
{
  int                 refcount;
  LfDisplay          *display;
  LfLauncherEventFunc event_func;
  void               *event_func_data;
  LfFreeFunc          free_data_func;
  char               *launch_id;
  Window              launch_window;
  LfLaunchType        type;
  Window              geometry_window;
  char               *name;
  char               *description;
  int                 workspace;
  char               *resource_class;
  char               *resource_name;
  char               *window_title;
  char               *binary_name;
  int                 pid;
  char               *icon_name;
  int                 x, y, width, height;
  unsigned int        supports_cancel : 1;
  unsigned int        completed : 1;
  unsigned int        canceled : 1;
  unsigned int        geometry_set : 1;
};

/**
 * lf_launcher_context_new:
 * @display: an #LfDisplay
 * @event_func: function to be called when a notable event occurs
 * @event_func_data: data to pass to @event_func
 * @free_data_func: function to be called on @event_func_data when freeing the context
 *
 * Creates a new launcher context, to be used by the program that is
 * starting a launch sequence. For example a file manager might
 * create a launcher context when the user double-clicks on
 * an application icon.
 *
 * Return value: a new #LfLauncherContext
 **/
LfLauncherContext*
lf_launcher_context_new (LfDisplay           *display,
                         LfLauncherEventFunc  event_func,
                         void                *event_func_data,
                         LfFreeFunc           free_data_func)
{
  LfLauncherContext *context;

  if (context_list == NULL)
    context_list = lf_list_new ();

  context = lf_new0 (LfLauncherContext, 1);

  context->refcount = 1;
  context->display = display;
  lf_display_ref (context->display);
  context->event_func = event_func;
  context->event_func_data = event_func_data;
  context->free_data_func = free_data_func;

  context->workspace = -1;
  context->pid = -1;
  context->type = LF_LAUNCH_TYPE_OTHER;
  
  lf_list_prepend (context_list, context);

  return context;
}

/**
 * lf_launcher_context_ref:
 * @context: a #LfLauncherContext
 *
 * Increments the reference count of @context
 **/
void
lf_launcher_context_ref (LfLauncherContext *context)
{
  context->refcount += 1;
}

/**
 * lf_launcher_context_unref:
 * @context: a #LfLauncherContext
 *
 * Decrements the reference count of @context and frees the
 * context if the count reaches zero.
 **/
void
lf_launcher_context_unref (LfLauncherContext *context)
{
  context->refcount -= 1;

  if (context->refcount == 0)
    {
      lf_list_remove (context_list, context);

      if (context->free_data_func)
        (* context->free_data_func) (context->event_func_data);

      lf_free (context->launch_id);
      
      if (context->launch_window != None)
        {
          lf_display_error_trap_push (context->display);

          XDestroyWindow (lf_display_get_x_display (context->display),
                          context->launch_window);

          lf_display_error_trap_pop (context->display);
        }

      lf_display_unref (context->display);
      lf_free (context);
    }
}

static char*
strip_slashes (const char *src)
{
  char *canonicalized_name;
  char *s;
  
  canonicalized_name = lf_internal_strdup (src);

  s = canonicalized_name;
  while (*s)
    {
      if (*s == '/')
        *s = '|';
      ++s;
    }

  return canonicalized_name;
}

/**
 * lf_launcher_context_initiate:
 * @context: an #LfLaunchContext
 * @launcher_name: name of the launcher app, suitable for debug output
 * @launchee_name: name of the launchee app, suitable for debug output
 * @timestamp: X timestamp of event causing the launch
 *
 * Initiates a launch sequence. All the properties of the launch (such
 * as type, geometry, description) should be set up prior to
 * initiating the sequence.
 **/
void
lf_launcher_context_initiate (LfLauncherContext *context,
                              const char        *launcher_name,
                              const char        *launchee_name,
                              Time               timestamp)
{
  static int sequence_number = 0;
  static lf_bool_t have_hostname = FALSE;
  static char hostbuf[257];
  char *s;
  int len;
  Display *xdisplay;
  char *canonicalized_launcher;
  char *canonicalized_launchee;
  Atom atoms[1];
  
  if (context->launch_id != NULL)
    {
      fprintf (stderr, "%s called twice for the same LfLaunchContext\n",
               __FUNCTION__);
      return;
    }

  if (!have_hostname)
    {
      if (gethostname (hostbuf, sizeof (hostbuf)-1) == 0)
        have_hostname = TRUE;
      else
        hostbuf[0] = '\0';
    }

  canonicalized_launcher = strip_slashes (launcher_name);
  canonicalized_launchee = strip_slashes (launchee_name);
  
  /* man I wish we could use g_strdup_printf */
  len = strlen (launcher_name) + strlen (launchee_name) +
    256; /* 256 is longer than a couple %d and some slashes */
  
  s = lf_malloc (len + 3);
  snprintf (s, len, "%s/%s/%lu/%d-%d-%s",
            canonicalized_launcher, canonicalized_launchee, (unsigned long) timestamp,
            (int) getpid (), (int) sequence_number, hostbuf);
  ++sequence_number;

  lf_free (canonicalized_launcher);
  lf_free (canonicalized_launchee);
  
  context->launch_id = s;

  xdisplay = lf_display_get_x_display (context->display);

  {
    XSetWindowAttributes attrs;

    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

    context->launch_window =
      XCreateWindow (xdisplay,
                     RootWindow (xdisplay, 0),
                     -100, -100, 1, 1,
                     0,
                     CopyFromParent,
                     CopyFromParent,
                     CopyFromParent,
                     CWOverrideRedirect | CWEventMask,
                     &attrs);
  }

  /* push outer error to allow avoiding XSync after every
   * property set
   */
  lf_display_error_trap_push (context->display);
  
  lf_internal_set_string (context->display,
                          context->launch_window,
                          "_NET_LAUNCH_ID",
                          context->launch_id);

  lf_internal_set_string (context->display,
                          context->launch_window,
                          "_NET_LAUNCH_HOSTNAME",
                          hostbuf);

  switch (context->type)
    {
    case LF_LAUNCH_TYPE_OTHER:
      atoms[0] = lf_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_OTHER");
      break;
    case LF_LAUNCH_TYPE_DOCK_ICON:
      atoms[0] = lf_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_DOCK_ICON");
      break;
    case LF_LAUNCH_TYPE_DESKTOP_ICON:
      atoms[0] = lf_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_DESKTOP_ICON");
      break;
    case LF_LAUNCH_TYPE_MENU:
      atoms[0] = lf_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_MENU");
      break;
    case LF_LAUNCH_TYPE_KEY_SHORTCUT:
      atoms[0] = lf_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_KEY_SHORTCUT");
      break;
    }

  lf_internal_set_atom_list (context->display,
                             context->launch_window,
                             "_NET_LAUNCH_TYPE",
                             atoms, 1);

  if (context->geometry_set)
    {
      int cardinals[4];
      
      cardinals[0] = context->x;
      cardinals[1] = context->y;
      cardinals[2] = context->width;
      cardinals[3] = context->height;
      
      lf_internal_set_cardinal_list (context->display,
                                     context->launch_window,
                                     "_NET_LAUNCH_GEOMETRY",
                                     cardinals, 4);
    }

  if (context->geometry_window != None)
    {
      lf_internal_set_window (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_GEOMETRY_WINDOW",
                              context->geometry_window);
    }

  if (context->supports_cancel)
    {
      lf_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_SUPPORTS_CANCEL",
                                context->supports_cancel);
    }

  if (context->name)
    {
      lf_internal_set_utf8_string (context->display,
                                   context->launch_window,
                                   "_NET_LAUNCH_NAME",
                                   context->name);
    }

  if (context->description)
    {
      lf_internal_set_utf8_string (context->display,
                                   context->launch_window,
                                   "_NET_LAUNCH_DESCRIPTION",
                                   context->description);
    }

  if (context->workspace >= 0)
    {
      lf_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_DESKTOP",
                                context->workspace);
    }

  if (context->pid >= 0)
    {
      lf_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_PID",
                                context->pid);
    }
  
  if (context->binary_name)
    {
      lf_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_BINARY_NAME",
                              context->binary_name);
    }

  if (context->icon_name)
    {
      lf_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_ICON_NAME",
                              context->icon_name);
    }

  if (context->resource_class)
    {
      lf_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_CLASS",
                              context->resource_class);
    }

  if (context->resource_name)
    {
      lf_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_NAME",
                              context->resource_name);
    }

  if (context->window_title)
    {
      lf_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_NAME",
                              context->window_title);
    }
  
  lf_display_error_trap_pop (context->display);
  
  /* Sync to server (so the launch window ID exists for example) */
  XFlush (xdisplay);
  
  /* Initiation message to all screens */
  {
    XEvent xev;
    
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.display = xdisplay;
    xev.xclient.window = context->launch_window;
    xev.xclient.message_type = lf_internal_atom_get (context->display,
                                                     "_NET_LAUNCH_INITIATE");
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = timestamp;
    xev.xclient.data.l[1] = 0;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    
    lf_internal_send_event_all_screens (context->display,
                                        PropertyChangeMask,
                                        &xev);
  }
}

Window
lf_launcher_context_get_launch_window (LfLauncherContext *context)
{
  return context->launch_window;
}

const char*
lf_launcher_context_get_launch_id (LfLauncherContext *context)
{

  return context->launch_id;
}

lf_bool_t
lf_launcher_context_get_initiated (LfLauncherContext *context)
{
  return context->launch_id != NULL;
}

lf_bool_t
lf_launcher_context_get_canceled (LfLauncherContext *context)
{
  return context->canceled;
}

/**
 * lf_launcher_context_get_completed:
 * @context: an #LfLauncherContext
 *
 * Returns %TRUE if _NET_LAUNCH_COMPLETE has been set or the
 * launch sequence window has been destroyed.
 *
 * Return value: %TRUE if the launch sequence has been completed
 **/
lf_bool_t
lf_launcher_context_get_completed (LfLauncherContext *context)
{
  return context->completed;
}

/**
 * lf_launcher_context_cancel:
 * @context: an #LfLauncherContext
 *
 * Marks the launch canceled by setting the _NET_LAUNCH_CANCELED
 * property on the launch window. May not be called if the launch has
 * not been initiated. An #LF_LAUNCHER_EVENT_CANCELED event should be
 * received in response to the cancellation, under normal
 * circumstances.
 *
 * lf_launcher_context_cancel() should be called to request a
 * cancellation.  Normally the launcher process is the process that
 * performs the cancellation as well, in response to an
 * #LF_LAUNCHER_EVENT_CANCELED event.
 *
 **/
void
lf_launcher_context_cancel (LfLauncherContext *context)
{
  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an LfLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  lf_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_CANCELED",
                            0);
}

/**
 * lf_launcher_context_complete:
 * @context: an #LfLauncherContext
 *
 * Marks @context as completed. Normally the launchee process marks a
 * launch sequence completed, however the launcher has to do it
 * if the launch is canceled.
 * 
 **/
void
lf_launcher_context_complete (LfLauncherContext *context)
{
  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an LfLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  lf_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_COMPLETE",
                            0);
}

/**
 * lf_launcher_context_setup_child_process:
 * @context: an #LfLauncherContext
 *
 * This function should be called after forking, but before exec(), in
 * the child process being launched. It sets up the environment variables
 * telling the child process about the launch ID and launch window.
 * This function will leak the strings passed to putenv() so should
 * only be used prior to an exec().
 *
 **/
void
lf_launcher_context_setup_child_process (LfLauncherContext *context)
{
  char *launch_id;
  char *s;
  char *launch_window;

  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an LfLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  /* Man we need glib here */

  launch_id = lf_malloc (strlen (context->launch_id) + strlen ("DESKTOP_LAUNCH_ID") + 3);
  strcpy (launch_id, "DESKTOP_LAUNCH_ID=");
  strcat (launch_id, context->launch_id);

  putenv (launch_id);

  launch_window = lf_malloc (strlen ("DESKTOP_LAUNCH_WINDOW") + 128);
  strcpy (launch_window, "DESKTOP_LAUNCH_WINDOW=");
  s = launch_window;
  while (*s)
    ++s;

  snprintf (s, 100, "0x%lx", context->launch_window);

  putenv (launch_window);

  /* Can't free strings passed to putenv */
}

#define WARN_ALREADY_INITIATED(context) do { if ((context)->launch_id != NULL) {               \
      fprintf (stderr, "%s called for an LfLauncherContext that has already been initiated\n", \
               __FUNCTION__);                                                                  \
      return;                                                                                  \
} } while (0)

void
lf_launcher_context_set_launch_type (LfLauncherContext *context,
                                     LfLaunchType       type)
{
  WARN_ALREADY_INITIATED (context);
  
  context->type = type;
}

void
lf_launcher_context_set_geometry_window (LfLauncherContext *context,
                                         Window             xwindow)
{
  WARN_ALREADY_INITIATED (context);

  context->geometry_window = xwindow;
}

void
lf_launcher_context_set_supports_cancel (LfLauncherContext *context,
                                         lf_bool_t          supports_cancel)
{
  WARN_ALREADY_INITIATED (context);

  context->supports_cancel = supports_cancel;
}

void
lf_launcher_context_set_launch_name (LfLauncherContext *context,
                                     const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->name);
  context->name = lf_internal_strdup (name);
}

void
lf_launcher_context_set_launch_description (LfLauncherContext *context,
                                            const char        *description)  
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->description);
  context->description = lf_internal_strdup (description);
}

void
lf_launcher_context_set_launch_workspace (LfLauncherContext *context,
                                          int                workspace)
{
  WARN_ALREADY_INITIATED (context);

  context->workspace = workspace;
}

void
lf_launcher_context_set_legacy_resource_class (LfLauncherContext *context,
                                               const char        *klass)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->resource_class);
  context->resource_class = lf_internal_strdup (klass);
}

void
lf_launcher_context_set_legacy_resource_name (LfLauncherContext *context,
                                              const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->resource_name);
  context->resource_name = lf_internal_strdup (name);
}

void
lf_launcher_context_set_legacy_window_title (LfLauncherContext *context,
                                             const char        *title)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->window_title);
  context->window_title = lf_internal_strdup (title);
}

void
lf_launcher_context_set_binary_name (LfLauncherContext *context,
                                     const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->binary_name);
  context->binary_name = lf_internal_strdup (name);
}

void
lf_launcher_context_set_pid (LfLauncherContext *context,
                             int                pid)
{
  context->pid = pid;

  /* set the X property if launch window already exists */
  if (context->launch_id != NULL)
    {
      lf_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_PID",
                                context->pid);
    }
}

void
lf_launcher_context_set_icon_name (LfLauncherContext *context,
                                   const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  lf_free (context->icon_name);
  context->icon_name = lf_internal_strdup (name);
}

struct LfLauncherEvent
{
  int refcount;
  LfLauncherEventType type;
  Time timestamp;
  LfLauncherContext *context;
};

/**
 * lf_launcher_event_copy:
 * @event: event to copy
 *
 * Creates a copy of @event, the copy has a reference count of one.
 *
 * Return value: a new #LfLauncherEvent that's a copy of @event
 **/
LfLauncherEvent*
lf_launcher_event_copy (LfLauncherEvent *event)
{
  LfLauncherEvent *copy;

  copy = lf_new (LfLauncherEvent, 1);

  copy->refcount = 1;
  copy->type = event->type;
  copy->timestamp = event->timestamp;
  copy->context = event->context;
  if (copy->context)
    lf_launcher_context_ref (copy->context);

  return copy;
}

/**
 * lf_launcher_event_ref:
 * @event: a #LfLauncherEvent
 *
 * Increments @event's reference count.
 **/
void
lf_launcher_event_ref (LfLauncherEvent *event)
{
  event->refcount += 1;
}

/**
 * lf_launcher_event_unref:
 * @event: a #LfLauncherEvent
 *
 * Decrements @event's reference count and frees @event
 * if the count reaches zero.
 **/
void
lf_launcher_event_unref (LfLauncherEvent *event)
{
  event->refcount -= 1;

  if (event->refcount == 0)
    {
      if (event->context)
        lf_launcher_context_unref (event->context);
      lf_free (event);
    }
}

/**
 * lf_launcher_event_get_type:
 * @event: a #LfLauncherEvent
 *
 * Gets the type of the launcher event.
 *
 * Return value: the type of event
 **/
LfLauncherEventType
lf_launcher_event_get_type (LfLauncherEvent *event)
{
  return event->type;
}

/**
 * lf_launcher_event_get_context:
 * @event: a #LfLauncherEvent
 *
 * Gets the context associated with @event. The
 * returned context is owned by the event, i.e.
 * the caller of lf_launcher_event_get_context() should
 * not unref the context.
 *
 * Return value: the context for this event
 **/
LfLauncherContext*
lf_launcher_event_get_context (LfLauncherEvent *event)
{
  return event->context;
}

/**
 * lf_launcher_event_get_time:
 * @event: a #LfLauncherEvent
 *
 * Gets the X Window System timestamp associated with this launcher
 * event.
 *
 * Return value: timestamp for the event, or CurrentTime if none available
 **/
Time
lf_launcher_event_get_time (LfLauncherEvent *event)
{
  return event->timestamp;
}

static lf_bool_t
check_cardinal_exists (LfDisplay  *display,
                       Window      xwindow,
                       const char *property)
{
  int val;

  return lf_internal_get_cardinal (display, xwindow, property,
                                   &val);
}

typedef struct
{
  LfDisplay *display;
  Window     launch_window;
  lf_bool_t  result;
} HaveContextsData;

static lf_bool_t
have_active_contexts_foreach (void *value,
                              void *data)
{
  LfLauncherContext *context = value;
  HaveContextsData *hcd = data;

  if (!context->completed &&
      context->launch_window == hcd->launch_window &&
      lf_display_get_x_display (context->display) ==
      lf_display_get_x_display (hcd->display))
    {
      hcd->result = TRUE;
      return FALSE;
    }

  return TRUE;
}

typedef struct
{
  LfDisplay *display;
  Window     launch_window;
  LfList    *contexts;
} FindContextsData;

static lf_bool_t
find_active_contexts_foreach (void *value,
                              void *data)
{
  LfLauncherContext *context = value;
  FindContextsData *fcd = data;

  if (!context->completed &&
      context->launch_window == fcd->launch_window &&
      lf_display_get_x_display (context->display) ==
      lf_display_get_x_display (fcd->display))
    lf_list_prepend (fcd->contexts, context);

  return TRUE;
}

typedef struct
{
  LfLauncherEvent *base_event;
  LfList *events;
} CreateEventsData;

static lf_bool_t
create_events_foreach (void *value,
                       void *data)
{
  LfLauncherContext *context = value;
  CreateEventsData *ced = data;
  LfLauncherEvent *event;

  event = lf_launcher_event_copy (ced->base_event);
  event->context = context;
  lf_launcher_context_ref (context);

  lf_list_prepend (ced->events, event);

  return TRUE;
}

static lf_bool_t
dispatch_events_foreach (void *value,
                         void *data)
{
  LfLauncherEvent *event = value;

  /* Filter out duplicate events and update flags */
  switch (event->type)
    {
    case LF_LAUNCHER_EVENT_CANCELED:
      if (event->context->canceled)
        {
          lf_launcher_event_unref (event);
          event = NULL;
        }
      else
        {
          event->context->canceled = TRUE;
        }
      break;
    case LF_LAUNCHER_EVENT_COMPLETED:
      if (event->context->completed)
        {
          lf_launcher_event_unref (event);
          event = NULL;
        }
      else
        {
          event->context->completed = TRUE;
        }
      break;

    default:
      break;
    }

  if (event)
    {
      if (event->context->event_func)
        (* event->context->event_func) (event,
                                        event->context->event_func_data);
      lf_launcher_event_unref (event);
    }

  return TRUE;
}

static void
dispatch_event (LfDisplay       *display,
                Window           launch_window,
                LfLauncherEvent *event)
{
  /* Find all applicable contexts, create an event for each, and send
   * the events out.
   */
  FindContextsData fcd;
  CreateEventsData ced;

  fcd.display = display;
  fcd.launch_window = launch_window;
  fcd.contexts = lf_list_new ();

  if (context_list != NULL)
    lf_list_foreach (context_list, find_active_contexts_foreach, &fcd);

  ced.base_event = event;
  ced.events = lf_list_new ();
  lf_list_foreach (fcd.contexts, create_events_foreach, &ced);

  /* This unref's each event as it's dispatched */
  lf_list_foreach (ced.events, dispatch_events_foreach, NULL);

  lf_list_free (fcd.contexts);
  lf_list_free (ced.events);
}
     
lf_bool_t
lf_internal_launcher_process_event (LfDisplay *display,
                                    XEvent    *xevent)
{
  lf_bool_t retval;
  LfLauncherEvent *event;
  Window event_xwindow;

  if (context_list == NULL ||
      lf_list_empty (context_list))
    return FALSE; /* no one cares */
  
  event_xwindow = None;
  event = NULL;
  retval = FALSE;

  switch (xevent->xany.type)
    {
    case PropertyNotify:
      if (xevent->xproperty.atom ==
          lf_internal_atom_get (display, "_NET_LAUNCH_CANCELED"))
        {
          event_xwindow = xevent->xproperty.window;

          if (check_cardinal_exists (display, event_xwindow,
                                     "_NET_LAUNCH_CANCELED"))
            {
              event = lf_new (LfLauncherEvent, 1);

              event->refcount = 1;
              event->type = LF_LAUNCHER_EVENT_CANCELED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
            }

          retval = TRUE;
        }
      else if (xevent->xproperty.atom ==
               lf_internal_atom_get (display, "_NET_LAUNCH_COMPLETE"))
        {
          event_xwindow = xevent->xproperty.window;

          if (check_cardinal_exists (display, event_xwindow,
                                     "_NET_LAUNCH_COMPLETE"))
            {
              event = lf_new (LfLauncherEvent, 1);

              event->refcount = 1;
              event->type = LF_LAUNCHER_EVENT_COMPLETED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
            }

          retval = TRUE;
        }
      break;

    case ClientMessage:
      if (xevent->xclient.message_type ==
          lf_internal_atom_get (display,
                                "_NET_LAUNCH_PULSE"))
        {
          event_xwindow = xevent->xclient.window;

          event = lf_new (LfLauncherEvent, 1);

          event->refcount = 1;
          event->type = LF_LAUNCHER_EVENT_PULSE;
          event->timestamp = CurrentTime;
          event->context = NULL;

          retval = TRUE;
        }
      break;

    case DestroyNotify:
      {
        HaveContextsData hcd;
        hcd.display = display;
        hcd.launch_window = xevent->xdestroywindow.window;
        hcd.result = FALSE;
        if (context_list)
          lf_list_foreach (context_list, have_active_contexts_foreach, &hcd);
        if (hcd.result)
          {
            event_xwindow = hcd.launch_window;

            event = lf_new (LfLauncherEvent, 1);

            event->refcount = 1;
            event->type = LF_LAUNCHER_EVENT_COMPLETED;
            event->timestamp = CurrentTime;
            event->context = NULL;
          }
      }
      break;

    default:
      break;
    }

  if (event != NULL)
    {
      dispatch_event (display, event_xwindow, event);
      
      lf_launcher_event_unref (event);
    }

  return retval;
}

