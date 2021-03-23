/* Copyright (c) 2006, Jim Tsillas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Jim Tsillas ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Jim Tsillas BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GENAVL_H
#define GENAVL_H

#define USE_OFFSET_PTR
#if defined(USE_OFFSET_PTR)
#include "offset_ptr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GENAVL_STACK 64

/***************************************************************
 *
 * The GenAVL is a flexible base class which
 * implements a balancing AVL tree as an
 * abstract class. The two classes used to
 * implement GenAVL are the GenAVLEntry and
 * the GenAVLTree. The GenAVLEntry is mostly pure and
 * serves as a specification for the format of
 * any entry in the GenAVL.
 *
 * The balance integer is stored as a signed char value in
 * all GenAVLEntry objects.
 *
 * Note that the given implementation does not track the number
 * of entries or any other statistics. This is left to derived
 * classes.
 *
 ***************************************************************/
typedef struct GENAVLENTRY {
#if defined(USE_OFFSET_PTR)
  offset_ptr<void> data;
  int balance;
  offset_ptr<struct GENAVLENTRY> right;
  offset_ptr<struct GENAVLENTRY> left;
#else
  void* data;
  struct GENAVLENTRY* right;
  struct GENAVLENTRY* left;
#endif
} GenAVLEntry;

/***************************************************************
 *
 * The GenAVLTree is a friend of the GenAVLEntry class. It can
 * manipulate GenAVLEntry objects through the GenAVLEntry
 * specification without knowing anything about the underlying
 * object format. The following functions
 * must be defined for the specification:
 *
 *   int Compare(void *key) const - returns an integer
 *   which is less that zero if the given entry is less
 *   than the referenced key, greater than zero if
 *   the entry is greater than the key and zero if they
 *   are equal.
 *
 *   void *Key() const - returns a pointer to the entry
 *   key data.
 *
 *   int KeyIncrement(void*) - increments the given key to
 *   the next value. This method is used by the NextFreeKey
 *   and does not otherwise need to be implemented. Returns
 *   1 if the value wrapped across the increment, else 0.
 *   When wrapping the value, it should wrap to one less than
 *   the smallest legal value in the range. For example, if
 *   the smallest legal value is 1, the wrap should set the
 *   key to zero.
 *
 *   int KeyCompare(void*, void*) - compares two keys. This
 *   method is used by the NextFreeKey and does not otherwise
 *   need to be implemented
 *
 * Note that the given implementation does not track the number
 * of entries or any other statistics. This is left to derived
 * classes.
 *
 ***************************************************************/
typedef struct GENAVLTREE {
  offset_ptr<GenAVLEntry> root;
  int (*Compare)(GenAVLEntry*, const void*);
  void* (*Key)(GenAVLEntry*);
  int (*KeyIncrement)(void*);
  int (*KeyCompare)(const void*, const void*);
} GenAVLTree;

void GenAVLInit(GenAVLEntry*, void*);
int GenAVLTreeAdd(GenAVLTree*, GenAVLEntry*);
int GenAVLTreeAddUnbal(GenAVLTree* gatp, GenAVLEntry* gae);
void* GenAVLTreeDelete(GenAVLTree*, const void*);
GenAVLEntry* GenAVLTreeFirst(GenAVLTree*);
GenAVLEntry* GenAVLTreeNext(GenAVLTree*, const void*);
GenAVLEntry* GenAVLTreeEqualNext(GenAVLTree*, const void*);
GenAVLEntry* GenAVLTreePrev(GenAVLTree*, const void*);
GenAVLEntry* GenAVLTreeEqualPrev(GenAVLTree*, const void*);
GenAVLEntry* GenAVLTreeFind(GenAVLTree*, const void*);
void* GenAVLTreeFirstData(GenAVLTree*);
void* GenAVLTreeNextData(GenAVLTree*, const void*);
void* GenAVLTreeEqualNextData(GenAVLTree*, const void*);
void* GenAVLTreePrevData(GenAVLTree*, const void*);
void* GenAVLTreeEqualPrevData(GenAVLTree*, const void*);
void* GenAVLTreeFindData(GenAVLTree*, const void*);
int GenAVLTreeNextFreeKey(GenAVLTree*, const void*, void*);

/***************************************************************
 *
 * This structure can be used by iterators to contain iterator
 * state
 *
 ***************************************************************/
struct GENAVLSTACK {
  GenAVLEntry* stack[MAX_GENAVL_STACK];
  int sp;
};

/***************************************************************
 *
 * GenAVLBFIter is a leaf-first iterator which operates over
 * GenAVLTree databases. When performing an leaf-first iteration
 * over the tree, first call GenAVLLFIterInitData to begin the
 * traversal and then call GenAVLLFIterNextData for each
 * node. Note that a leaf-first iteration will unlink each
 * element from the tree as it is accessed. Leaf-first is useful
 * for quickly deleting a tree without rebalancing.
 * These functions return 0 if the traversal is done or
 * the data pointer if the next is available. For example:
 *
 * void walk_tree(GenAVLTree *t) {
 *   GenAVLLFIter galfi;
 *   MyData *data;
 *
 *   for (data = GenAVLLFIterInitData(&galfi, t); data != 0;
 *        data = GenAVLLFIterNextData(&galfi, t)) {
 *     DoSomething(data);
 *   }
 *
 * Note that there is no protection against additions and
 * deletions to the tree while an iterator is operating on it
 *
 ***************************************************************/
typedef struct GENAVLSTACK GenAVLLFIter;
void* GenAVLLFIterInitData(GenAVLLFIter*, GenAVLTree*);
void* GenAVLLFIterNextData(GenAVLLFIter*, GenAVLTree*);
void GenAVLLLFIterReplace(GenAVLTree*, GenAVLEntry*);

/***************************************************************
 *
 * GenAVLDFIter is a depth-first iterator which operates over
 * GenAVLTree databases. When performing an in-order iteration
 * over the tree, first call GenAVLDFIterInitData to begin the
 * traversal and then call GenAVLDFIterNextData for each
 * node. These functions return 0 if the traversal is done or
 * the data pointer if the next is available. For example:
 *
 * void walk_tree(GenAVLTree *t) {
 *   GenAVLDFIter gadfi;
 *   MyData *data;
 *
 *   for (data = GenAVLDFIterInitData(&gadfi, t); data != 0;
 *        data = GenAVLDFIterNextData(&gadfi)) {
 *     DoSomething(data);
 *   }
 *
 * The GenAVLDFIterInitNextData function is similar to
 * GenAVLDFIterInitData with the additional key parameter. This
 * parameters serves as the index immediately less-than the
 * first element to be returned. For instance, an iteration
 * over elements starting at "greater-than" 10 and
 * ending at "less-than" 20 would be done with:
 *
 * void walk_part_of_tree(GenAVLTree *t) {
 *   GenAVLDFIter gadfi;
 *   int start = 10;
 *   MyData *data;
 *
 *   for (data = GenAVLDFIterInitNextData(&gadfi, t, &start);
 *        data != 0 && data->key < 20;
 *        data = GenAVLDFIterNextData(&gadfi)) {
 *     DoSomething(data);
 *   }
 *
 * Similarly, "greater-than-or-equal" can be done by using
 * the GenAVLDFIterInitNextEqualData initializer.
 *
 * Note that there is no protection against additions and
 * deletions to the tree while an iterator is operating on it
 *
 ***************************************************************/
typedef struct GENAVLSTACK GenAVLDFIter;
void* GenAVLDFIterInitData(GenAVLDFIter*, GenAVLTree*);
void* GenAVLDFIterInitNextEqualData(GenAVLDFIter*, GenAVLTree*, const void*);
void* GenAVLDFIterInitNextData(GenAVLDFIter*, GenAVLTree*, const void*);
void* GenAVLDFIterNextData(GenAVLDFIter*);

/***************************************************************
 *
 * GenAVLBFIter is a breadth-first iterator which operates over
 * GenAVLTree databases. When performing an breadth-first iteration
 * over the tree, first call GenAVLBFIterInitData to begin the
 * traversal and then call GenAVLBFIterNextData for each
 * node. These functions return 0 if the traversal is done or
 * the data pointer if the next is available. For example:
 *
 * void walk_tree(GenAVLTree *t) {
 *   GenAVLBFIter gabfi;
 *   MyData *data;
 *
 *   for (data = GenAVLBFIterInitData(&gabfi, t); data != 0;
 *        data = GenAVLBFIterNextData(&gabfi)) {
 *     DoSomething(data);
 *   }
 *
 * Note that there is no protection against additions and
 * deletions to the tree while an iterator is operating on it
 *
 ***************************************************************/
typedef struct {
  GenAVLEntry* es[MAX_GENAVL_STACK];
  int dir[MAX_GENAVL_STACK];
  int sp;
  int maxdepth;
  GenAVLEntry* root;
} GenAVLBFIter;
void* GenAVLBFIterInitData(GenAVLBFIter*, GenAVLTree*);
void* GenAVLBFIterNextData(GenAVLBFIter*);

#ifdef __cplusplus
}
#endif

#endif /* GENAVL_H */
