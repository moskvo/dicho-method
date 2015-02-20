#ifndef _DICHOSOLVER_H_
#define _DICHOSOLVER_H_

#include "burkov.h"


typedef struct branch_t {
  int level, branch; // level and number of fork
  struct branch_t *next;
} branch_t;
typedef struct bud_t {
  int count; // count of branches
  int *oldbranch; // [level1,branch1,level2,...]
  int oldcount; 
  branch_t *next;
} bud_t;
size_t BRANCH_SIZE;
size_t BUD_SIZE;

bud_t* createbud0();
bud_t* createbud(int, int*);
bud_t* lightcopybud (bud_t *bud);
bud_t* budoff(bud_t *bud, int level, int branch);

void grow (bud_t*, int, int);

node_t* receive_brother (int, node_t*, int*, MPI_Status*);
void reconstruction (node_t *root, knint weight, int, bud_t*);
task_t* readnspread_task (char*, int*);
task_t* readnspread_task_parallel (char*, int*);


#endif
