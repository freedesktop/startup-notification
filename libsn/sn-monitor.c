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
#include "lf-monitor.h"
#include "lf-internals.h"
#include "lf-xmessages.h"

#define KDE_STARTUP_INFO_ATOM "_KDE_STARTUP_INFO"

struct LfMonitorContext
{
  int refcount;
  LfDisplay *display;
  LfMonitorEventFunc event_func;
  void *event_func_data;
  LfFreeFunc free_data_func;
  /* a context doesn't get events for sequences
   * started prior to context creation
   */
  int creation_serial; 
};

struct LfMonitorEvent
{
  int refcount;
  LfMonitorEventType type;
  LfMonitorContext *context;
  LfLaunchSequence *sequence;
  Time timestamp;
};

struct LfLaunchSequence
{
  int refcount;

  char *id;
  LfDisplay *display;

  /* launch_window is NULL for Xmessage-based launches */
  Window launch_window;

  char *name;
  char *description;

  char *resource_class;
  char *resource_name;
  char *window_title;

  int workspace;

  char *binary_name;
  char *hostname;
  char *icon_name;
  int pid;
  
  /* geometry */
  Window geometry_window;
  int x, y, width, height;

  unsigned int geometry_set : 1;
  unsigned int canceled : 1;
  unsigned int completed : 1;
  unsigned int supports_cancel : 1;
  
  int creation_serial;
};

static LfList *context_list = NULL;
static LfList *sequence_list = NULL;
static int next_sequence_serial = 0;

static void xmessage_func (LfDisplay       *display,
                           const char      *message_type,
                           const char      *message,
                           void            *user_data);

/**
 * lf_monitor_context_new:
 * @display: an #LfDisplay
 * @event_func: function to call when an event is received
 * @event_func_data: extra data to pass to @event_func
 * @free_data_func: function to free @event_func_data when the context is freed
 *
 * Creates a new context for monitoring launch sequences.  Normally
 * only launch feedback indicator applications such as the window
 * manager or task manager would use #LfMonitorContext.
 * #LfLauncherContext and #LfLauncheeContext are more often used by
 * applications.
 *
 * To detect launch sequence initiations, PropertyChangeMask must be
 * selected on all root windows for a display.  liblf does not do this
 * for you because it's pretty likely to mess something up. So you
 * have to do it yourself in programs that use #LfMonitorContext.
 * 
 * Return value: a new #LfMonitorContext
 **/
LfMonitorContext*
lf_monitor_context_new (LfDisplay           *display,
                        LfMonitorEventFunc   event_func,
                        void                *event_func_data,
                        LfFreeFunc           free_data_func)
{
  LfMonitorContext *context;
  
  context = lf_new0 (LfMonitorContext, 1);

  context->refcount = 1;
  context->event_func = event_func;
  context->event_func_data = event_func_data;
  context->free_data_func = free_data_func;

  context->display = display;
  lf_display_ref (context->display);

  if (context_list == NULL)
    context_list = lf_list_new ();

  if (lf_list_empty (context_list))
    lf_internal_add_xmessage_func (display,
                                   KDE_STARTUP_INFO_ATOM,
                                   xmessage_func,
                                   NULL, NULL);
    
  lf_list_prepend (context_list, context);

  /* We get events for serials >= creation_serial */
  context->creation_serial = next_sequence_serial;
  
  return context;
}

/**
 * lf_monitor_context_ref:
 * @context: an #LfMonitorContext
 *
 * Increments the reference count on @context.
 * 
 **/
void
lf_monitor_context_ref (LfMonitorContext *context)
{
  context->refcount += 1;
}

/**
 * lf_monitor_context_unref:
 * @context: an #LfMonitorContext
 * 
 * Decrements the reference count on @context and frees the
 * context if the count reaches 0.
 **/
void
lf_monitor_context_unref (LfMonitorContext *context)
{
  context->refcount -= 1;

  if (context->refcount == 0)
    {
      lf_list_remove (context_list, context);

      if (lf_list_empty (context_list))
        lf_internal_remove_xmessage_func (context->display,
                                          KDE_STARTUP_INFO_ATOM,
                                          xmessage_func,
                                          NULL);
      
      if (context->free_data_func)
        (* context->free_data_func) (context->event_func_data);
      
      lf_display_unref (context->display);
      lf_free (context);
    }
}

void
lf_monitor_event_ref (LfMonitorEvent *event)
{
  event->refcount += 1;
}

void
lf_monitor_event_unref (LfMonitorEvent *event)
{
  event->refcount -= 1;

  if (event->refcount == 0)
    {
      if (event->context)
        lf_monitor_context_unref (event->context);
      if (event->sequence)
        lf_launch_sequence_unref (event->sequence);
      lf_free (event);
    }
}

LfMonitorEvent*
lf_monitor_event_copy (LfMonitorEvent *event)
{
  LfMonitorEvent *copy;

  copy = lf_new0 (LfMonitorEvent, 1);

  copy->refcount = 1;

  copy->type = event->type;
  copy->context = event->context;
  if (copy->context)
    lf_monitor_context_ref (copy->context);
  copy->sequence = event->sequence;
  if (copy->sequence)
    lf_launch_sequence_ref (copy->sequence);

  copy->timestamp = event->timestamp;

  return copy;
}

LfMonitorEventType
lf_monitor_event_get_type (LfMonitorEvent *event)
{
  return event->type;
}

LfLaunchSequence*
lf_monitor_event_get_launch_sequence (LfMonitorEvent *event)
{
  return event->sequence;
}

LfMonitorContext*
lf_monitor_event_get_context (LfMonitorEvent *event)
{
  return event->context;
}

Time
lf_monitor_event_get_time (LfMonitorEvent *event)
{
  return event->timestamp;
}

static void
update_geometry (LfLaunchSequence *sequence)
{
  int *vals;
  int n_vals;

  sequence->geometry_set = FALSE;

  vals = NULL;
  n_vals = 0;
  if (lf_internal_get_cardinal_list (sequence->display,
                                     sequence->launch_window,
                                     "_NET_LAUNCH_GEOMETRY",
                                     &vals, &n_vals) &&
      n_vals == 4)
    {
      sequence->x = vals[0];
      sequence->y = vals[1];
      sequence->width = vals[2];
      sequence->height = vals[3];
    }

  lf_free (vals);
}

static void
update_pid (LfLaunchSequence *sequence)
{
  int val;
  
  sequence->pid = -1;

  val = -1;
  if (lf_internal_get_cardinal (sequence->display,
                                sequence->launch_window,
                                "_NET_LAUNCH_PID",
                                &val))
    sequence->pid = val;
}

static LfLaunchSequence*
lf_launch_sequence_new (LfDisplay *display,
                        Window     launch_window)
{
  LfLaunchSequence *sequence;
  char *id;
  int val;
  
  /* Select input, then get _NET_LAUNCH_ID,
   * because we want to be sure we detect a BadWindow
   * if it happens prior to selecting input, and
   * getting _NET_LAUNCH_ID will bomb on BadWindow
   */
  if (launch_window != None)   /* launch_window may be NULL for xmessage sequence */
    {
      lf_display_error_trap_push (display);
      XSelectInput (lf_display_get_x_display (display),
                    launch_window,
                    PropertyChangeMask | StructureNotifyMask);
      lf_display_error_trap_pop (display);
  
      id = NULL;
      if (!lf_internal_get_string (display, launch_window,
                                   "_NET_LAUNCH_ID", &id))
        {
          return NULL;
        }
    }
  
  sequence = lf_new0 (LfLaunchSequence, 1);

  sequence->refcount = 1;

  sequence->creation_serial = next_sequence_serial;
  ++next_sequence_serial;
  
  sequence->id = id;
  sequence->launch_window = launch_window;
  sequence->display = display;
  lf_display_ref (display);

  sequence->workspace = -1; /* not set */
  sequence->pid = -1;

  /* Update stuff that can change over time */
  update_geometry (sequence);
  update_pid (sequence);  

  if (sequence->launch_window != None)
    {
      /* Grab all the stuff that can't be changed later */
      lf_internal_get_utf8_string (sequence->display,
                                   sequence->launch_window,
                                   "_NET_LAUNCH_NAME",
                                   &sequence->name);
      lf_internal_get_utf8_string (sequence->display,
                                   sequence->launch_window,
                                   "_NET_LAUNCH_DESCRIPTION",
                                   &sequence->description);
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_CLASS",
                              &sequence->resource_class);
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_LEGACY_RESOURCE_NAME",
                              &sequence->resource_name);
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_LEGACY_NAME",
                              &sequence->window_title);
      if (lf_internal_get_cardinal (sequence->display,
                                    sequence->launch_window,
                                    "_NET_LAUNCH_DESKTOP",
                                    &val))
        sequence->workspace = val;
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_BINARY_NAME",
                              &sequence->binary_name);
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_HOSTNAME",
                              &sequence->hostname);
      lf_internal_get_string (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_ICON_NAME",
                              &sequence->icon_name);
      lf_internal_get_window (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_GEOMETRY_WINDOW",
                              &sequence->geometry_window);

      if (lf_internal_get_cardinal (sequence->display,
                                    sequence->launch_window,
                                    "_NET_LAUNCH_SUPPORTS_CANCEL",
                                    &val))
        sequence->supports_cancel = val != 0;
    }
  
  return sequence;
}

void
lf_launch_sequence_ref (LfLaunchSequence *sequence)
{
  sequence->refcount += 1;
}

void
lf_launch_sequence_unref (LfLaunchSequence *sequence)
{
  sequence->refcount -= 1;

  if (sequence->refcount == 0)
    {      
      lf_free (sequence->id);

      lf_free (sequence->name);
      lf_free (sequence->description);
      lf_free (sequence->resource_class);
      lf_free (sequence->resource_name);
      lf_free (sequence->window_title);
      lf_free (sequence->binary_name);
      lf_free (sequence->icon_name);
      lf_free (sequence->hostname);
      
      lf_display_unref (sequence->display);
      lf_free (sequence);
    }
}

const char*
lf_launch_sequence_get_id (LfLaunchSequence *sequence)
{
  return sequence->id;
}

Window
lf_launch_sequence_get_window (LfLaunchSequence *sequence)
{
  return sequence->launch_window;
}

lf_bool_t
lf_launch_sequence_get_geometry (LfLaunchSequence *sequence,
                                 int              *x,
                                 int              *y,
                                 int              *width,
                                 int              *height)
{
  if (sequence->geometry_set)
    {
      *x = sequence->x;
      *y = sequence->y;
      *width = sequence->width;
      *height = sequence->height;
      return TRUE;
    }
  else
    {
      *x = 0;
      *y = 0;
      *width = 0;
      *height = 0;
      return FALSE;
    }
}

Window
lf_launch_sequence_get_geometry_window (LfLaunchSequence *sequence)
{
  return sequence->geometry_window;
}

lf_bool_t
lf_launch_sequence_get_completed (LfLaunchSequence *sequence)
{
  return sequence->completed;
}

lf_bool_t
lf_launch_sequence_get_canceled (LfLaunchSequence *sequence)
{
  return sequence->canceled;
}

const char*
lf_launch_sequence_get_name (LfLaunchSequence *sequence)
{
  return sequence->name;
}

const char*
lf_launch_sequence_get_description (LfLaunchSequence *sequence)
{
  return sequence->description;
}

int
lf_launch_sequence_get_workspace (LfLaunchSequence *sequence)
{
  return sequence->workspace;
}

const char*
lf_launch_sequence_get_legacy_resource_class (LfLaunchSequence *sequence)
{
  return sequence->resource_class;
}

const char*
lf_launch_sequence_get_legacy_resource_name (LfLaunchSequence *sequence)
{
  return sequence->resource_name;
}

const char*
lf_launch_sequence_get_legacy_window_title (LfLaunchSequence *sequence)
{
  return sequence->window_title;
}

lf_bool_t
lf_launch_sequence_get_supports_cancel (LfLaunchSequence *sequence)
{
  return sequence->supports_cancel;
}

int
lf_launch_sequence_get_pid (LfLaunchSequence *sequence)
{
  return sequence->pid;
}

const char*
lf_launch_sequence_get_binary_name (LfLaunchSequence *sequence)
{
  return sequence->binary_name;
}

const char*
lf_launch_sequence_get_hostname (LfLaunchSequence *sequence)
{
  return sequence->hostname;
}

const char*
lf_launch_sequence_get_icon_name (LfLaunchSequence *sequence)
{
  return sequence->icon_name;
}

void
lf_launch_sequence_cancel (LfLaunchSequence *sequence)
{
  if (sequence->supports_cancel)
    lf_internal_set_cardinal (sequence->display,
                              sequence->launch_window,
                              "_NET_LAUNCH_CANCELED",
                              0);
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
  LfMonitorEvent *base_event;
  LfList *events;
} CreateContextEventsData;

static lf_bool_t
create_context_events_foreach (void *value,
                               void *data)
{
  /* Make a list of events holding a ref to the context they'll go to,
   * for reentrancy robustness
   */
  LfMonitorContext *context = value;
  CreateContextEventsData *ced = data;

  /* Don't send events for launch sequences initiated before the
   * context was created
   */
  if (ced->base_event->sequence->creation_serial >=
      context->creation_serial)
    {
      LfMonitorEvent *copy;
      
      copy = lf_monitor_event_copy (ced->base_event);
      copy->context = context;
      lf_monitor_context_ref (copy->context);
      
      lf_list_prepend (ced->events, copy);
    }
  
  return TRUE;
}

static lf_bool_t
dispatch_event_foreach (void *value,
                        void *data)
{
  LfMonitorEvent *event = value;

  /* Dispatch and free events */
  
  if (event->context->event_func)
    (* event->context->event_func) (event,
                                    event->context->event_func_data);

  lf_monitor_event_unref (event);

  return TRUE;
}

static lf_bool_t
filter_event (LfMonitorEvent *event)
{
  lf_bool_t retval;

  retval = FALSE;

  /* Filter out duplicate events and update flags */
  switch (event->type)
    {
    case LF_MONITOR_EVENT_CANCELED:
      if (event->sequence->canceled)
        {
          retval = TRUE;
        }
      else
        {
          event->sequence->canceled = TRUE;
        }
      break;
    case LF_MONITOR_EVENT_COMPLETED:
      if (event->sequence->completed)
        {
          retval = TRUE;
        }
      else
        {
          event->sequence->completed = TRUE;
        }
      break;

    default:
      break;
    }
  
  return retval;
}

typedef struct
{
  LfDisplay *display;
  Window   launch_window;
  LfLaunchSequence *found;
} FindSequenceData;

static lf_bool_t
find_sequence_foreach (void *value,
                       void *data)
{
  LfLaunchSequence *sequence = value;
  FindSequenceData *fsd = data;
  
  if (sequence->launch_window == fsd->launch_window &&
      lf_display_get_x_display (sequence->display) ==
      lf_display_get_x_display (fsd->display))
    {
      fsd->found = sequence;
      return FALSE;
    }

  return TRUE;
}

static LfLaunchSequence*
find_sequence_for_window (LfDisplay      *display,
                          Window          event_window)
{
  FindSequenceData fsd;
  
  if (sequence_list == NULL)
    return NULL;
  
  fsd.display = display;
  fsd.launch_window = event_window;
  fsd.found = NULL;
  
  lf_list_foreach (sequence_list, find_sequence_foreach, &fsd);

  return fsd.found;
}

static LfLaunchSequence*
add_sequence (LfDisplay *display,
              Window     event_xwindow)
{
  LfLaunchSequence *sequence;
  
  sequence =
    lf_launch_sequence_new (display, event_xwindow);
  
  if (sequence)
    {
      lf_launch_sequence_ref (sequence); /* ref held by sequence list */
      if (sequence_list == NULL)
        sequence_list = lf_list_new ();
      lf_list_prepend (sequence_list, sequence);
    }

  return sequence;
}

static void
remove_sequence (LfLaunchSequence *sequence)
{
  lf_list_remove (sequence_list, sequence);
  lf_launch_sequence_unref (sequence);
}

static void
dispatch_monitor_event (LfDisplay      *display,
                        LfMonitorEvent *event,
                        Window          event_xwindow)
{
  if (event->type == LF_MONITOR_EVENT_INITIATED)
    {
      if (event->sequence == NULL)
        event->sequence = add_sequence (display, event_xwindow);
    }  
  else if (event->sequence == NULL)
    {
      event->sequence = find_sequence_for_window (display,
                                                  event_xwindow);
      if (event->sequence)
        lf_launch_sequence_ref (event->sequence); /* ref held by event */
    }

  if (event->sequence != NULL)
    {
      switch (event->type)
        {
        case LF_MONITOR_EVENT_GEOMETRY_CHANGED:
          update_geometry (event->sequence);
          break;

        case LF_MONITOR_EVENT_PID_CHANGED:
          update_pid (event->sequence);
          break;
          
        default:
          break;
        }
    }
      
  if (event->sequence != NULL &&
      !filter_event (event))
    {
      CreateContextEventsData cced;
          
      cced.base_event = event;
      cced.events = lf_list_new ();
          
      lf_list_foreach (context_list, create_context_events_foreach,
                       &cced);
          
      lf_list_foreach (cced.events, dispatch_event_foreach, NULL);

      /* values in the events list freed on dispatch */
      lf_list_free (cced.events);

      /* remove from sequence list */
      if (event->type == LF_MONITOR_EVENT_COMPLETED)
        remove_sequence (event->sequence);
    }
}

lf_bool_t
lf_internal_monitor_process_event (LfDisplay *display,
                                   XEvent    *xevent)
{
  lf_bool_t retval;
  LfMonitorEvent *event;
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
              event = lf_new (LfMonitorEvent, 1);

              event->refcount = 1;
              event->type = LF_MONITOR_EVENT_CANCELED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
              event->sequence = NULL;

              retval = TRUE;
            }
        }
      else if (xevent->xproperty.atom ==
               lf_internal_atom_get (display, "_NET_LAUNCH_COMPLETE"))
        {
          event_xwindow = xevent->xproperty.window;

          if (check_cardinal_exists (display, event_xwindow,
                                     "_NET_LAUNCH_COMPLETE"))
            {
              event = lf_new (LfMonitorEvent, 1);

              event->refcount = 1;
              event->type = LF_MONITOR_EVENT_COMPLETED;
              event->timestamp = xevent->xproperty.time;
              event->context = NULL;
              event->sequence = NULL;

              retval = TRUE;
            }
        }
      else if (xevent->xproperty.atom ==
               lf_internal_atom_get (display, "_NET_LAUNCH_GEOMETRY"))
        {
          event_xwindow = xevent->xproperty.window;
          
          event = lf_new (LfMonitorEvent, 1);
          
          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_GEOMETRY_CHANGED;
          event->timestamp = xevent->xproperty.time;
          event->context = NULL;
          event->sequence = NULL;

          retval = TRUE;
        }
      else if (xevent->xproperty.atom ==
               lf_internal_atom_get (display, "_NET_LAUNCH_PID"))
        {
          event_xwindow = xevent->xproperty.window;
          
          event = lf_new (LfMonitorEvent, 1);
          
          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_PID_CHANGED;
          event->timestamp = xevent->xproperty.time;
          event->context = NULL;
          event->sequence = NULL;

          retval = TRUE;
        }
      break;

    case ClientMessage:
      if (xevent->xclient.message_type ==
          lf_internal_atom_get (display,
                                "_NET_LAUNCH_PULSE"))
        {
          event_xwindow = xevent->xclient.window;

          event = lf_new (LfMonitorEvent, 1);

          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_PULSE;
          event->timestamp = CurrentTime;
          event->context = NULL;
          event->sequence = NULL;

          retval = TRUE;
        }
      else if (xevent->xclient.message_type ==
               lf_internal_atom_get (display,
                                     "_NET_LAUNCH_INITIATE"))
        {
          LfLaunchSequence *sequence;
          
          /* Don't be fooled by duplicate initiate messages -
           * check that the sequence doesn't exist yet
           */
          sequence = find_sequence_for_window (display,
                                               xevent->xclient.window);

          if (sequence == NULL)
            {
              event_xwindow = xevent->xclient.window;
              
              event = lf_new (LfMonitorEvent, 1);
              
              event->refcount = 1;
              event->type = LF_MONITOR_EVENT_INITIATED;
              event->timestamp = xevent->xclient.data.l[0];
              event->context = NULL;
              event->sequence = NULL;
            }

          retval = TRUE;
        }
      break;

    case DestroyNotify:
      {
        LfLaunchSequence *sequence;

        sequence = find_sequence_for_window (display,
                                             xevent->xdestroywindow.window);

        if (sequence != NULL)
          {
            event_xwindow = xevent->xdestroywindow.window;

            event = lf_new (LfMonitorEvent, 1);

            event->refcount = 1;
            event->type = LF_MONITOR_EVENT_COMPLETED;
            event->timestamp = CurrentTime;
            event->context = NULL;
            event->sequence = sequence;
            lf_launch_sequence_ref (sequence);
          }
      }
      break;

    default:
      break;
    }

  if (event != NULL)
    {
      dispatch_monitor_event (display, event, event_xwindow);
      
      lf_monitor_event_unref (event);
    }

  return retval;
}

typedef struct
{
  LfDisplay *display;
  const char *id;
  LfLaunchSequence *found;
} FindSequenceByIdData;

static lf_bool_t
find_sequence_by_id_foreach (void *value,
                             void *data)
{
  LfLaunchSequence *sequence = value;
  FindSequenceByIdData *fsd = data;
  
  if (strcmp (sequence->id, fsd->id) == 0 &&
      lf_display_get_x_display (sequence->display) ==
      lf_display_get_x_display (fsd->display))
    {
      fsd->found = sequence;
      return FALSE;
    }

  return TRUE;
}

static LfLaunchSequence*
find_sequence_for_id (LfDisplay      *display,
                      const char     *id)
{
  FindSequenceByIdData fsd;
  
  if (sequence_list == NULL)
    return NULL;
  
  fsd.display = display;
  fsd.id = id;
  fsd.found = NULL;
  
  lf_list_foreach (sequence_list, find_sequence_by_id_foreach, &fsd);

  return fsd.found;
}

static lf_bool_t
do_xmessage_event_foreach (void *value,
                           void *data)
{
  LfMonitorEvent *event = value;
  LfDisplay *display = data;
  
  dispatch_monitor_event (display, event, None);

  return TRUE;
}

static lf_bool_t
unref_event_foreach (void *value,
                     void *data)
{
  lf_monitor_event_unref (value);
  return TRUE;
}

static void
xmessage_func (LfDisplay  *display,
               const char *message_type,
               const char *message,
               void       *user_data)
{
  /* assert (strcmp (message_type, KDE_STARTUP_INFO_ATOM) == 0); */
  char *prefix;
  char **names;
  char **values;
  int i;
  const char *launch_id;
  LfLaunchSequence *sequence;
  LfList *events;
  
  prefix = NULL;
  names = NULL;
  values = NULL;
  if (!lf_internal_unserialize_message (message, &prefix, &names, &values))
    return;
  
  launch_id = NULL;
  i = 0;
  while (names[i])
    {
      if (strcmp (names[i], "ID") == 0)
        {
          launch_id = values[i];
          break;
        }
      ++i;
    }

  events = lf_list_new ();
  
  if (launch_id == NULL)
    goto out;
  
  sequence = find_sequence_for_id (display, launch_id);

  if (strcmp (prefix, "new") == 0)
    {
      if (sequence == NULL)
        {
          LfMonitorEvent *event;

          sequence = add_sequence (display, None);
          if (sequence == NULL)
            goto out;
          
          sequence->id = lf_internal_strdup (launch_id);
          
          event = lf_new (LfMonitorEvent, 1);
          
          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_INITIATED;
          event->timestamp = CurrentTime;
          event->context = NULL;
          event->sequence = sequence; /* ref from add_sequence goes here */

          lf_list_append (events, event);
        }
    }

  if (sequence == NULL)
    goto out;
  
  if (strcmp (prefix, "change") == 0 ||
      strcmp (prefix, "new") == 0)
    {
      lf_bool_t random_stuff_changed = FALSE;
      lf_bool_t pid_changed = FALSE;
      lf_bool_t workspace_changed = FALSE;

      i = 0;
      while (names[i])
        {
          if (strcmp (names[i], "BIN") == 0)
            {
              if (sequence->binary_name == NULL)
                {
                  sequence->binary_name = lf_internal_strdup (values[i]);
                  random_stuff_changed = TRUE;
                }              
            }
          else if (strcmp (names[i], "NAME") == 0)
            {
              if (sequence->name == NULL)
                {
                  sequence->name = lf_internal_strdup (values[i]);
                  random_stuff_changed = TRUE;
                }
            }
          else if (strcmp (names[i], "ICON") == 0)
            {
              if (sequence->icon_name == NULL)
                {
                  sequence->icon_name = lf_internal_strdup (values[i]);
                  random_stuff_changed = TRUE;
                }
            }
          else if (strcmp (names[i], "DESKTOP") == 0)
            {
              int workspace;
              
              workspace = lf_internal_string_to_ulong (values[i]);

              sequence->workspace = workspace;
              workspace_changed = TRUE;
            }
          else if (strcmp (names[i], "WMCLASS") == 0)
            {
              if (sequence->resource_class == NULL)
                {
                  sequence->resource_class = lf_internal_strdup (values[i]);
                  random_stuff_changed = TRUE;
                }
            }
          else if (strcmp (names[i], "PID") == 0)
            {
              int pid;
              
              pid = lf_internal_string_to_ulong (values[i]);

              if (pid > 0)
                {
                  sequence->pid = pid;
              
                  pid_changed = TRUE;
                }
            }
          else if (strcmp (names[i], "HOSTNAME") == 0)
            {
              if (sequence->hostname == NULL)
                {
                  sequence->hostname = lf_internal_strdup (values[i]);
                  random_stuff_changed = TRUE;
                }
            }
          
          ++i;
        }

      if (pid_changed)
        {
          LfMonitorEvent *event;
          
          event = lf_new (LfMonitorEvent, 1);
          
          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_PID_CHANGED;
          event->timestamp = CurrentTime;
          event->context = NULL;
          event->sequence = sequence;
          lf_launch_sequence_ref (sequence);

          lf_list_append (events, event);
        }

      if (workspace_changed)
        {
          LfMonitorEvent *event;
          
          event = lf_new (LfMonitorEvent, 1);
          
          event->refcount = 1;
          event->type = LF_MONITOR_EVENT_WORKSPACE_CHANGED;
          event->timestamp = CurrentTime;
          event->context = NULL;
          event->sequence = sequence;
          lf_launch_sequence_ref (sequence);

          lf_list_append (events, event);
        }
      
      if (random_stuff_changed)
        {
          /* FIXME */
        }          
    }
  else if (strcmp (prefix, "remove") == 0)
    {
      LfMonitorEvent *event;
      
      event = lf_new (LfMonitorEvent, 1);
      
      event->refcount = 1;
      event->type = LF_MONITOR_EVENT_COMPLETED;
      event->timestamp = CurrentTime;
      event->context = NULL;
      event->sequence = sequence;
      lf_launch_sequence_ref (sequence);

      lf_list_append (events, event);
    }

  lf_list_foreach (events,
                   do_xmessage_event_foreach,
                   display); 
  
 out:
  if (events != NULL)
    {
      lf_list_foreach (events, unref_event_foreach, NULL);
      lf_list_free (events);
    }
  
  lf_free (prefix);
  lf_internal_strfreev (names);
  lf_internal_strfreev (values);
}
