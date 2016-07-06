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
    //TODO(denis): dunno about this id thing
    int id;
    
    Node *front;
};

void adt_addTo(LinkedList *ll, NodeDataType data);

#endif
