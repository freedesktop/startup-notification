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

#ifndef __LF_XUTILS_H__
#define __LF_XUTILS_H__

#include <liblf/lf-common.h>

LF_BEGIN_DECLS

Atom lf_internal_atom_get        (LfDisplay  *display,
                                  const char *atom_name);
void lf_internal_set_utf8_string (LfDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  const char *str);
void lf_internal_set_string      (LfDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  const char *str);
void lf_internal_set_cardinal    (LfDisplay  *display,
                                  Window      xwindow,
                                  const char *property,
                                  int         val);
void lf_internal_set_window        (LfDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    Window      val);
void lf_internal_set_cardinal_list (LfDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    int        *vals,
                                    int         n_vals);
void lf_internal_set_atom_list     (LfDisplay  *display,
                                    Window      xwindow,
                                    const char *property,
                                    Atom       *vals,
                                    int         n_vals);


lf_bool_t lf_internal_get_utf8_string   (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         char       **val);
lf_bool_t lf_internal_get_string        (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         char       **val);
lf_bool_t lf_internal_get_cardinal      (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         int         *val);
lf_bool_t lf_internal_get_window        (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         Window      *val);
lf_bool_t lf_internal_get_atom_list     (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         Atom       **atoms,
                                         int         *n_atoms);
lf_bool_t lf_internal_get_cardinal_list (LfDisplay   *display,
                                         Window       xwindow,
                                         const char  *property,
                                         int        **vals,
                                         int         *n_vals);

void lf_internal_send_event_all_screens (LfDisplay    *display,
                                         unsigned long mask,
                                         XEvent       *xevent);


LF_END_DECLS

#endif /* __LF_XUTILS_H__ */
