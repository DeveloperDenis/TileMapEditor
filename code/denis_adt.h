#ifndef DENIS_ADT_H_
#define DENIS_ADT_H_

//IMPORTANT(denis): you are required to define NodeDataType as a valid value
#include "NodeDataType.h"

struct Node
{
    Node *next;
    NodeDataType data;
};

struct LinkedList
{
    Node *front;
};

void adt_addToFront(LinkedList *ll, NodeDataType data);
void adt_addToBack(LinkedList *ll, NodeDataType data);

#endif
