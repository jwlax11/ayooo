#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_URL 256
#define MAX_CACHE 1049000
#define MAX_OBJECT 102400

struct cNode {
	char url[MAX_URL];
	int size;
	char content[MAX_URL]; //response
	struct Node *next;
	struct Node *prev;
};

cNode* insert(cNode* head, char url, int size, char content);
void remove(cNode* toRemove);
cNode* search(cNode* head, char url);
cNode* mostRecent(cNode* head, cNode* newHead);

#endif