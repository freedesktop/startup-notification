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
#include "sn-launcher.h"
#include "sn-internals.h"

#include <sys/types.h>
#include <unistd.h>

static SnList* context_list = NULL;

struct SnLauncherContext
{
  int                 refcount;
  SnDisplay          *display;
  SnLauncherEventFunc event_func;
  void               *event_func_data;
  SnFreeFunc          free_data_func;
  char               *launch_id;
  Window              launch_window;
  SnLaunchType        type;
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
 * sn_launcher_context_new:
 * @display: an #SnDisplay
 * @event_func: function to be called when a notable event occurs
 * @event_func_data: data to pass to @event_func
 * @free_data_func: function to be called on @event_func_data when freeing the context
 *
 * Creates a new launcher context, to be used by the program that is
 * starting a launch sequence. For example a file manager might
 * create a launcher context when the user double-clicks on
 * an application icon.
 *
 * Return value: a new #SnLauncherContext
 **/
SnLauncherContext*
sn_launcher_context_new (SnDisplay           *display,
                         SnLauncherEventFunc  event_func,
                         void                *event_func_data,
                         SnFreeFunc           free_data_func)
{
  SnLauncherContext *context;

  if (context_list == NULL)
    context_list = sn_list_new ();

  context = sn_new0 (SnLauncherContext, 1);

  context->refcount = 1;
  context->display = display;
  sn_display_ref (context->display);
  context->event_func = event_func;
  context->event_func_data = event_func_data;
  context->free_data_func = free_data_func;

  context->workspace = -1;
  context->pid = -1;
  context->type = SN_LAUNCH_TYPE_OTHER;
  
  sn_list_prepend (context_list, context);

  return context;
}

/**
 * sn_launcher_context_ref:
 * @context: a #SnLauncherContext
 *
 * Increments the reference count of @context
 **/
void
sn_launcher_context_ref (SnLauncherContext *context)
{
  context->refcount += 1;
}

/**
 * sn_launcher_context_unref:
 * @context: a #SnLauncherContext
 *
 * Decrements the reference count of @context and frees the
 * context if the count reaches zero.
 **/
void
sn_launcher_context_unref (SnLauncherContext *context)
{
  context->refcount -= 1;

  if (context->refcount == 0)
    {
      sn_list_remove (context_list, context);

      if (context->free_data_func)
        (* context->free_data_func) (context->event_func_data);

      sn_free (context->launch_id);
      
      if (context->launch_window != None)
        {
          sn_display_error_trap_push (context->display);

          XDestroyWindow (sn_display_get_x_display (context->display),
                          context->launch_window);

          sn_display_error_trap_pop (context->display);
        }

      sn_display_unref (context->display);
      sn_free (context);
    }
}

static char*
strip_slashes (const char *src)
{
  char *canonicalized_name;
  char *s;
  
  canonicalized_name = sn_internal_strdup (src);

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
 * sn_launcher_context_initiate:
 * @context: an #SnLaunchContext
 * @launcher_name: name of the launcher app, suitable for debug output
 * @launchee_name: name of the launchee app, suitable for debug output
 * @timestamp: X timestamp of event causing the launch
 *
 * Initiates a launch sequence. All the properties of the launch (such
 * as type, geometry, description) should be set up prior to
 * initiating the sequence.
 **/
void
sn_launcher_context_initiate (SnLauncherContext *context,
                              const char        *launcher_name,
                              const char        *launchee_name,
                              Time               timestamp)
{
  static int sequence_number = 0;
  static sn_bool_t have_hostname = FALSE;
  static char hostbuf[257];
  char *s;
  int len;
  Display *xdisplay;
  char *canonicalized_launcher;
  char *canonicalized_launchee;
  Atom atoms[1];
  
  if (context->launch_id != NULL)
    {
      fprintf (stderr, "%s called twice for the same SnLaunchContext\n",
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
  
  s = sn_malloc (len + 3);
  snprintf (s, len, "%s/%s/%lu/%d-%d-%s",
            canonicalized_launcher, canonicalized_launchee, (unsigned long) timestamp,
            (int) getpid (), (int) sequence_number, hostbuf);
  ++sequence_number;

  sn_free (canonicalized_launcher);
  sn_free (canonicalized_launchee);
  
  context->launch_id = s;

  xdisplay = sn_display_get_x_display (context->display);

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
  sn_display_error_trap_push (context->display);
  
  sn_internal_set_string (context->display,
                          context->launch_window,
                          "_NET_LAUNCH_ID",
                          context->launch_id);

  sn_internal_set_string (context->display,
                          context->launch_window,
                          "_NET_LAUNCH_HOSTNAME",
                          hostbuf);

  switch (context->type)
    {
    case SN_LAUNCH_TYPE_OTHER:
      atoms[0] = sn_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_OTHER");
      break;
    case SN_LAUNCH_TYPE_DOCK_ICON:
      atoms[0] = sn_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_DOCK_ICON");
      break;
    case SN_LAUNCH_TYPE_DESKTOP_ICON:
      atoms[0] = sn_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_DESKTOP_ICON");
      break;
    case SN_LAUNCH_TYPE_MENU:
      atoms[0] = sn_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_MENU");
      break;
    case SN_LAUNCH_TYPE_KEY_SHORTCUT:
      atoms[0] = sn_internal_atom_get (context->display,
                                       "_NET_LAUNCH_TYPE_KEY_SHORTCUT");
      break;
    }

  sn_internal_set_atom_list (context->display,
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
      
      sn_internal_set_cardinal_list (context->display,
                                     context->launch_window,
                                     "_NET_LAUNCH_GEOMETRY",
                                     cardinals, 4);
    }

  if (context->geometry_window != None)
    {
      sn_internal_set_window (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_GEOMETRY_WINDOW",
                              context->geometry_window);
    }

  if (context->supports_cancel)
    {
      sn_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_SUPPORTS_CANCEL",
                                context->supports_cancel);
    }

  if (context->name)
    {
      sn_internal_set_utf8_string (context->display,
                                   context->launch_window,
                                   "_NET_LAUNCH_NAME",
                                   context->name);
    }

  if (context->description)
    {
      sn_internal_set_utf8_string (context->display,
                                   context->launch_window,
                                   "_NET_LAUNCH_DESCRIPTION",
                                   context->description);
    }

  if (context->workspace >= 0)
    {
      sn_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_DESKTOP",
                                context->workspace);
    }

  if (context->pid >= 0)
    {
      sn_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_PID",
                                context->pid);
    }
  
  if (context->binary_name)
    {
      sn_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_BINARY_NAME",
                              context->binary_name);
    }

  if (context->icon_name)
    {
      sn_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_ICON_NAME",
                              context->icon_name);
    }

  if (context->resource_class)
    {
      sn_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_CLASS",
                              context->resource_class);
    }

  if (context->resource_name)
    {
      sn_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_NAME",
                              context->resource_name);
    }

  if (context->window_title)
    {
      sn_internal_set_string (context->display,
                              context->launch_window,
                              "_NET_LAUNCH_LEGACY_NAME",
                              context->window_title);
    }
  
  sn_display_error_trap_pop (context->display);
  
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
    xev.xclient.message_type = sn_internal_atom_get (context->display,
                                                     "_NET_LAUNCH_INITIATE");
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = timestamp;
    xev.xclient.data.l[1] = 0;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    
    sn_internal_send_event_all_screens (context->display,
                                        PropertyChangeMask,
                                        &xev);
  }
}

Window
sn_launcher_context_get_launch_window (SnLauncherContext *context)
{
  return context->launch_window;
}

const char*
sn_launcher_context_get_launch_id (SnLauncherContext *context)
{

  return context->launch_id;
}

sn_bool_t
sn_launcher_context_get_initiated (SnLauncherContext *context)
{
  return context->launch_id != NULL;
}

sn_bool_t
sn_launcher_context_get_canceled (SnLauncherContext *context)
{
  return context->canceled;
}

/**
 * sn_launcher_context_get_completed:
 * @context: an #SnLauncherContext
 *
 * Returns %TRUE if _NET_LAUNCH_COMPLETE has been set or the
 * launch sequence window has been destroyed.
 *
 * Return value: %TRUE if the launch sequence has been completed
 **/
sn_bool_t
sn_launcher_context_get_completed (SnLauncherContext *context)
{
  return context->completed;
}

/**
 * sn_launcher_context_cancel:
 * @context: an #SnLauncherContext
 *
 * Marks the launch canceled by setting the _NET_LAUNCH_CANCELED
 * property on the launch window. May not be called if the launch has
 * not been initiated. An #SN_LAUNCHER_EVENT_CANCELED event should be
 * received in response to the cancellation, under normal
 * circumstances.
 *
 * sn_launcher_context_cancel() should be called to request a
 * cancellation.  Normally the launcher process is the process that
 * performs the cancellation as well, in response to an
 * #SN_LAUNCHER_EVENT_CANCELED event.
 *
 **/
void
sn_launcher_context_cancel (SnLauncherContext *context)
{
  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an SnLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  sn_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_CANCELED",
                            0);
}

/**
 * sn_launcher_context_complete:
 * @context: an #SnLauncherContext
 *
 * Marks @context as completed. Normally the launchee process marks a
 * launch sequence completed, however the launcher has to do it
 * if the launch is canceled.
 * 
 **/
void
sn_launcher_context_complete (SnLauncherContext *context)
{
  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an SnLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  sn_internal_set_cardinal (context->display,
                            context->launch_window,
                            "_NET_LAUNCH_COMPLETE",
                            0);
}

/**
 * sn_launcher_context_setup_child_process:
 * @context: an #SnLauncherContext
 *
 * This function should be called after forking, but before exec(), in
 * the child process being launched. It sets up the environment variables
 * telling the child process about the launch ID and launch window.
 * This function will leak the strings passed to putenv() so should
 * only be used prior to an exec().
 *
 **/
void
sn_launcher_context_setup_child_process (SnLauncherContext *context)
{
  char *launch_id;
  char *s;
  char *launch_window;

  if (context->launch_id == NULL)
    {
      fprintf (stderr, "%s called for an SnLauncherContext that hasn't been initiated\n",
               __FUNCTION__);
      return;
    }

  /* Man we need glib here */

  launch_id = sn_malloc (strlen (context->launch_id) + strlen ("DESKTOP_LAUNCH_ID") + 3);
  strcpy (launch_id, "DESKTOP_LAUNCH_ID=");
  strcat (launch_id, context->launch_id);

  putenv (launch_id);

  launch_window = sn_malloc (strlen ("DESKTOP_LAUNCH_WINDOW") + 128);
  strcpy (launch_window, "DESKTOP_LAUNCH_WINDOW=");
  s = launch_window;
  while (*s)
    ++s;

  snprintf (s, 100, "0x%lx", context->launch_window);

  putenv (launch_window);

  /* Can't free strings passed to putenv */
}

#define WARN_ALREADY_INITIATED(context) do { if ((context)->launch_id != NULL) {               \
      fprintf (stderr, "%s called for an SnLauncherContext that has already been initiated\n", \
               __FUNCTION__);                                                                  \
      return;                                                                                  \
} } while (0)

void
sn_launcher_context_set_launch_type (SnLauncherContext *context,
                                     SnLaunchType       type)
{
  WARN_ALREADY_INITIATED (context);
  
  context->type = type;
}

void
sn_launcher_context_set_geometry_window (SnLauncherContext *context,
                                         Window             xwindow)
{
  WARN_ALREADY_INITIATED (context);

  context->geometry_window = xwindow;
}

void
sn_launcher_context_set_supports_cancel (SnLauncherContext *context,
                                         sn_bool_t          supports_cancel)
{
  WARN_ALREADY_INITIATED (context);

  context->supports_cancel = supports_cancel;
}

void
sn_launcher_context_set_launch_name (SnLauncherContext *context,
                                     const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->name);
  context->name = sn_internal_strdup (name);
}

void
sn_launcher_context_set_launch_description (SnLauncherContext *context,
                                            const char        *description)  
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->description);
  context->description = sn_internal_strdup (description);
}

void
sn_launcher_context_set_launch_workspace (SnLauncherContext *context,
                                          int                workspace)
{
  WARN_ALREADY_INITIATED (context);

  context->workspace = workspace;
}

void
sn_launcher_context_set_legacy_resource_class (SnLauncherContext *context,
                                               const char        *klass)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->resource_class);
  context->resource_class = sn_internal_strdup (klass);
}

void
sn_launcher_context_set_legacy_resource_name (SnLauncherContext *context,
                                              const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->resource_name);
  context->resource_name = sn_internal_strdup (name);
}

void
sn_launcher_context_set_legacy_window_title (SnLauncherContext *context,
                                             const char        *title)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->window_title);
  context->window_title = sn_internal_strdup (title);
}

void
sn_launcher_context_set_binary_name (SnLauncherContext *context,
                                     const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->binary_name);
  context->binary_name = sn_internal_strdup (name);
}

void
sn_launcher_context_set_pid (SnLauncherContext *context,
                             int                pid)
{
  context->pid = pid;

  /* set the X property if launch window already exists */
  if (context->launch_id != NULL)
    {
      sn_internal_set_cardinal (context->display,
                                context->launch_window,
                                "_NET_LAUNCH_PID",
                                context->pid);
    }
}

void
sn_launcher_context_set_icon_name (SnLauncherContext *context,
                                   const char        *name)
{
  WARN_ALREADY_INITIATED (context);

  sn_free (context->icon_name);
  context->icon_name = sn_internal_strdup (name);
}

struct SnLauncherEvent
{
  int refcount;
  SnLauncherEventType type;
  Time timestamp;
  SnLauncherContext *context;
};

/**
 * sn_launcher_event_copy:
 * @event: event to copy
 *
 * Creates a copy of @event, the copy has a reference count of one.
 *
 * Return value: a new #SnLauncherEvent that's a copy of @event
 **/
SnLauncherEvent*
sn_launcher_event_copy (SnLauncherEvent *event)
{
  SnLauncherEvent *copy;

  copy = sn_new (SnLauncherEvent, 1);

  copy->refcount = 1;
  copy->type = event->type;
  copy->timestamp = event->timestamp;
  copy->context = event->context;
  if (copy->context)
    sn_launcher_context_ref (copy->context);

  return copy;
}

/**
 * sn_launcher_event_ref:
 * @event: a #SnLauncherEvent
 *
 * Increments @event's reference count.
 **/
void
sn_launcher_event_ref (SnLauncherEvent *event)
{
  event->refcount += 1;
}

/**
 * sn_launcher_event_unref:
 * @event: a #SnLauncherEvent
 *
 * Decrements @event's reference count and frees @event
 * if the count reaches zero.
 **/
void
sn_launcher_event_unref (SnLauncherEvent *event)
{
  event->refcount -= 1;

  if (event->refcount == 0)
    {
      if (event->context)
        sn_launcher_context_unref (event->context);
      sn_free (event);
    }
}

/**
 * sn_launcher_event_get_type:
 * @event: a #SnLauncherEvent
 *
 * Gets the type of the launcher event.
 *
 * Return value: the type of event
 **/
SnLauncherEventType
sn_launcher_event_get_type (SnLauncherEvent *event)
{
  return event->type;
}

/**
 * sn_launcher_event_get_context:
 * @event: a #SnLauncherEvent
 *
 * Gets the context associated with @event. The
 * returned context is owned by the event, i.e.
 * the caller of sn_launcher_event_get_context() should
 * not unref the context.
 *
 * Return value: the context for this event
 **/
SnLauncherContext*
sn_launcher_event_get_context (SnLauncherEvent *event)
{
  return event->context;
}

/**
 * sn_launcher_event_get_time:
 * @event: a #SnLauncherEvent
 *
 * Gets the X Window System timestamp associated with this launcher
 * event.
 *
 * Return value: timestamp for the event, or CurrentTime if none available
 **/
Time
sn_launcher_event_get_time (SnLauncherEvent *event)
{
  return event->timestamp;
}

static sn_bool_t
check_cardinal_exists (SnDisplay  *display,
                       Window      xwindow,
                       const char *property)
{
  int val;

  return sn_internal_get_cardinal (display, xwindow, property,
                                   &val);
}

typedef struct
{
  SnDisplay *display;
  Window     launch_window;
  sn_bool_t  result;
} HaveContextsData;

static sn_bool_t
have_active_contexts_foreach (void *value,
                              void *data)
{
  SnLauncherContext *context = value;
  HaveContextsData *hcd = data;

  if (!context->completed &&
      context->launch_window == hcd->launch_window &&
      sn_display_get_x_display (context->display) ==
      sn_display_get_x_display (hcd->display))
    {
      hcd->result = TRUE;
      return FALSE;
    }

  return TRUE;
}

typedef struct
{
  SnDisplay *display;
  Window     launch_window;
  SnList    *contexts;
} FindContextsData;

static sn_bool_t
find_active_contexts_foreach (void *value,
                              void *data)
{
  SnLauncherContext *context = value;
  FindContextsData *fcd = data;

  if (!context->completed &&
      context->launch_window == fcd->launch_window &&
      sn_display_get_x_display (context->display) ==
      sn_display_get_x_display (fcd->display))
    sn_list_prepend (fcd->contexts, context);

  return TRUE;
}

typedef struct
{
  SnLauncherEvent *base_event;
  SnList *events;
} CreateEventsData;

static sn_bool_t
create_events_foreach (void *value,
                       void *data)
{
  SnLauncherContext *context = value;
  CreateEventsData *ced = data;
  SnLauncherEvent *event;

  event = sn_launcher_event_copy (ced->base_event);
  event->context = context;
  sn_launcher_context_ref (context);

  sn_list_prepend (ced->events, event);

  return TRUE;
}

static sn_bool_t
dispatch_events_foreach (void *value,
                         void *data)
{
  SnLauncherEvent *event = value;

  /* Filter out duplicate events and update flags */
  switch (event->type)
    {
    case SN_LAUNCHER_EVENT_CANCELED:
      if (event->context->canceled)
        {
          sn_launcher_event_unref (event);
          event = NULL;
        }
      else
        {
          event->context->canceled = TRUE;
        }
      break;
    case SN_LAUNCHER_EVENT_COMPLETED:
      if (event->context->completed)
        {
          sn_launcher_event_unref (event);
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
      sn_launcher_event_unref (event);
    }

  return TRUE;
}

static void
dispatch_event (SnDisplay       *display,
                Window           launch_window,
                SnLauncherEvent *event)
{
  /* Find all applicable contexts, create an event for each, and send
   * the events out.
   */
  FindContextsData fcd;
  CreateEventsData ced;

  fcd.display = display;
  fcd.launch_window = launch_window;
  fcd.contexts = sn_list_new ();

  if (context_list != NULL)
    sn_list_foreach (context_list, find_active_contexts_foreach, &fcd);

  ced.base_event = event;
  ced.events = sn_list_new ();
  sn_list_foreach (fcd.contexts, create_events_foreach, &ced);

  /* This unref's each event as it's dispatched */
  sn_list_foreach (ced.events, dispatch_events_foreach, NULL);

  sn_list_free (fcd.contexts);
  sn_list_free (ced.events);
}
     
sn_bool_t
sn_internal_launcher_process_event (SnDisplay *display,
                                    XEvent    *xevent)
{
  sn_bool_t retval;
  SnLauncherEvent *event;
  Window event_xwindow;

  if (context_list == NULL ||
      sn_list_empty (context_list))
    return FALSE; /* no one cares */
  
  event_xwindow = None;
  event = NULL;
  retval = FALSE;

  switch (xevent->xany.type)
    {
    case PropertyNotify:
      if (xevent->xproperty.atom ==
          sn_internal_atom_get (display, "_NET_LAUNCH_CANCELED"))
        {
          event_xwindow = xevent->xproperty.window;

          if (check_cardinal_exists (display, event_xwindow,
                                     "_NET_LAUNCH_CANCELED"))
            {
              event = sn_new (SnLauncherEvent, 1);

              event->refcount = 1;
              event->type = SN_LAUNCHER_EVENT_CANCELED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
            }

          retval = TRUE;
        }
      else if (xevent->xproperty.atom ==
               sn_internal_atom_get (display, "_NET_LAUNCH_COMPLETE"))
        {
          event_xwindow = xevent->xproperty.window;

          if (check_cardinal_exists (display, event_xwindow,
                                     "_NET_LAUNCH_COMPLETE"))
            {
              event = sn_new (SnLauncherEvent, 1);

              event->refcount = 1;
              event->type = SN_LAUNCHER_EVENT_COMPLETED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
            }

          retval = TRUE;
        }
      break;

    case ClientMessage:
      if (xevent->xclient.message_type ==
          sn_internal_atom_get (display,
                                "_NET_LAUNCH_PULSE"))
        {
          event_xwindow = xevent->xclient.window;

          event = sn_new (SnLauncherEvent, 1);

          event->refcount = 1;
          event->type = SN_LAUNCHER_EVENT_PULSE;
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
          sn_list_foreach (context_list, have_active_contexts_foreach, &hcd);
        if (hcd.result)
          {
            event_xwindow = hcd.launch_window;

            event = sn_new (SnLauncherEvent, 1);

            event->refcount = 1;
            event->type = SN_LAUNCHER_EVENT_COMPLETED;
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
      
      sn_launcher_event_unref (event);
    }

  return retval;
}

