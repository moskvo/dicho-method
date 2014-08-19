#ifndef _BURKOV_H_
#define _BURKOV_H_

#ifndef _TASK_H_
#include "task.h"
#endif

#ifndef _MATH_H_
#include <math.h>
#endif

#define MAXINT(x,y) ((x>y)?(x):(y))
/*
*  building optimum dichotomy tree on Burkov,Burkova's works
*/
node_t* optimal_dichotomic_tree ( const task_t* );
node_t* (*burkovtree) (const task_t*);
void dicho_tree_notrecursive ( node_t*, const int, item_t* );
void (*dicho_tree)(node_t*, const int, item_t*);
void prep_items (const int, item_t*, const int, item_t**, item_t**);
int find_q (knint);

void treesolver (node_t* , knint );
item_t* dichosolve( int,item_t*, int,item_t*, knint, int*);


#endif