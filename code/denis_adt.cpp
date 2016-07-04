#include "denis_adt.h"
#include "stdlib.h"

void adt_addTo(LinkedList *ll, NodeDataType data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->next = 0;
    newNode->data = data;

    if (ll->front != 0)
    {
	newNode->next = ll->front;
    }
    
    ll->front = newNode;
}
