#ifndef _RBTREE_H_
#define _RBTREE_H_

#include "types.h"

typedef unsigned int uint;

struct rb_node {
  uint key;
  uint value;
  char color; // 0 for black, 1 for red
  struct rb_node *left;
  struct rb_node *right;
  struct rb_node *parent;
  struct rb_node *next; // For doubly linked list (insertion order)
  struct rb_node *prev;
};

struct rb_tree {
  struct rb_node *root;
  struct rb_node *nil;
  int node_count;
  int node_max;
  struct rb_node *nodes; // Pointer to the node array
  int node_next_free;    // Index of next free node
  struct rb_node *head;  // Oldest node
  struct rb_node *tail;  // Most recently accessed node
};

void rbtree_init(struct rb_tree **treep, void *memory);
struct rb_node *rbtree_find(struct rb_tree *tree, uint key);
void rbtree_insert(struct rb_tree *tree, uint key, uint value);
void rbtree_delete(struct rb_tree *tree, struct rb_node *z);

#endif // _RBTREE_H_

