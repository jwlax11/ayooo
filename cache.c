#include "cache.h"
#include <unistd.h>

int cacheSize = 0;

cNode* insert(cNode* head, char url, int size, char content){

	Node* toAdd = initNode();
	toAdd->url = url
	toAdd->size = size;
	toAdd->content = content;

	toAdd = mostRecent(head, toAdd);
	cacheSize = toAdd->size + cacheSize;


	//can probs change to an if statement here
	while(cacheSize > MAX_CACHE){
		remove(head->prev);
	}


	if(cacheSize > 0){
		return head;
	}
	else{
		return NULL;
	}

}

cNode* initNode(){
	cNode* n = (cNode*)malloc(sizeof(cNode));
	n->url[0] = 0;
	n->size = 0;
	n->content = NULL;
	n->next = NULL;
	n->prev = NULL;
	return n;
}

void remove(cNode* toRemove){

//	if(toRemove->prev != NULL){
	if(toRemove == NULL)
		cNode *head = toRemove->next;
		head->prev = toRemove->prev;
		toRemove->prev->next = head;
		cacheSize = cacheSize - toRemove->size;
		free(toRemove->size);
		free(toRemove->content);
		free(toRemove);
		toRemove = NULL;

/*
		cNode *newLast = toRemove->prev;
		toRemove->prev = NULL;
		newLast->next = NULL;
		cacheSize = cacheSize - toRemove->size;
		free(toRemove->size);
		free(toRemove->content);
		free(toRemove);
		toRemove = NULL;
*/
	}
	else{
		return;
	}

}

cNode* search(cNode* head, char url){
	//add if null statement before calling this.
	cNode* curr = head;

	while(curr != NULL){

		if(strcmp(curr->url, url) == 0){
			return curr;
		}
		curr = curr->next;
	}

	return NULL;
}

cNode* mostRecent(cNode* head, cNode* newHead){

	if(newHead->prev != NULL){
		newHead->prev->next = newHead->next;
		newHead->next->prev = newHead->prev;
	}

	//no idea what this is:
	newHead->next = (head == NULL) ? newHead : head;
	newHead->prev = (head == NULL) ? newHead : head->prev;
	if(head != NULL){
		head->prev->next = newHead;
		head->prev = newHead;
	}
	head = newHead;
	return head;

}





