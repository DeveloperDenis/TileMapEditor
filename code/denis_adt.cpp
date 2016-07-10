#include "denis_adt.h"
#include "stdlib.h"

void adt_addToFront(LinkedList *ll, NodeDataType data)
{
    if (ll)
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
}

void adt_addToBack(LinkedList *ll, NodeDataType data)
{
    if (ll)
    {
	Node *newNode = (Node *)malloc(sizeof(Node));
	newNode->next = 0;
	newNode->data = data;

	if (ll->front == 0)
	{
	    ll->front = newNode;
	}
	else
	{
	    Node *current = ll->front;
	    while (current != 0 && current->next != 0)
	    {
		current = current->next;
	    }

	    if (current)
	    {
		current->next = newNode;
	    }
	}
    }
}
