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


#ifndef __LF_COMMON_H__
#define __LF_COMMON_H__

#include <liblf/lf-util.h>
#include <X11/Xlib.h>

LF_BEGIN_DECLS

typedef struct LfDisplay LfDisplay;

typedef enum
{
  LF_LAUNCH_TYPE_OTHER,
  LF_LAUNCH_TYPE_DOCK_ICON,
  LF_LAUNCH_TYPE_DESKTOP_ICON,
  LF_LAUNCH_TYPE_MENU,
  LF_LAUNCH_TYPE_KEY_SHORTCUT

} LfLaunchType;

typedef void (* LfDisplayErrorTrapPush) (LfDisplay *display,
                                         Display   *xdisplay);
typedef void (* LfDisplayErrorTrapPop)  (LfDisplay *display,
                                         Display   *xdisplay);

LfDisplay* lf_display_new             (Display                *xdisplay,
                                       LfDisplayErrorTrapPush  push_trap_func,
                                       LfDisplayErrorTrapPop   pop_trap_func);
void       lf_display_ref             (LfDisplay              *display);
void       lf_display_unref           (LfDisplay              *display);
Display*   lf_display_get_x_display   (LfDisplay              *display);
Screen*    lf_display_get_x_screen    (LfDisplay              *display,
                                       int                     number);
lf_bool_t  lf_display_process_event   (LfDisplay              *display,
                                       XEvent                 *xevent);
void       lf_display_error_trap_push (LfDisplay              *display);
void       lf_display_error_trap_pop  (LfDisplay              *display);



LF_END_DECLS

#endif /* __LF_COMMON_H__ */
