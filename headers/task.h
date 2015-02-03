#ifndef _TASK_H_
#define _TASK_H_

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STRING_H
#include <string.h>
#endif

#ifndef UTHASH_H
#include "uthash.h"
#endif

/*-- item section --*/

#define KNINT_LONG
//#define KNINT_INT
typedef long int knint;
size_t KNINT_SIZE;

typedef struct item_t {
  knint *p, *w; // payoff and weight
  UT_hash_handle hh;
} item_t;
size_t ITEM_SIZE;

/*typedef struct elem_t {
  item_t *
  struct elem_t *next, *prev;
} elem_t;*/

item_t* createitems (int);
item_t* createitems0 (int);
item_t* copyitem (item_t*);
item_t* copyitems (int, item_t*);
item_t* copyhash (item_t*);
item_t* joinitems (int, item_t*, int, item_t*);
void print_items (int,item_t*);
void print_items_line (int,item_t*);
void print_hash (item_t*);


/*-- list for collect all solutions section --*/

typedef struct node_list_t {
  item_t *items;
  int length;
  struct node_list_t *next;
} node_list_t;
typedef struct head_list_t {
  node_list_t *next;
  int count;
} head_list_t;
node_list_t* createlistnode ();
head_list_t* createlisthead ();

void additems (head_list_t*, int, item_t*);
/*	add adjunct's list to end of head's list
*/	void addlist (head_list_t*, head_list_t*);
void addnode (head_list_t*, node_list_t*);
//items_list_t* addhead (items_list_t* head, int length , item_t*);
void print_list (head_list_t*);
void free_list(head_list_t**);


/*-- task section --*/

typedef struct task{
  knint b;
  item_t *items;
  int length;
} task_t;

task_t* createtask (int,knint);
task_t* readtask(char*);
void print_task (task_t*);

void free_items (item_t**);
void free_hash (item_t**);
void free_task (task_t**);

/*-- tree section --*/

typedef struct node {
  item_t  *items; // hash
  int length; // if length = 1, then free through DL_FOREACH, otherwise "HASH_CLEAR(hh,&items); free(items)"
  struct node  *lnode, *rnode, *hnode; // left, right and head nodes in tree
  int source;
} node_t;

node_t* createnodes(int size);

void print_tree ( node_t* );
void print_node ( char*, node_t* );

void free_tree ( node_t* ) ;
void free_node ( node_t* ) ;

int value_sort ( item_t*, item_t* );

/*-- solutions tree section --*/

size_t SOLNODE_SIZE;

typedef struct solnode_t {
	item_t *items;
	struct solnode_t *childs;
	int level, branch;
	UT_hash_handle hh;
} solnode_t;
solnode_t* createsolnode0 ();
solnode_t* createsolnode (int, int);
solnode_t* addsolchild ( solnode_t*, int, int );
solnode_t* addsolitem ( solnode_t*, knint*, knint* );
void print_solutions ( solnode_t* );
void free_solnodes ( solnode_t* );

#endif
