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

#ifndef __SN_XUTILS_H__
#define __SN_XUTILS_H__

#include <libsn/sn-common.h>

SN_BEGIN_DECLS

Atom sn_internal_atom_get        (SnDisplay  *display,
                                  const char *atom_name);
void sn_internal_set_utf8_string (SnDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  const char *str);
void sn_internal_set_string      (SnDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  const char *str);
void sn_internal_set_cardinal    (SnDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  int         val);
void sn_internal_set_window        (SnDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    Window      val);
void sn_internal_set_cardinal_list (SnDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    int        *vals,
                                    int         n_vals);
void sn_internal_set_atom_list     (SnDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    Atom       *vals,
                                    int         n_vals);


sn_bool_t sn_internal_get_utf8_string   (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         char       **val);
sn_bool_t sn_internal_get_string        (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         char       **val);
sn_bool_t sn_internal_get_cardinal      (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         int         *val);
sn_bool_t sn_internal_get_window        (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         Window      *val);
sn_bool_t sn_internal_get_atom_list     (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         Atom       **atoms,
                                         int         *n_atoms);
sn_bool_t sn_internal_get_cardinal_list (SnDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         int        **vals,
                                         int         *n_vals);

void sn_internal_send_event_all_screens (SnDisplay    *display,
                                         unsigned long mask,
                                         XEvent       *xevent);


SN_END_DECLS

#endif /* __SN_XUTILS_H__ */
