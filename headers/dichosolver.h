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

typedef struct branch_t {
  int level, branch; // level and number of fork
  struct branch_t *next;
} branch_t;
typedef struct bud_t {
  int count; // count of branches
  int *oldbranch; // [level1,branch1,level2,...]
  int oldcount; 
  struct branch_t *next;
} branch_t;
size_t BRANCH_SIZE;
size_t BUD_SIZE;

bud_t* createbud();
bud_t* createbud(int, int*);
bud_t* lightcopybud (bud_t *bud);
bud_t* budoff(bud_t *bud, int level, int branch);

void grow (bud_t*, int, int);



#endif
