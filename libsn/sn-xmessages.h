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

#ifndef __LF_XMESSAGES_H__
#define __LF_XMESSAGES_H__

#include <liblf/lf-common.h>

LF_BEGIN_DECLS

typedef void (* LfXmessageFunc) (LfDisplay       *display,
                                 const char      *message_type,
                                 const char      *message,
                                 void            *user_data);

void lf_internal_add_xmessage_func    (LfDisplay      *display,
                                       const char     *message_type,
                                       LfXmessageFunc  func,
                                       void           *func_data,
                                       LfFreeFunc      free_data_func);
void lf_internal_remove_xmessage_func (LfDisplay      *display,
                                       const char     *message_type,
                                       LfXmessageFunc  func,
                                       void           *func_data);
void lf_internal_broadcast_xmessage   (LfDisplay      *display,
                                       const char     *message_type,
                                       const char     *message);

char*     lf_internal_serialize_message   (const char   *prefix,
                                           const char  **property_names,
                                           const char  **property_values);
lf_bool_t lf_internal_unserialize_message (const char   *message,
                                           char        **prefix,
                                           char       ***property_names,
                                           char       ***property_values);

LF_END_DECLS

#endif /* __LF_XMESSAGES_H__ */
