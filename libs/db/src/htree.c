/***************************************************************************
 *   File: htree.c                                                         *
 *  Usage: Generalized hash tree code for fast lookups                     *
 *                                                                         *
 * This code is released under the CircleMud License                       *
 * Written by Elie Rosenblum <fnord@cosanostra.net>                        *
 * Copyright (c) 7-Oct-2004                                                *
 ***************************************************************************/

#include "dbat/db/htree.h"
#include <stdlib.h>
#include <string.h>

#undef HTREE_TEST_CYCLES

struct htree_node *HTREE_NULL = NULL;
int htree_total_nodes = 0;
int htree_depth_used = 0;

void htree_shutdown()
{
  free(HTREE_NULL);
  HTREE_NULL = NULL;
}

struct htree_node *htree_init()
{
  struct htree_node *newnode;
  int i;

  if (! HTREE_NULL) {
    htree_total_nodes++;
    HTREE_NULL = (struct htree_node *)calloc(1, sizeof(struct htree_node));
    for (i = 0; i < HTREE_NODE_SUBS; i++) {
      HTREE_NULL->subs[i] = HTREE_NULL;
    }
    HTREE_NULL->content = NOWHERE;
    HTREE_NULL->parent = NULL;
  }

  if (! htree_depth_used)
    htree_depth_used = 1;

  htree_total_nodes++;
  newnode = (struct htree_node *)calloc(1, sizeof(struct htree_node));
  memcpy(newnode->subs, HTREE_NULL->subs, HTREE_NODE_SUBS * sizeof(struct htree_node *));
  newnode->content = NOWHERE;
  newnode->parent = HTREE_NULL;

  return newnode;
}

void htree_free(struct htree_node *root)
{
  int i;

  if (! root || root == HTREE_NULL)
    return;

  for (i = 0; i < HTREE_NODE_SUBS; i++)
    htree_free(root->subs[i]);

  free(root);
}

void htree_add(struct htree_node *root, IDXTYPE htindex, IDXTYPE content)
{
  struct htree_node *tmp;
  int i, depth;

  if (! root)
    return;

  tmp = root;
  depth = 0;
  while (htindex) {
    depth++;
    i = htindex & HTREE_NODE_MASK;
    htindex >>= HTREE_NODE_BITS;
    if (tmp->subs[i] == HTREE_NULL) {
      htree_total_nodes++;
      tmp->subs[i] = (struct htree_node *)calloc(1, sizeof(struct htree_node));
      memcpy(tmp->subs[i]->subs, HTREE_NULL->subs, HTREE_NODE_SUBS * sizeof(struct htree_node *));
      tmp->subs[i]->content = NOWHERE;
      tmp->subs[i]->parent = HTREE_NULL;
    }
    tmp = tmp->subs[i];
  }

  if (tmp == HTREE_NULL) /* We fell off somehow! Time to crap our pants */
    return;

  if (depth > htree_depth_used)
    htree_depth_used = depth;

  tmp->content = content;
}

struct htree_node *htree_find_node(struct htree_node *root, IDXTYPE htindex)
{
  struct htree_node *tmp;
  int i;
  
  tmp = root;
  while (htindex) {
    i = htindex & HTREE_NODE_MASK;
    htindex >>= HTREE_NODE_BITS;
    tmp = tmp->subs[i];
  }

  return tmp;
}

void htree_del(struct htree_node *root, IDXTYPE htindex)
{
  struct htree_node *tmp;

  tmp = htree_find_node(root, htindex);
  tmp->content = NOWHERE;
}

IDXTYPE htree_find(struct htree_node *root, IDXTYPE htindex)
{
  struct htree_node *tmp;

  tmp = htree_find_node(root, htindex);
  return tmp->content;
}