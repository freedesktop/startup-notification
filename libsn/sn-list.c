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

#include "lf-list.h"
#include "lf-internals.h"

typedef struct LfListNode
{
  void *data;
  struct LfListNode *next;
} LfListNode;

struct LfList
{
  LfListNode *head;
};

static LfListNode*
lf_list_node_alloc (void)
{
  return lf_new0 (LfListNode, 1);
}

LfList*
lf_list_new (void)
{
  LfList *list;

  list = lf_new (LfList, 1);
  list->head = NULL;

  return list;
}

void
lf_list_free (LfList *list)
{
  LfListNode *node;

  node = list->head;
  while (node != NULL)
    {
      LfListNode *next = node->next;

      lf_free (node);

      node = next;
    }

  lf_free (list);
}

void
lf_list_prepend (LfList *list,
                 void   *data)
{
  if (list->head == NULL)
    {
      list->head = lf_list_node_alloc ();
      list->head->data = data;
    }
  else
    {
      LfListNode *node;

      node = lf_list_node_alloc ();
      node->data = data;
      node->next = list->head;
      list->head = node;
    }
}

void
lf_list_append (LfList *list,
                void   *data)
{
  if (list->head == NULL)
    {
      list->head = lf_list_node_alloc ();
      list->head->data = data;
    }
  else
    {
      LfListNode *node;
      
      node = list->head;
      while (node->next != NULL)
        node = node->next;
      
      node->next = lf_list_node_alloc ();
      node->next->data = data;
    }
}

void
lf_list_remove (LfList *list,
                void   *data)
{
  LfListNode *node;
  LfListNode *prev;

  prev = NULL;
  node = list->head;
  while (node != NULL)
    {
      if (node->data == data)
        {
          if (prev)
            prev->next = node->next;
          else
            list->head = node->next;

          lf_free (node);

          return;
        }

      prev = node;
      node = node->next;
    }
}

void
lf_list_foreach (LfList            *list,
                 LfListForeachFunc  func,
                 void              *data)
{
  LfListNode *node;

  node = list->head;
  while (node != NULL)
    {
      LfListNode *next = node->next; /* reentrancy safety */
      
      if (!(* func) (node->data, data))
        return;
      
      node = next;
    }
}

lf_bool_t
lf_list_empty (LfList *list)
{
  return list->head == NULL;
}
