/* List abstraction used internally */
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


#ifndef __LF_LIST_H__
#define __LF_LIST_H__

#include <liblf/lf-util.h>

LF_BEGIN_DECLS

/* FIXME use lf_internal prefix for all this */

typedef struct LfList LfList;

typedef lf_bool_t (* LfListForeachFunc) (void *value, void *data);

LfList* lf_list_new     (void);
void    lf_list_free    (LfList            *list);
void    lf_list_prepend (LfList            *list,
                         void              *data);
void    lf_list_append  (LfList            *list,
                         void              *data);
void    lf_list_remove  (LfList            *list,
                         void              *data);
void    lf_list_foreach (LfList            *list,
                         LfListForeachFunc  func,
                         void              *data);
lf_bool_t lf_list_empty (LfList            *list);

LF_END_DECLS

#endif /* __LF_LIST_H__ */
