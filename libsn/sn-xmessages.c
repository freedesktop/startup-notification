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

#include "sn-xmessages.h"
#include "sn-list.h"
#include "sn-internals.h"

typedef struct
{
  Display       *xdisplay;
  Atom           type_atom;
  char          *message_type;
  SnXmessageFunc func;
  void          *func_data;
  SnFreeFunc     free_data_func;
} SnXmessageHandler;

typedef struct
{
  Atom type_atom;
  Window xwindow;
  char *message;
  int allocated;
} SnXmessage;

static SnList *xmessage_funcs = NULL;
static SnList *pending_messages = NULL;

void
sn_internal_add_xmessage_func (SnDisplay      *display,
                               const char     *message_type,
                               SnXmessageFunc  func,
                               void           *func_data,
                               SnFreeFunc      free_data_func)
{
  SnXmessageHandler *handler;
  
  if (xmessage_funcs == NULL)
    xmessage_funcs = sn_list_new ();

  handler = sn_new0 (SnXmessageHandler, 1);

  handler->xdisplay = sn_display_get_x_display (display);
  handler->type_atom = sn_internal_atom_get (display, message_type);
  handler->message_type = sn_internal_strdup (message_type);
  handler->func= func;
  handler->func_data = func_data;
  handler->free_data_func = free_data_func;
  
  sn_list_prepend (xmessage_funcs, handler);
}

typedef struct
{
  const char *message_type;
  SnXmessageFunc func;
  void *func_data;
  SnXmessageHandler *handler;
} FindHandlerData;

static sn_bool_t
find_handler_foreach (void *value,
                      void *data)
{
  FindHandlerData *fhd = data;
  SnXmessageHandler *handler = value;

  if (handler->func == fhd->func &&
      handler->func_data == fhd->func_data &&
      strcmp (fhd->message_type, handler->message_type) == 0)
    {
      fhd->handler = handler;
      return FALSE;
    }

  return TRUE;
}

void
sn_internal_remove_xmessage_func (SnDisplay      *display,
                                  const char     *message_type,
                                  SnXmessageFunc  func,
                                  void           *func_data)
{
  FindHandlerData fhd;

  fhd.func = func;
  fhd.func_data = func_data;
  fhd.handler = NULL;

  if (xmessage_funcs != NULL)
    sn_list_foreach (xmessage_funcs, find_handler_foreach, &fhd);

  if (fhd.handler != NULL)
    {
      sn_list_remove (xmessage_funcs, fhd.handler);

      sn_free (fhd.handler->message_type);
      
      if (fhd.handler->free_data_func)
        (* fhd.handler->free_data_func) (fhd.handler->func_data);
      
      sn_free (fhd.handler);
    }
}

void
sn_internal_broadcast_xmessage   (SnDisplay      *display,
                                  const char     *message_type,
                                  const char     *message)
{
  Atom type_atom;
  Window xwindow;
  Display *xdisplay;

  if (!sn_internal_utf8_validate (message, -1))
    {
      fprintf (stderr,
               "Attempted to send non-UTF-8 X message: %s\n",
               message);
      return;
    }

  xdisplay = sn_display_get_x_display (display);

  {
    XSetWindowAttributes attrs;

    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

    xwindow =
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

  type_atom = sn_internal_atom_get (display, message_type);
  
  {
    XEvent xevent;
    const char *src;
    const char *src_end;
    char *dest;
    char *dest_end;
    
    xevent.xclient.type = ClientMessage;
    xevent.xclient.message_type = type_atom;
    xevent.xclient.display = xdisplay;
    xevent.xclient.window = xwindow;
    xevent.xclient.format = 8;

    src = message;
    src_end = message + strlen (message) + 1; /* +1 to include nul byte */
    
    while (src != src_end)
      {
        dest = &xevent.xclient.data.b[0];
        dest_end = dest + 20;
        
        while (dest != dest_end &&
               src != src_end)
          {
            *dest = *src;
            ++dest;
            ++src;
          }

        sn_internal_send_event_all_screens (display, PropertyChangeMask,
                                            &xevent);
      }
  }

  XDestroyWindow (xdisplay, xwindow);
  XFlush (xdisplay);
}

typedef struct
{
  Display *xdisplay;
  Atom atom;
  sn_bool_t found_handler;
} HandlerForAtomData;

static sn_bool_t
handler_for_atom_foreach (void *value,
                          void *data)
{
  SnXmessageHandler *handler = value;
  HandlerForAtomData *hfad = data;

  if (handler->xdisplay == hfad->xdisplay &&
      handler->type_atom == hfad->atom)
    {
      hfad->found_handler = TRUE;
      return FALSE;
    }
  else
    return TRUE;
}

static sn_bool_t
some_handler_handles_event (SnDisplay *display,
                            XEvent    *xevent)
{
  HandlerForAtomData hfad;

  hfad.atom = xevent->xclient.message_type;
  hfad.xdisplay = sn_display_get_x_display (display);
  hfad.found_handler = FALSE;

  if (xmessage_funcs)
    sn_list_foreach (xmessage_funcs,
                     handler_for_atom_foreach,
                     &hfad);

  return hfad.found_handler;
}

typedef struct
{
  Display *xdisplay;
  XEvent *xevent;
  SnXmessage *message;
} FindMessageData;

static sn_bool_t
find_message_foreach (void *value,
                      void *data)
{
  SnXmessage *message = value;
  FindMessageData *fmd = data;

  if (fmd->xevent->xclient.window ==
      message->xwindow &&
      fmd->xevent->xclient.message_type ==
      message->type_atom)
    {
      fmd->message = message;
      return FALSE;
    }

  return TRUE;
}

static SnXmessage*
add_event_to_messages (SnDisplay *display,
                       XEvent    *xevent)
{
  FindMessageData fmd;
  SnXmessage *message;
  const char *src;
  const char *src_end;
  char *dest;
  sn_bool_t completed;
  
  /* We don't want screwy situations to end up causing us to allocate
   * infinite memory. Cap the length of a message.
   */
#define MAX_MESSAGE_LENGTH 4096
  
  fmd.xdisplay = sn_display_get_x_display (display);
  fmd.xevent = xevent;
  fmd.message = NULL;

  if (pending_messages)
    sn_list_foreach (pending_messages, find_message_foreach, &fmd);

  message = fmd.message;

  if (message == NULL)
    {
      /* Create a new message */
      message = sn_new0 (SnXmessage, 1);

      message->type_atom = xevent->xclient.message_type;
      message->xwindow = xevent->xclient.window;
      message->message = NULL;
      message->allocated = 0;

      if (pending_messages == NULL)
        pending_messages = sn_list_new ();

      sn_list_prepend (pending_messages, message);
    }

  if (message->allocated > MAX_MESSAGE_LENGTH)
    {
      /* This message is some kind of crap - just dump it. */
      sn_free (message->message);
      sn_list_remove (pending_messages, message);
      sn_free (message);
      return NULL;
    }
  
  src = &xevent->xclient.data.b[0];
  src_end = src + 20;
  
  message->message = sn_realloc (message->message,
                                 message->allocated + 20);
  dest = message->message + message->allocated;
  message->allocated += 20;

  completed = FALSE;
  
  /* Copy bytes, be sure we get nul byte also */
  while (src != src_end)
    {
      *dest = *src;

      if (*src == '\0')
        {
          completed = TRUE;
          break;
        }
      
      ++dest;
      ++src;    
    }
  
  if (completed)
    {
      /* Pull message out of the pending queue and return it */
      sn_list_remove (pending_messages, message);
      return message;
    }
  else
    return NULL;
}

typedef struct
{
  SnDisplay *display;
  SnXmessage *message;
} MessageDispatchData;

static sn_bool_t
dispatch_message_foreach (void *value,
                          void *data)
{
  SnXmessageHandler *handler = value;
  MessageDispatchData *mdd = data;  

  (* handler->func) (mdd->display,
                     handler->message_type,
                     mdd->message->message,
                     handler->func_data);
  
  return TRUE;
}

sn_bool_t
sn_internal_xmessage_process_event (SnDisplay *display,
                                    XEvent    *xevent)
{
  sn_bool_t retval;
  SnXmessage *message;

  retval = FALSE;
  message = NULL;
  
  switch (xevent->xany.type)
    {
    case ClientMessage:
      if (some_handler_handles_event (display, xevent))
        {
          retval = TRUE;

          message = add_event_to_messages (display, xevent);
        }
      break;
    }

  if (message != NULL)
    {
      /* We need to dispatch and free this message; ignore
       * messages containing invalid UTF-8
       */

      if (sn_internal_utf8_validate (message->message, -1))
        {
          MessageDispatchData mdd;
          
          mdd.display = display;
          mdd.message = message;
          
          /* We could stand to be more reentrant here; it will
           * barf if you add/remove a handler from inside the
           * dispatch
           */
          if (xmessage_funcs != NULL)
            sn_list_foreach (xmessage_funcs,
                             dispatch_message_foreach,
                             &mdd);
        }

      sn_free (message->message);
      sn_free (message);
    }
  
  return retval;
}

static void
append_to_string (char      **append_to,
                  int        *current_len,
                  const char *append)
{
  int len;
  char *end;
  
  len = strlen (append);

  *append_to = sn_realloc (*append_to, *current_len + len + 1);
  
  end = *append_to + *current_len;
  strcpy (end, append);
  *current_len = *current_len + len;  
}

static void
append_to_string_escaped (char      **append_to,
                          int        *current_len,
                          const char *append)
{
  char *escaped;
  int len;
  char buf[2];
  const char *p;
  
  buf[1] = '\0';
  len = 0;
  escaped = NULL;

  /* We are the most inefficient algorithm ever! woot! */
  /* really need GString here */
  p = append;
  while (*p)
    {
      if (*p == '\\' || *p == '"' || *p == ' ')
        {
          buf[0] = '\\';
          append_to_string (&escaped, &len, buf);
        }
      buf[0] = *p;
      append_to_string (&escaped, &len, buf);
      
      ++p;
    }

  append_to_string (append_to, current_len, escaped);

  sn_free (escaped);
}

char*
sn_internal_serialize_message (const char   *prefix,
                               const char  **property_names,
                               const char  **property_values)
{
  int len;
  char *retval;
  int i;
  
  /* GLib would simplify this a lot... */  
  len = 0;
  retval = NULL;

  append_to_string (&retval, &len, prefix);
  append_to_string (&retval, &len, ":  ");

  i = 0;
  while (property_names[i])
    {
      append_to_string (&retval, &len, property_names[i]);
      append_to_string (&retval, &len, "=");
      append_to_string_escaped (&retval, &len, property_values[i]);
      
      ++i;
    }

  return retval;
}

static void
append_string_to_list (char     ***list,
                       const char *append)
{
  if (*list == NULL)
    {
      *list = sn_new0 (char*, 2);
      (*list)[0] = sn_internal_strdup (append);
    }
  else
    {
      int i;

      i = 0;
      while ((*list)[i] != NULL)
        ++i;

      *list = sn_renew (char*, *list, i + 2);
      (*list)[i] = sn_internal_strdup (append);
      (*list)[i+1] = NULL;
    }
}

static char*
parse_prefix_up_to (const char *str,
                    int         up_to,
                    const char **end)
{
  char *prefix;
  const char *p;
  int len;
  
  prefix = NULL;
  *end = NULL;
  
  p = str;
  while (*p && *p != up_to)
    ++p;

  if (*p == '\0')
    return NULL;

  len = p - str;
  prefix = sn_internal_strndup (str, len);

  *end = str + len;

  return prefix;
}                    


/* Single quotes preserve the literal string exactly. escape
 * sequences are not allowed; not even \' - if you want a '
 * in the quoted text, you have to do something like 'foo'\''bar'
 *
 * Double quotes allow $ ` " \ and newline to be escaped with backslash.
 * Otherwise double quotes preserve things literally.
 *
 * (This is overkill for X messages, copied from GLib shell code,
 *  copyright Red Hat Inc. also)
 */

static sn_bool_t
unquote_string_inplace (char  *str,
                        char **end)
{
  char* dest;
  char* s;
  char quote_char;
  
  dest = s = str;

  quote_char = *s;
  
  if (!(*s == '"' || *s == '\''))
    {
      /* doesn't begin with quotation mark */
      *end = str;
      return FALSE;
    }

  /* Skip the initial quote mark */
  ++s;

  if (quote_char == '"')
    {
      while (*s)
        {
          /* g_assert(s > dest); */ /* loop invariant */
      
          switch (*s)
            {
            case '"':
              /* End of the string, return now */
              *dest = '\0';
              ++s;
              *end = s;
              return TRUE;
              break;

            case '\\':
              /* Possible escaped quote or \ */
              ++s;
              switch (*s)
                {
                case '"':
                case '\\':
                case '`':
                case '$':
                case '\n':
                  *dest = *s;
                  ++s;
                  ++dest;
                  break;

                default:
                  /* not an escaped char */
                  *dest = '\\';
                  ++dest;
                  /* ++s already done. */
                  break;
                }
              break;

            default:
              *dest = *s;
              ++dest;
              ++s;
              break;
            }

          /* g_assert(s > dest);*/ /* loop invariant */
        }
    }
  else
    {
      while (*s)
        {
          /* g_assert(s > dest); */ /* loop invariant */
          
          if (*s == '\'')
            {
              /* End of the string, return now */
              *dest = '\0';
              ++s;
              *end = s;
              return TRUE;
            }
          else
            {
              *dest = *s;
              ++dest;
              ++s;
            }

          /* g_assert(s > dest); */ /* loop invariant */
        }
    }
  
  /* If we reach here this means the close quote was never encountered */

  *dest = '\0';
  *end = s;
  return FALSE;
}

static sn_bool_t
unescape_string_inplace (char  *str,
                         char **end)
{
  char* dest;
  char* s;
  sn_bool_t escaped;
  
  dest = s = str;
  escaped = FALSE;
  
  while (*s)
    {
      if (escaped)
        {
          escaped = FALSE;
          
          *dest = *s;
          ++dest;
        }
      else
        {
          if (*s == ' ')
            break;
          else if (*s == '\\')
            escaped = TRUE;
          else
            {
              *dest = *s;
              ++dest;
            }
        }

      ++s;
    }

  *dest = '\0';
  *end = s;
  
  return TRUE;
}

static sn_bool_t
parse_property (const char  *str,
                char       **name_p,
                char       **val_p,
                const char **end_p)
{
  char *val;
  char *name;
  char *copy;  
  char *p;
  
  *end_p = NULL;

  copy = sn_internal_strdup (str);
  p = copy;
  
  while (*p == ' ')
    ++p;

  name = parse_prefix_up_to (p, '=', (const char**) &p);
  if (name == NULL)
    {
      sn_free (copy);
      return FALSE;
    }
  ++p; /* skip '=' */

  while (*p == ' ')
    ++p;

  if (*p == '\'' ||
      *p == '"')
    {
      char *end;

      end = NULL;
      if (!unquote_string_inplace (p, &end))
        {
          sn_free (copy);
          return FALSE;
        }

      val = sn_internal_strndup (p, end - p);

      p = end;
    }
  else
    {
      char *end;

      end = NULL;
      if (!unescape_string_inplace (p, &end))
        {
          sn_free (copy);
          return FALSE;
        }

      val = sn_internal_strndup (p, end - p);

      p = end;
    }

  while (*p == ' ')
    ++p;

  *end_p = str + (p - copy);

  sn_free (copy);

  *name_p = name;
  *val_p = val;

  return TRUE;
}

sn_bool_t
sn_internal_unserialize_message (const char *message,
                                 char      **prefix_p,
                                 char     ***property_names,
                                 char     ***property_values)
{
  /* GLib would simplify this a lot... */
  char *prefix;
  char **names;
  char **values;
  const char *p;
  char *name;
  char *value;
  
  *prefix_p = NULL;
  *property_names = NULL;
  *property_values = NULL;
  
  prefix = NULL;
  names = NULL;
  values = NULL;

  prefix = parse_prefix_up_to (message, ':', &p);
  if (prefix == NULL)
    return FALSE;

  ++p; /* skip ':' */

  name = NULL;
  value = NULL;
  while (parse_property (p, &name, &value, &p))
    {
      append_string_to_list (&names, name);
      append_string_to_list (&values, value);
    }
  
  *prefix_p = prefix;
  *property_names = names;
  *property_values = values;

  return TRUE;
}