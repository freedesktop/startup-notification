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

#include "lf-internals.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

Atom
lf_internal_atom_get (LfDisplay  *display,
                      const char *atom_name)
{
  return XInternAtom (lf_display_get_x_display (display),
                      atom_name,
                      False);
}

void
lf_internal_set_utf8_string (LfDisplay  *display,
                             Window      xwindow,
                             const char *property,
                             const char *str)
{
  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   lf_internal_atom_get (display, "UTF8_STRING"),
                   8, PropModeReplace, (unsigned char*) str,
                   strlen (str));

  lf_display_error_trap_pop (display);
}

void
lf_internal_set_string (LfDisplay  *display,
                        Window      xwindow,
                        const char *property,
                        const char *str)
{
  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   XA_STRING,
                   8, PropModeReplace, (unsigned char*) str,
                   strlen (str));

  lf_display_error_trap_pop (display);
}

void
lf_internal_set_cardinal (LfDisplay  *display,
                          Window      xwindow,
                          const char *property,
                          int         val)
{
  unsigned long long_val = val;

  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   XA_CARDINAL,
                   32, PropModeReplace, (unsigned char*) &long_val, 1);

  lf_display_error_trap_pop (display);
}

void
lf_internal_set_window (LfDisplay  *display,
                        Window      xwindow,
                        const char *property,
                        Window      val)
{
  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   XA_WINDOW,
                   32, PropModeReplace, (unsigned char*) &val, 1);

  lf_display_error_trap_pop (display);
}

void
lf_internal_set_cardinal_list (LfDisplay  *display,
                               Window      xwindow,
                               const char *property,
                               int        *vals,
                               int         n_vals)
{
  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   XA_CARDINAL,
                   32, PropModeReplace, (unsigned char*) vals, n_vals);

  lf_display_error_trap_pop (display);
}

void
lf_internal_set_atom_list (LfDisplay  *display,
                           Window      xwindow,
                           const char *property,
                           Atom       *vals,
                           int         n_vals)
{
  lf_display_error_trap_push (display);

  XChangeProperty (lf_display_get_x_display (display),
                   xwindow,
                   lf_internal_atom_get (display, property),
                   XA_ATOM,
                   32, PropModeReplace, (unsigned char*) vals, n_vals);

  lf_display_error_trap_pop (display);
}

lf_bool_t
lf_internal_get_cardinal (LfDisplay  *display,
                          Window      xwindow,
                          const char *property,
                          int        *val)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned long *num;
  int result;

  *val = 0;

  lf_display_error_trap_push (display);
  type = None;
  num = NULL;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 256, /* 256 = random number */
			       False, XA_CARDINAL, &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&num);
  lf_display_error_trap_pop (display);
  if (result != Success || num == NULL || nitems == 0)
    {
      if (num)
        XFree (num);
      return FALSE;
    }

  if (type != XA_CARDINAL)
    {
      XFree (num);
      return FALSE;
    }

  *val = *num;
  XFree (num);

  return TRUE;
}

lf_bool_t
lf_internal_get_utf8_string (LfDisplay   *display,
                             Window       xwindow,
                             const char  *property,
                             char       **val)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  char *str;
  int result;
  Atom utf8_string;

  utf8_string = lf_internal_atom_get (display, "UTF8_STRING");

  *val = NULL;

  lf_display_error_trap_push (display);
  type = None;
  str = NULL;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 20000, /* 20000 = random number */
			       False,
                               utf8_string,
                               &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&str);
  lf_display_error_trap_pop (display);
  if (result != Success || str == NULL)
    {
      if (str)
        XFree (str);
      return FALSE;
    }

  if (type != utf8_string ||
      format != 8 ||
      nitems == 0)
    {
      XFree (str);
      return FALSE;
    }

  if (!lf_internal_utf8_validate (str, nitems))
    {
      fprintf (stderr, "Warning: invalid UTF-8 in property %s on window 0x%lx\n",
               property, xwindow);
      XFree (str);
      return FALSE;
    }
  
  *val = lf_internal_strdup (str);
  XFree (str);

  return TRUE;
}

lf_bool_t
lf_internal_get_string (LfDisplay   *display,
                        Window       xwindow,
                        const char  *property,
                        char       **val)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  char *str;
  int result;

  *val = NULL;

  lf_display_error_trap_push (display);
  type = None;
  str = NULL;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 20000, /* 20000 = random number */
			       False,
                               XA_STRING,
                               &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&str);
  lf_display_error_trap_pop (display);
  if (result != Success || str == NULL)
    {
      if (str)
        XFree (str);
      return FALSE;
    }

  if (type != XA_STRING ||
      format != 8 ||
      nitems == 0)
    {
      XFree (str);
      return FALSE;
    }
  
  *val = lf_internal_strdup (str);
  XFree (str);

  return TRUE;
}

lf_bool_t
lf_internal_get_window (LfDisplay   *display,
                        Window       xwindow,
                        const char  *property,
                        Window      *val)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  Window *win;
  int result;

  *val = 0;

  lf_display_error_trap_push (display);
  type = None;
  win = NULL;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 256, /* 256 = random number */
			       False, XA_WINDOW, &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&win);
  lf_display_error_trap_pop (display);
  if (result != Success || win == NULL || nitems == 0)
    {
      if (win)
        XFree (win);
      return FALSE;
    }

  if (type != XA_WINDOW)
    {
      XFree (win);
      return FALSE;
    }

  *val = *win;
  XFree (win);

  return TRUE;
}

lf_bool_t
lf_internal_get_atom_list (LfDisplay   *display,
                           Window       xwindow,
                           const char  *property,
                           Atom       **atoms,
                           int         *n_atoms)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  Atom *data;
  int result;

  *atoms = NULL;
  *n_atoms = 0;
  data = NULL;
  
  lf_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 1000, /* 1000 = random number */
			       False, XA_ATOM, &type, &format, &nitems,
                               &bytes_after, (unsigned char **)&data);  
  lf_display_error_trap_pop (display);
  if (result != Success || data == NULL)
    {
      if (data)
        XFree (data);
      return FALSE;
    }
  
  if (type != XA_ATOM)
    {
      XFree (data);
      return FALSE;
    }

  *atoms = lf_new (Atom, nitems);
  memcpy (*atoms, data, sizeof (Atom) * nitems);
  *n_atoms = nitems;
  
  XFree (data);

  return TRUE;
}

lf_bool_t
lf_internal_get_cardinal_list (LfDisplay   *display,
                               Window       xwindow,
                               const char  *property,
                               int        **vals,
                               int         *n_vals)
{
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned long *nums;
  int result;
  int i;
  
  *vals = NULL;
  *n_vals = 0;
  nums = NULL;
  
  lf_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (lf_display_get_x_display (display),
			       xwindow,
                               lf_internal_atom_get (display, property),
			       0, 1000, /* 1000 = random number */
			       False, XA_CARDINAL, &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&nums);  
  lf_display_error_trap_pop (display);
  if (result != Success || nums == NULL)
    {
      if (nums)
        XFree (nums);
      return FALSE;
    }
  
  if (type != XA_CARDINAL)
    {
      XFree (nums);
      return FALSE;
    }

  *vals = lf_new (int, nitems);
  *n_vals = nitems;
  i = 0;
  while (i < *n_vals)
    {
      (*vals)[i] = nums[i];
      ++i;
    }
  
  XFree (nums);

  return TRUE;
}

void
lf_internal_send_event_all_screens (LfDisplay    *display,
                                    unsigned long mask,
                                    XEvent       *xevent)
{
  int i;
  Display *xdisplay;

  xdisplay = lf_display_get_x_display (display);
  
  i = 0;
  while (lf_display_get_x_screen (display, i) != NULL)
    {
      XSendEvent (xdisplay,
                  RootWindow (xdisplay, i),
                  False,
                  mask,
                  xevent);

      ++i;
    }
}
