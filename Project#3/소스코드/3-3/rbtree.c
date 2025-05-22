#include "rbtree.h"
#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "spinlock.h"

void rbtree_init(struct rb_tree **treep, void *memory) {
  struct rb_tree *tree = (struct rb_tree *)memory;
  *treep = tree;

  tree->node_max = 100;
  tree->node_count = 0;
  tree->node_next_free = 0;
  tree->head = 0;
  tree->tail = 0;

  // Set up nil node
  tree->nil = (struct rb_node *)((char *)memory + sizeof(struct rb_tree));
  memset(tree->nil, 0, sizeof(struct rb_node));
  tree->nil->color = 0; // Black
  tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;

  // Set root to nil
  tree->root = tree->nil;

  // Set up node array
  tree->nodes = (struct rb_node *)((char *)memory + sizeof(struct rb_tree) + sizeof(struct rb_node));
}

struct rb_node *rbtree_find(struct rb_tree *tree, uint key) {
  struct rb_node *x = tree->root;
  while (x != tree->nil && key != x->key) {
    if (key < x->key)
      x = x->left;
    else
      x = x->right;
  }
  return x;
}

void rbtree_left_rotate(struct rb_tree *tree, struct rb_node *x) {
  struct rb_node *y = x->right;
  x->right = y->left;
  if (y->left != tree->nil)
    y->left->parent = x;
  y->parent = x->parent;
  if (x->parent == tree->nil)
    tree->root = y;
  else if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;
  y->left = x;
  x->parent = y;
}

void rbtree_right_rotate(struct rb_tree *tree, struct rb_node *x) {
  struct rb_node *y = x->left;
  x->left = y->right;
  if (y->right != tree->nil)
    y->right->parent = x;
  y->parent = x->parent;
  if (x->parent == tree->nil)
    tree->root = y;
  else if (x == x->parent->right)
    x->parent->right = y;
  else
    x->parent->left = y;
  y->right = x;
  x->parent = y;
}

void rbtree_insert_fixup(struct rb_tree *tree, struct rb_node *z) {
  struct rb_node *y;
  while (z->parent->color == 1) {
    if (z->parent == z->parent->parent->left) {
      y = z->parent->parent->right;
      if (y->color == 1) {
        z->parent->color = 0;
        y->color = 0;
        z->parent->parent->color = 1;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rbtree_left_rotate(tree, z);
        }
        z->parent->color = 0;
        z->parent->parent->color = 1;
        rbtree_right_rotate(tree, z->parent->parent);
      }
    } else {
      y = z->parent->parent->left;
      if (y->color == 1) {
        z->parent->color = 0;
        y->color = 0;
        z->parent->parent->color = 1;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rbtree_right_rotate(tree, z);
        }
        z->parent->color = 0;
        z->parent->parent->color = 1;
        rbtree_left_rotate(tree, z->parent->parent);
      }
    }
  }
  tree->root->color = 0;
}

void rbtree_transplant(struct rb_tree *tree, struct rb_node *u, struct rb_node *v) {
  if (u->parent == tree->nil)
    tree->root = v;
  else if (u == u->parent->left)
    u->parent->left = v;
  else
    u->parent->right = v;
  v->parent = u->parent;
}

struct rb_node *rbtree_minimum(struct rb_tree *tree, struct rb_node *x) {
  while (x->left != tree->nil)
    x = x->left;
  return x;
}

void rbtree_delete_fixup(struct rb_tree *tree, struct rb_node *x) {
  struct rb_node *w;
  while (x != tree->root && x->color == 0) {
    if (x == x->parent->left) {
      w = x->parent->right;
      if (w->color == 1) {
        w->color = 0;
        x->parent->color = 1;
        rbtree_left_rotate(tree, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == 0 && w->right->color == 0) {
        w->color = 1;
        x = x->parent;
      } else {
        if (w->right->color == 0) {
          w->left->color = 0;
          w->color = 1;
          rbtree_right_rotate(tree, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = 0;
        w->right->color = 0;
        rbtree_left_rotate(tree, x->parent);
        x = tree->root;
      }
    } else {
      w = x->parent->left;
      if (w->color == 1) {
        w->color = 0;
        x->parent->color = 1;
        rbtree_right_rotate(tree, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == 0 && w->left->color == 0) {
        w->color = 1;
        x = x->parent;
      } else {
        if (w->left->color == 0) {
          w->right->color = 0;
          w->color = 1;
          rbtree_left_rotate(tree, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = 0;
        w->left->color = 0;
        rbtree_right_rotate(tree, x->parent);
        x = tree->root;
      }
    }
  }
  x->color = 0;
}

void rbtree_delete(struct rb_tree *tree, struct rb_node *z) {
  struct rb_node *y = z;
  struct rb_node *x;
  char y_original_color = y->color;

  if (z->left == tree->nil) {
    x = z->right;
    rbtree_transplant(tree, z, z->right);
  } else if (z->right == tree->nil) {
    x = z->left;
    rbtree_transplant(tree, z, z->left);
  } else {
    y = rbtree_minimum(tree, z->right);
    y_original_color = y->color;
    x = y->right;
    if (y->parent == z)
      x->parent = y;
    else {
      rbtree_transplant(tree, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    rbtree_transplant(tree, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
  }
  if (y_original_color == 0)
    rbtree_delete_fixup(tree, x);
}

void rbtree_insert(struct rb_tree *tree, uint key, uint value) {
  // First, check if the node exists
  struct rb_node *existing_node = rbtree_find(tree, key);
  if (existing_node != tree->nil) {
    // Update the value
    existing_node->value = value;
    // Move the node to the tail of the doubly linked list
    if (existing_node != tree->tail) {
      // Remove from current position
      if (existing_node->prev)
        existing_node->prev->next = existing_node->next;
      else
        tree->head = existing_node->next;
      if (existing_node->next)
        existing_node->next->prev = existing_node->prev;

      // Insert at tail
      existing_node->prev = tree->tail;
      existing_node->next = 0;
      if (tree->tail)
        tree->tail->next = existing_node;
      tree->tail = existing_node;
    }
    return;
  }

  // If node count >= node_max, delete the oldest node
  if (tree->node_count >= tree->node_max) {
    // Delete the oldest node (tree->head)
    struct rb_node *oldest = tree->head;
    // Remove from red-black tree
    rbtree_delete(tree, oldest);
    // Remove from doubly linked list
    tree->head = oldest->next;
    if (tree->head)
      tree->head->prev = 0;
    else
      tree->tail = 0; // List is now empty
    // Decrement node count
    tree->node_count--;
    // Reuse the node by resetting node_next_free
    tree->node_next_free = oldest - tree->nodes;
  }

  // Allocate a new node
  struct rb_node *z;
  if (tree->node_next_free >= tree->node_max)
    tree->node_next_free = 0; // Wrap around
  z = &tree->nodes[tree->node_next_free++];
  memset(z, 0, sizeof(struct rb_node));

  z->key = key;
  z->value = value;

  z->left = z->right = z->parent = tree->nil;

  // Standard BST insert
  struct rb_node *y = tree->nil;
  struct rb_node *x = tree->root;

  while (x != tree->nil) {
    y = x;
    if (z->key < x->key)
      x = x->left;
    else
      x = x->right;
  }

  z->parent = y;
  if (y == tree->nil)
    tree->root = z;
  else if (z->key < y->key)
    y->left = z;
  else
    y->right = z;

  // Set node color to red
  z->color = 1;

  // Insert the node at the tail of the doubly linked list
  z->prev = tree->tail;
  z->next = 0;
  if (tree->tail)
    tree->tail->next = z;
  tree->tail = z;
  if (tree->head == 0)
    tree->head = z;

  tree->node_count++;

  // Fix up the red-black tree properties
  rbtree_insert_fixup(tree, z);
}

