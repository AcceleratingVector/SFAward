/*
 *---------------------------------------------------------------------------------
 *  Copyright (c) 2018, Kirk Benell
 *  All rights reserved. 
 *---------------------------------------------------------------------------------
 * 
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *
 * Permission is hereby  granted, free of charge, to any  person obtaining a copy
 * of this software and associated  documentation files (the "Software"), to deal
 * in the Software  without restriction, including without  limitation the rights
 * to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 * copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 * IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 * FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 * AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 * LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * Simple list - linked list. 
 *
 * this is a hack/modification of Stack list from the Arudino playground. 
 */

#ifndef _SIMPLELIST_H
#define _SIMPLELIST_H

// include Arduino basic header.
#include <Arduino.h>

// Def
template<typename T>
class SimpleList {
  public:
    SimpleList ();
    ~SimpleList ();

    // add an item to the list
    void add (const T i);

    // remove item N from the list n is 0 based
    T remove(int iItem);

    // get item N from the list
    T get(int iItem) const;

    // clear out the list
    void clear() const;

    // check if the stack is empty.
    bool isEmpty () const;

    // get the number of items in the stack.
    int count () const;


  private:

    // the structure of each node in the list.
    typedef struct node {
      T item;      // the item in the node.
      node * next; // the next node in the list.
    } node;

    typedef node * link; // synonym for pointer to a node.

    int size;        // the size of the stack.
    link head;       // the head of the list.
};

// Constructor
template<typename T>
SimpleList<T>::SimpleList () {
  size = 0;       // set the size of stack to zero.
  head = NULL;    // set the head of the list to point nowhere.
}

// clear the list
template<typename T>
SimpleList<T>::~SimpleList () {
  // deallocate memory space of each node in the list.
  for (link t = head; t != NULL; head = t) {
    t = head->next; delete head;
  }

  size = 0;       // set the size of stack to zero.
}

// add item
template<typename T>
void SimpleList<T>::add(const T i) {
  // create a new node.
  link t = (link) new node;

  // if there is a memory allocation error.
  if (t == NULL)
    return;

  // store the item to the new node.
  t->item = i;

  // new node should be the head of the list.
  t->next = head; head = t;

  // increase the items.
  size++;
}

// remove an item from the list -- position is used
template<typename T>
T SimpleList<T>::remove(int iItem) {
  // check if the stack is empty.
  if (isEmpty () || iItem > size-1)
    return (T)NULL; 

  link currNode = head;
  link prevNode = NULL;

  for(int i=0; i < iItem; i++ ){
    prevNode = currNode;
    currNode = currNode->next;
  }

  // get the item of the node being deleted
  T item = currNode->item;

  if(prevNode == NULL){
      head = currNode->next;
  }else{
      prevNode->next = currNode->next;
  }
  delete currNode; 

  // decrease the items.
  size--;

  // return the item.
  return item;
}

// get an item from the list
template<typename T>
T SimpleList<T>::get(int iItem) const {
  

    if (isEmpty () || iItem > size -1 )
        return NULL;

    link currNode = head;

    for(int i=0; i < iItem; i++ )
        currNode = currNode->next;

    return currNode->item;
}

// Clear
template<typename T>
void SimpleList<T>::clear () const {
    while(!isEmpty()){
       remove(0);
    }
}
// Empty?
template<typename T>
bool SimpleList<T>::isEmpty () const {
  return head == NULL;
}

// get the number of items in the list.
template<typename T>
int SimpleList<T>::count () const {
  return size;
}


#endif 
