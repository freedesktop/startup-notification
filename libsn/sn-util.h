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


#ifndef __LF_UTIL_H__
#define __LF_UTIL_H__

/* Guard C code in headers, while including them from C++ */
#ifdef  __cplusplus
# define LF_BEGIN_DECLS  extern "C" {
# define LF_END_DECLS    }
#else
# define LF_BEGIN_DECLS
# define LF_END_DECLS
#endif

LF_BEGIN_DECLS

typedef unsigned long lf_size_t;
typedef int           lf_bool_t;

/* Padding in vtables */
typedef void (* LfPaddingFunc) (void);
/* Free data */
typedef void (* LfFreeFunc)    (void *data);

void* lf_malloc      (lf_size_t  n_bytes);
void* lf_malloc0     (lf_size_t  n_bytes);
void* lf_realloc     (void      *mem,
                      lf_size_t  n_bytes);
void  lf_free        (void      *mem);
void* lf_try_malloc  (lf_size_t  n_bytes);
void* lf_try_realloc (void      *mem,
                      lf_size_t  n_bytes);

/* Convenience memory allocators
 */
#define lf_new(struct_type, n_structs)		\
    ((struct_type *) lf_malloc (((lf_size_t) sizeof (struct_type)) * ((lf_size_t) (n_structs))))
#define lf_new0(struct_type, n_structs)		\
    ((struct_type *) lf_malloc0 (((lf_size_t) sizeof (struct_type)) * ((lf_size_t) (n_structs))))
#define lf_renew(struct_type, mem, n_structs)	\
    ((struct_type *) lf_realloc ((mem), ((lf_size_t) sizeof (struct_type)) * ((lf_size_t) (n_structs))))


/* Memory allocation virtualization, so you can override memory
 * allocation behavior.  lf_mem_set_vtable() has to be the very first
 * liblf function called if being used
 */
typedef struct
{
  void* (*malloc)      (lf_size_t    n_bytes);
  void* (*realloc)     (void        *mem,
                        lf_size_t    n_bytes);
  void  (*free)        (void        *mem);
  /* optional */
  void* (*calloc)      (lf_size_t    n_blocks,
                        lf_size_t    n_block_bytes);
  void* (*try_malloc)  (lf_size_t    n_bytes);
  void* (*try_realloc) (void        *mem,
                        lf_size_t    n_bytes);
  LfPaddingFunc        padding1;
  LfPaddingFunc        padding2;
} LfMemVTable;

void      lf_mem_set_vtable       (LfMemVTable *vtable);
lf_bool_t lf_mem_is_system_malloc (void);

typedef lf_bool_t (* LfUtf8ValidateFunc) (const char *str,
                                          int         max_len);

void lf_set_utf8_validator (LfUtf8ValidateFunc validate_func);

LF_END_DECLS

#endif /* __LF_UTIL_H__ */
