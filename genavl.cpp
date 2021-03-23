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
#include "genavl.h"

/***********************************************
 *
 * The GenAVLStackEntry is a private structure
 * used to keep track of the path to a
 * GenAVLEntry which is to be removed from the
 * tree. This path is useful in rebalancing the
 * tree after the entry has been removed.
 *
 ***********************************************/
typedef struct {
  GenAVLEntry* e;
  int d;
} GenAVLStackEntry;

/**************************************************
 * Sets the referenced child to be either the left
 * or right child of the given entry. Used
 * internally to make implementation simpler.
 **************************************************/
static void set(GenAVLTree* gatp,
                GenAVLEntry* gaep,
                int dir,
                GenAVLEntry* gaepnext) {
  if (gaep) {
    if (dir < 0)
      gaep->left = gaepnext;
    else
      gaep->right = gaepnext;
  } else
    gatp->root = gaepnext;
}

/**************************************************
 * Returns a pointer to the specified GenAVLEntry
 * given via a pointer to the parent and a
 * direction. Used internally to make the
 * implementation simpler.
 **************************************************/
static GenAVLEntry* val(GenAVLTree* gatp, GenAVLEntry* gaep, int dir) {
  if (gaep)
    return dir < 0 ? gaep->left : gaep->right;
  else
    return gatp->root;
}

/***********************************************************
 *
 * Shift the children of the given entry from left to right
 * where the imbalance extends one additional level.
 *
 ***********************************************************/
static void shiftdblright(GenAVLTree* gatp, GenAVLEntry* gaep, int dir) {
  GenAVLEntry* gaepnext;
  GenAVLEntry* gaepnextl;
  GenAVLEntry* gaepnextr;

  gaepnext = val(gatp, gaep, dir);
  gaepnextl = gaepnext->left;
  gaepnextr = gaepnextl->right;

  gaepnextl->right = gaepnextr->left;
  gaepnext->left = gaepnextr->right;
  gaepnextr->left = gaepnextl;
  gaepnextr->right = gaepnext;

  if (gaepnextr->balance == -1) {
    gaepnextl->balance = 0;
    gaepnext->balance = 1;
  } else {
    if (gaepnextr->balance == 1) {
      gaepnextl->balance = -1;
      gaepnext->balance = 0;
    } else {
      gaepnextl->balance = 0;
      gaepnext->balance = 0;
    }
  }
  gaepnextr->balance = 0;
  set(gatp, gaep, dir, gaepnextr);
}

/***********************************************************
 *
 * Shift the children of the given entry from left to right
 * where the imbalance is only a single level.
 *
 ***********************************************************/
static void shiftright(GenAVLTree* gatp, GenAVLEntry* gaep, int dir) {
  GenAVLEntry* gaepnext;
  GenAVLEntry* gaepnextl;

  gaepnext = val(gatp, gaep, dir);
  gaepnextl = gaepnext->left;

  gaepnext->left = gaepnextl->right;
  gaepnextl->right = gaepnext;
  set(gatp, gaep, dir, gaepnextl);

  if (gaepnextl->balance == -1) {
    gaepnextl->balance = 0;
    gaepnext->balance = 0;
  } else {
    gaepnextl->balance = 1;
    gaepnext->balance = -1;
  }
}

/***********************************************************
 *
 * Shift the children of the given entry from right to left
 * where the imbalance is two levels deep.
 *
 ***********************************************************/
static void shiftdblleft(GenAVLTree* gatp, GenAVLEntry* gaep, int dir) {
  GenAVLEntry* gaepnext;
  GenAVLEntry* gaepnextl;
  GenAVLEntry* gaepnextr;

  gaepnext = val(gatp, gaep, dir);
  gaepnextr = gaepnext->right;
  gaepnextl = gaepnextr->left;

  gaepnext->right = gaepnextl->left;
  gaepnextr->left = gaepnextl->right;
  gaepnextl->left = gaepnext;
  gaepnextl->right = gaepnextr;

  if (gaepnextl->balance == -1) {
    gaepnext->balance = 0;
    gaepnextr->balance = 1;
  } else {
    if (gaepnextl->balance == 1) {
      gaepnext->balance = -1;
      gaepnextr->balance = 0;
    } else {
      gaepnext->balance = 0;
      gaepnextr->balance = 0;
    }
  }
  gaepnextl->balance = 0;
  set(gatp, gaep, dir, gaepnextl);
}

/***********************************************************
 *
 * Shift the children of the given entry from right to left
 * where the imbalance is only a single level.
 *
 ***********************************************************/
static void shiftleft(GenAVLTree* gatp, GenAVLEntry* gaep, int dir) {
  GenAVLEntry* gaepnext;
  GenAVLEntry* gaepnextr;

  gaepnext = val(gatp, gaep, dir);
  gaepnextr = gaepnext->right;

  gaepnext->right = gaepnextr->left;
  gaepnextr->left = gaepnext;
  set(gatp, gaep, dir, gaepnextr);

  if (gaepnextr->balance == 1) {
    gaepnextr->balance = 0;
    gaepnext->balance = 0;
  } else {
    gaepnextr->balance = -1;
    gaepnext->balance = 1;
  }
}

/*******************************************************
 *
 * Initialize the AVL Entry
 *
 *******************************************************/
void GenAVLInit(GenAVLEntry* e, void* d) {
  e->balance = 0;
  e->right = 0;
  e->left = 0;
  e->data = d;
}

/*******************************************************
 *
 * Begins a traversal of the tree in breadth-first
 * order. Returns 0 if the tree is empty or the data
 * pointer of the root, otherwise
 *
 *******************************************************/
void* GenAVLBFIterInitData(GenAVLBFIter* gabfip, GenAVLTree* gatp) {
  return 0;
}

/*******************************************************
 *
 * Continues a traversal of the tree in breadth-first
 * order. Returns 0 if the traversal is done or the data
 * pointer of the next node, otherwise
 *
 *******************************************************/
void* GenAVLBFIterNextData(GenAVLBFIter* gabfip) {
  return 0;
}

/*******************************************************
 *
 * Traverses to the left-most leaf element, pushing all
 * nodes onto the stack and returns the data pointer
 * of the element or 0 if there are no such
 *
 *******************************************************/
void* GenAVLLFIterInitData(GenAVLLFIter* gadlip, GenAVLTree* gatp) {
  GenAVLEntry* gaep;

  gadlip->sp = 0;
  gaep = gatp->root;

  while (gaep) {
    gadlip->stack[gadlip->sp++] = gaep;
    if (gaep->left == nullptr)
      gaep = gaep->right;
    else
      gaep = gaep->left;
  }
  return GenAVLLFIterNextData(gadlip, gatp);
}

/*******************************************************
 *
 * Traverses to the next node from the top-most entry
 * in the stack and pops the stack, then traverses to
 * left most leaf-entry pushing onto the stack. When
 * a leaf is found, it is unlinked from the tree (the
 * tree is left unbalanced) and returned. The returned
 * element's avl pointers are also set to point to the
 * elements parent (the right pointer is set, if the
 * parent was pointing to the element via its right
 * pointer; the left pointer is set if the parent was
 * pointing to the element via its left pointer). This
 * is to allow a quick way of replacing the element
 * back on the tree, if necessary (using
 * GenAVLLLFReplace)
 *
 *******************************************************/
void* GenAVLLFIterNextData(GenAVLLFIter* gadlip, GenAVLTree* gatp) {
  GenAVLEntry* gaep;
  GenAVLEntry* pgaep;
  void* dp;

  /* If the stack is empty we've already visited all   */
  if (gadlip->sp == 0)
    return 0;

  /* Pop the stack to get the last visited element     */
  gaep = gadlip->stack[--gadlip->sp];
  dp = gaep->data;
  gaep->left = 0;
  gaep->right = 0;

  /* Remove element from the tree (tree is unbalanced) */
  if (gadlip->sp) {
    pgaep = gadlip->stack[gadlip->sp - 1];
    if (gatp->Compare(gaep, gatp->Key(pgaep)) > 0) {
      gaep->right = pgaep;
      pgaep->right = 0;
    } else {
      gaep->left = pgaep;
      pgaep->left = 0;
      /* Check if there is still a right branch and    */
      /* traverse down that branch, pushing all nodes  */
      if (pgaep->right) {
        gaep = pgaep->right;
        while (gaep) {
          gadlip->stack[gadlip->sp++] = gaep;
          if (gaep->left == nullptr)
            gaep = gaep->right;
          else
            gaep = gaep->left;
        }
      }
    }
  } else
    gatp->root = 0;

  return dp;
}

/*******************************************************
 *
 * replace a node removed from the tree via a leaf-first
 * iterator
 *
 *******************************************************/
void GenAVLLLFIterReplace(GenAVLTree* gatp, GenAVLEntry* gaep) {
  if (gaep->left) {
    gaep->left->left = gaep;
    gaep->left = 0;
  } else {
    if (gaep->right) {
      gaep->right->right = gaep;
      gaep->right = 0;
    } else
      gatp->root = gaep;
  }
}

/*******************************************************
 *
 * Traverses to the left-most element, pushing all
 * nodes onto the stack and returns the data pointer
 * of the left-most element or 0 if there are no such
 *
 *******************************************************/
void* GenAVLDFIterInitData(GenAVLDFIter* gadfip, GenAVLTree* gatp) {
  GenAVLEntry* gaep;

  gadfip->sp = 0;
  gaep = gatp->root;
  while (gaep) {
    gadfip->stack[gadfip->sp++] = gaep;
    gaep = gaep->left;
  }
  return GenAVLDFIterNextData(gadfip);
}

/*******************************************************
 *
 * Traverses to the left-most element lexicographically
 * greater than or equal to the element given by the key,
 * pushing all nodes onto the stack and returns the data
 * pointer of the element or 0 if there are no such
 *
 *******************************************************/
void* GenAVLDFIterInitNextEqualData(GenAVLDFIter* gadfip,
                                    GenAVLTree* gatp,
                                    const void* key) {
  GenAVLEntry* gaep;

  gadfip->sp = 0;
  gaep = gatp->root;
  while (gaep) {
    if (gatp->Compare(gaep, key) >= 0) {
      gadfip->stack[gadfip->sp++] = gaep;
      gaep = gaep->left;
    } else
      gaep = gaep->right;
  }
  return GenAVLDFIterNextData(gadfip);
}

/*******************************************************
 *
 * Traverses to the left-most element lexicographically
 * greater than the element given by the key, pushing
 * all nodes onto the stack and returns the data
 * pointer of the element or 0 if there are no such
 *
 *******************************************************/
void* GenAVLDFIterInitNextData(GenAVLDFIter* gadfip,
                               GenAVLTree* gatp,
                               const void* key) {
  GenAVLEntry* gaep;

  gadfip->sp = 0;
  gaep = gatp->root;
  while (gaep) {
    if (gatp->Compare(gaep, key) > 0) {
      gadfip->stack[gadfip->sp++] = gaep;
      gaep = gaep->left;
    } else
      gaep = gaep->right;
  }
  return GenAVLDFIterNextData(gadfip);
}

/*******************************************************
 *
 * Traverses to the next node from the top-most entry
 * in the stack and pops the stack, then traverses to
 * left most entry pushing onto the stack
 *
 *******************************************************/
void* GenAVLDFIterNextData(GenAVLDFIter* gadfip) {
  GenAVLEntry* gaep;
  void* dp;

  /* If the stack is empty we've already visited all   */
  if (gadfip->sp == 0)
    return 0;

  /* Pop the stack to get the last visited element     */
  gaep = gadfip->stack[--gadfip->sp];
  dp = gaep->data;

  /* For the node to the right, push all the left nodes*/
  if (gaep->right) {
    gaep = gaep->right;
    while (gaep) {
      gadfip->stack[gadfip->sp++] = gaep;
      gaep = gaep->left;
    }
  }

  /* Return the data pointer of the last visited node  */
  return dp;
}

/*******************************************************
 *
 * Finds the next free key value - if the value is found
 * returns 1, if there are no free values, return 0. The
 * value is returned through a passed pointer
 *
 *******************************************************/
int GenAVLTreeNextFreeKey(GenAVLTree* gatp, const void* start, void* next) {
  GenAVLEntry* gaep;
  GenAVLEntry* st[MAX_GENAVL_STACK];
  int sp = 0;

  /* If the AVL is empty, simply return the next value */
  if (gatp->root == nullptr) {
    gatp->KeyIncrement(next);
    return 1;
  }
  gaep = gatp->root;

  /* This loop performs the following:                 */
  /*   - Find the lowest value >= the start            */
  /*   - From the found value do an in-order xversal   */
  /*     until a hole is found                         */
  /*   - if the max value is found, start from the min */
  /*   - if the search wraps back to start give up     */
  while (1) {
    if (gatp->Compare(gaep, next) > 0) {
      /* Node is greater that next, so go left         */
      if (gaep->left == nullptr) {
        /* Couldn't go left - if the node != next + 1  */
        /* we've found the hole - also check for wrap  */
        gatp->KeyIncrement(next);
        if (gatp->KeyCompare(next, start) == 0)
          return 0;
        if (gatp->Compare(gaep, next) == 0)
          /* There is no hole so continue on right     */
          goto tryright;
        return 1;
      } else {
        /* Keep going lower, but push the current node */
        /* so we can return to it for later comparison */
        st[sp++] = gaep;
        gaep = gaep->left;
      }
    } else {
    tryright:
      /* Node is <= the next, so go right              */
      if (gaep->right == nullptr) {
        /* Couldn't go right so check if there's a     */
        /* previous left pushed onto the stack         */
        if (sp) {
          /* Take the node off the stack and check if  */
          /* there's a hole - also check for wrap      */
          gaep = st[--sp];
          gatp->KeyIncrement(next);
          if (gatp->KeyCompare(next, start) == 0)
            return 0;
          if (gatp->Compare(gaep, next) == 0)
            /* There is no hole so continue on left    */
            goto tryright;
          return 1;
        } else {
          /* No further old lefts on stack so we've    */
          /* reached the right-most node, check if we  */
          /* need to wrap on max - otherwise simply    */
          /* get next value and return                 */
          if (gatp->KeyIncrement(next) == 1)
            gaep = gatp->root;
          else
            return 1;
        }
      } else
        /* Keep going higher                          */
        gaep = gaep->right;
    }
  }
}

/*******************************************************
 *
 * Find and return a pointer to the entry given by the
 * key data. Return 0 if no such entry is in the tree.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreeFind(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;
  int dir;

  for (gaep = gatp->root; gaep;) {
    if ((dir = gatp->Compare(gaep, key)) > 0)
      gaep = gaep->left;
    else {
      if (dir < 0)
        gaep = gaep->right;
      else
        return gaep;
    }
  }

  return 0;
}
void* GenAVLTreeFindData(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreeFind(gatp, key)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Find and return a pointer to the first entry in the
 * tree based on lexicographical order.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreeFirst(GenAVLTree* gatp) {
  GenAVLEntry* gaep;
  GenAVLEntry* first;

  for (first = 0, gaep = gatp->root; gaep;) {
    first = gaep;
    gaep = gaep->left;
  }

  return first;
}
void* GenAVLTreeFirstData(GenAVLTree* gatp) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreeFirst(gatp)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Return a pointer to the GenAVLEntry which is
 * next (lexicographically) to the given key. Return
 * 0 if there is no such entry.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreeNext(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;
  GenAVLEntry* next;

  for (next = 0, gaep = gatp->root; gaep;) {
    if (gatp->Compare(gaep, key) > 0) {
      next = gaep;
      gaep = gaep->left;
    } else {
      gaep = gaep->right;
    }
  }

  return next;
}
void* GenAVLTreeNextData(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreeNext(gatp, key)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Return a pointer to the GenAVLEntry which is
 * either equal to the given key or next
 * (lexicographically) to the given key. Return
 * 0 if there is no such entry.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreeEqualNext(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;
  GenAVLEntry* next;
  int dir;

  for (next = 0, gaep = gatp->root; gaep;) {
    if ((dir = gatp->Compare(gaep, key)) > 0) {
      next = gaep;
      gaep = gaep->left;
    } else {
      if (dir < 0)
        gaep = gaep->right;
      else
        return gaep;
    }
  }

  return next;
}
void* GenAVLTreeEqualNextData(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreeEqualNext(gatp, key)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Return a pointer to the GenAVLEntry which is
 * previous (lexicographically) to the given key. Return
 * 0 if there is no such entry.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreePrev(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;
  GenAVLEntry* prev;

  for (prev = 0, gaep = gatp->root; gaep;) {
    if (gatp->Compare(gaep, key) < 0) {
      prev = gaep;
      gaep = gaep->right;
    } else {
      gaep = gaep->left;
    }
  }

  return prev;
}
void* GenAVLTreePrevData(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreePrev(gatp, key)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Return a pointer to the GenAVLEntry which is
 * either equal to the given key or previous
 * (lexicographically) to the given key. Return
 * 0 if there is no such entry.
 *
 *******************************************************/
GenAVLEntry* GenAVLTreeEqualPrev(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;
  GenAVLEntry* prev;
  int dir;

  for (prev = 0, gaep = gatp->root; gaep;) {
    if ((dir = gatp->Compare(gaep, key)) < 0) {
      prev = gaep;
      gaep = gaep->right;
    } else {
      if (dir > 0)
        gaep = gaep->left;
      else
        return gaep;
    }
  }

  return prev;
}
void* GenAVLTreeEqualPrevData(GenAVLTree* gatp, const void* key) {
  GenAVLEntry* gaep;

  if ((gaep = GenAVLTreeEqualPrev(gatp, key)) == nullptr)
    return 0;
  else
    return gaep->data;
}

/*******************************************************
 *
 * Add the given GenAVLEntry to the tree. If there
 * exists an entry with the same key already in the
 * tree, return 0. Otherwise return 1.
 *
 * This function performs an AVL tree insertion without
 * balancing. Note that a tree which has been left
 * unbalanced cannot be change using the regular AVL
 * calls.
 * 
 * Using Unbalanced add has the advantage that the tree
 * will always be in a consistent state, even if the
 * process doing the Add is interrupted or killed.
 *
 *******************************************************/
int GenAVLTreeAddUnbal(GenAVLTree* gatp, GenAVLEntry* gae) {
  GenAVLEntry* gaep = 0;
  GenAVLEntry* gaepnext;
  int dir = 0;

  /* Initialize the left and right ptrs   */
  gae->left = 0;
  gae->right = 0;
  gae->balance = 0;

  /* Find the insertion and balance point */
  for (gaepnext = gatp->root; gaepnext;) {
    gaep = gaepnext;
    if ((dir = gatp->Compare(gae, gatp->Key(gaepnext))) < 0)
      gaepnext = gaepnext->left;
    else {
      if (dir > 0)
        gaepnext = gaepnext->right;
      else
        /* Entry already exists */
        return 0;
    }
  }

  /* Insert the new entry */
  set(gatp, gaep, dir, gae);

  return 1;
}

/*******************************************************
 *
 * Add the given GenAVLEntry to the tree. If there
 * exists an entry with the same key already in the
 * tree, return 0. Otherwise return 1.
 *
 * This function performs an AVL tree insertion with
 * balancing.
 *
 *******************************************************/
int GenAVLTreeAdd(GenAVLTree* gatp, GenAVLEntry* gae) {
  GenAVLEntry* gaep = 0;
  GenAVLEntry* balgaep = 0;
  GenAVLEntry* gaepnext;
  int baldir = 0;
  int dir = 0;

  /* Initialize the left and right ptrs   */
  gae->left = 0;
  gae->right = 0;
  gae->balance = 0;

  /* Find the insertion and balance point */
  for (gaepnext = gatp->root; gaepnext;) {
    if (gaepnext->balance && gaep) {
      balgaep = gaep;
      baldir = dir;
    }

    gaep = gaepnext;
    if ((dir = gatp->Compare(gae, gatp->Key(gaepnext))) < 0)
      gaepnext = gaepnext->left;
    else {
      if (dir > 0)
        gaepnext = gaepnext->right;
      else
        /* Entry already exists */
        return 0;
    }
  }

  /* Insert the new entry */
  set(gatp, gaep, dir, gae);

  /* Balance starting at the balance point */
  for (gaep = val(gatp, balgaep, baldir); gaep != gae;) {
    if (gatp->Compare(gae, gatp->Key(gaep)) < 0) {
      gaep->balance--;
      gaep = gaep->left;
    } else {
      gaep->balance++;
      gaep = gaep->right;
    }
  }

  /* Shift to restore proper balance if needed */
  gaep = val(gatp, balgaep, baldir);
  if (gaep->balance == 2) {
    if (gaep->right->balance == 1)
      shiftleft(gatp, balgaep, baldir);
    else if (gaep->right->balance == -1)
      shiftdblleft(gatp, balgaep, baldir);
  } else if (gaep->balance == -2) {
    if (gaep->left->balance == -1)
      shiftright(gatp, balgaep, baldir);
    else if (gaep->left->balance == 1)
      shiftdblright(gatp, balgaep, baldir);
  }
  return 1;
}

/***********************************************************
 *
 * Remove the entry with the given key from the tree. If
 * the entry is not in the tree, return 0. Otherwise, return
 * data ptr.
 *
 * This function performs an AVL tree entry removal and
 * a balance.
 *
 ***********************************************************/
void* GenAVLTreeDelete(GenAVLTree* gatp, const void* key) {
  GenAVLStackEntry stack[MAX_GENAVL_STACK];
  GenAVLStackEntry* gasep;
  GenAVLEntry* gaepnext;
  void* data;
  GenAVLEntry* gaep = 0;
  int dir = 0;

  for (gasep = stack, gaepnext = gatp->root;;) {
    if (!gaepnext)
      return 0;

    /* Push the previous entry onto the stack */
    gasep->e = gaep;
    gasep->d = dir;
    gasep++;

    gaep = gaepnext;
    if ((dir = gatp->Compare(gaepnext, key)) > 0)
      gaepnext = gaepnext->left;
    else {
      if (dir < 0)
        gaepnext = gaepnext->right;
      else
        break;
    }

    /* Change direction since the key is compared  */
    /* against some entry - the compare polarity   */
    /* is opposite the normal direction            */
    dir = 0 - dir;
  }
  data = gaep->data;

  /* Swap with previous element */
  if (gaep->right && gaep->left) {
    GenAVLStackEntry* savegasep;
    GenAVLEntry* saveleft;
    GenAVLStackEntry* tempgasep;

    savegasep = gasep;
    gasep->e = gaep;
    gasep->d = -1;
    gasep++;
    gaepnext = gaep->left;
    while (gaepnext->right) {
      gasep->e = gaepnext;
      gasep->d = 1;
      gasep++;
      gaepnext = gaepnext->right;
    }
    saveleft = gaepnext->left;
    /* Swap gaep and savegaep */
    savegasep->e = gaepnext;
    savegasep->d = -1;
    tempgasep = savegasep - 1;
    set(gatp, tempgasep->e, tempgasep->d, gaepnext);
    gaepnext->right = gaep->right;
    gaepnext->left = gaep->left;
    gaepnext->balance = gaep->balance;
    gaep->right = 0;
    gaep->left = saveleft;
  }

  /* Delete entry from tree */
  gasep = gasep - 1;
  set(gatp, gasep->e, gasep->d, gaep->right ? gaep->right : gaep->left);

  /* Go back up tree, rebalancing when necessary */
  while (gasep > stack) {
    gaepnext = gasep->e;
    if (gasep->d > 0)
      gaepnext->balance--;
    else
      gaepnext->balance++;

    gasep = gasep - 1;
    gaep = gasep->e;
    dir = gasep->d;
    if (gaepnext->balance == 2) {
      if (gaepnext->right->balance == 1)
        shiftleft(gatp, gaep, dir);
      else {
        if (gaepnext->right->balance == -1)
          shiftdblleft(gatp, gaep, dir);
        else {
          shiftleft(gatp, gaep, dir);
          break;
        }
      }
      continue;
    } else if (gaepnext->balance == -2) {
      if (gaepnext->left->balance == -1)
        shiftright(gatp, gaep, dir);
      else {
        if (gaepnext->left->balance == 1)
          shiftdblright(gatp, gaep, dir);
        else {
          shiftright(gatp, gaep, dir);
          break;
        }
      }
      continue;
    } else {
      if (gaepnext->balance == 0)
        continue;
      else
        break;
    }
  }

  return data;
}
