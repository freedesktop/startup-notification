/* This code derived from GLib, Copyright (C) 1997-2002
 * by the GLib team.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "lf-util.h"
#include "lf-internals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef	REALLOC_0_WORKS
static void*
standard_realloc (void* mem,
		  lf_size_t    n_bytes)
{
  if (!mem)
    return malloc (n_bytes);
  else
    return realloc (mem, n_bytes);
}
#endif	/* !REALLOC_0_WORKS */

#ifdef SANE_MALLOC_PROTOS
#  define standard_malloc	malloc
#  ifdef REALLOC_0_WORKS
#    define standard_realloc	realloc
#  endif /* REALLOC_0_WORKS */
#  define standard_free		free
#  define standard_calloc	calloc
#  define standard_try_malloc	malloc
#  define standard_try_realloc	realloc
#else	/* !SANE_MALLOC_PROTOS */
static void*
standard_malloc (lf_size_t n_bytes)
{
  return malloc (n_bytes);
}
#  ifdef REALLOC_0_WORKS
static void*
standard_realloc (void* mem,
		  lf_size_t    n_bytes)
{
  return realloc (mem, n_bytes);
}
#  endif /* REALLOC_0_WORKS */
static void
standard_free (void* mem)
{
  free (mem);
}
static void*
standard_calloc (lf_size_t n_blocks,
		 lf_size_t n_bytes)
{
  return calloc (n_blocks, n_bytes);
}
#define	standard_try_malloc	standard_malloc
#define	standard_try_realloc	standard_realloc
#endif	/* !SANE_MALLOC_PROTOS */


/* --- variables --- */
static LfMemVTable lf_mem_vtable = {
  standard_malloc,
  standard_realloc,
  standard_free,
  standard_calloc,
  standard_try_malloc,
  standard_try_realloc,
  NULL,
  NULL
};


/* --- functions --- */
void*
lf_malloc (lf_size_t n_bytes)
{
  if (n_bytes)
    {
      void* mem;

      mem = lf_mem_vtable.malloc (n_bytes);
      if (mem)
	return mem;

      fprintf (stderr,
               "liblf: failed to allocate %lu bytes",
               (unsigned long) n_bytes);
    }

  return NULL;
}

void*
lf_malloc0 (lf_size_t n_bytes)
{
  if (n_bytes)
    {
      void* mem;

      mem = lf_mem_vtable.calloc (1, n_bytes);
      if (mem)
	return mem;

      fprintf (stderr,
               "liblf: failed to allocate %lu bytes",
               (unsigned long) n_bytes);
    }

  return NULL;
}

void*
lf_realloc (void     *mem,
            lf_size_t n_bytes)
{
  if (n_bytes)
    {
      mem = lf_mem_vtable.realloc (mem, n_bytes);
      if (mem)
	return mem;

      fprintf (stderr,
               "liblf: failed to allocate %lu bytes",
               (unsigned long) n_bytes);
    }

  if (mem)
    lf_mem_vtable.free (mem);

  return NULL;
}

void
lf_free (void* mem)
{
  if (mem)
    lf_mem_vtable.free (mem);
}

void*
lf_try_malloc (lf_size_t n_bytes)
{
  if (n_bytes)
    return lf_mem_vtable.try_malloc (n_bytes);
  else
    return NULL;
}

void*
lf_try_realloc (void       *mem,
                lf_size_t   n_bytes)
{
  if (n_bytes)
    return lf_mem_vtable.try_realloc (mem, n_bytes);

  if (mem)
    lf_mem_vtable.free (mem);

  return NULL;
}

static void*
fallback_calloc (lf_size_t n_blocks,
		 lf_size_t n_block_bytes)
{
  lf_size_t l = n_blocks * n_block_bytes;
  void* mem = lf_mem_vtable.malloc (l);

  if (mem)
    memset (mem, 0, l);

  return mem;
}

static lf_bool_t vtable_set = FALSE;

lf_bool_t
lf_mem_is_system_malloc (void)
{
  return !vtable_set;
}

void
lf_mem_set_vtable (LfMemVTable *vtable)
{
  if (!vtable_set)
    {
      vtable_set = TRUE;
      if (vtable->malloc && vtable->realloc && vtable->free)
	{
	  lf_mem_vtable.malloc = vtable->malloc;
	  lf_mem_vtable.realloc = vtable->realloc;
	  lf_mem_vtable.free = vtable->free;
	  lf_mem_vtable.calloc = vtable->calloc ? vtable->calloc : fallback_calloc;
	  lf_mem_vtable.try_malloc = vtable->try_malloc ? vtable->try_malloc : lf_mem_vtable.malloc;
	  lf_mem_vtable.try_realloc = vtable->try_realloc ? vtable->try_realloc : lf_mem_vtable.realloc;
	}
      else
        {
          fprintf (stderr,
                   "liblf: memory allocation vtable lacks one of malloc(), realloc() or free()");
        }
    }
  else
    {
      fprintf (stderr,
               "liblf: memory allocation vtable can only be set once at startup");
    }
}

static LfUtf8ValidateFunc utf8_validator = NULL;

void
lf_set_utf8_validator (LfUtf8ValidateFunc validate_func)
{
  utf8_validator = validate_func;
}

lf_bool_t
lf_internal_utf8_validate (const char *str,
                           int         max_len)
{
  if (utf8_validator)
    {
      if (max_len < 0)
        max_len = strlen (str);
      return (* utf8_validator) (str, max_len);
    }
  else
    return TRUE;
}

char*
lf_internal_strdup (const char *str)
{
  char *s;

  s = lf_malloc (strlen (str) + 1);
  strcpy (s, str);

  return s;
}

char*
lf_internal_strndup (const char *str,
                     int         n)
{
  char *new_str;
  
  if (str)
    {
      new_str = lf_new (char, n + 1);
      strncpy (new_str, str, n);
      new_str[n] = '\0';
    }
  else
    new_str = NULL;

  return new_str;
}

void
lf_internal_strfreev (char **strings)
{
  int i;

  if (strings == NULL)
    return;
  
  i = 0;
  while (strings[i])
    {
      lf_free (strings[i]);
      ++i;
    }
  lf_free (strings);
}

unsigned long
lf_internal_string_to_ulong (const char* str)
{
  unsigned long retval;
  char *end;
  errno = 0;
  retval = strtoul (str, &end, 0);
  if (end == str || errno != 0)
    retval = 0;

  return retval;
}
