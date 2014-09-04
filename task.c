#include "task.h"

/*-- item section --*/
size_t KNINT_SIZE = sizeof(knint);
size_t ITEM_SIZE = sizeof(item_t);

item_t* createitems(int size){
  if( size < 1 ) return 0;
  knint *p = (knint*)malloc (size*KNINT_SIZE),
        *w = (knint*)malloc (size*KNINT_SIZE);
  item_t *items = (item_t*)malloc (size*sizeof(item_t)), *i;
  if( (p == 0) || (w == 0) || (items == 0) ) { return 0; }

  for( i=items ; i < items+size ; i++ )
  {  i->p = p++; i->w = w++; }

  return items;
}

item_t* createitems0(int size){
  if( size < 1 ) return 0;
  knint *p = (knint*)calloc (size,KNINT_SIZE),
        *w = (knint*)calloc (size,KNINT_SIZE);
  item_t *items = (item_t*)malloc (size*sizeof(item_t)), *i;
  if( (p == 0) || (w == 0) || (items == 0) ) { return 0; }

  for( i=items ; i < items+size ; i++ )
  {  i->p = p++; i->w = w++; }

  return items;
}

item_t* copyitem (item_t *other){
  item_t* r = (item_t*)malloc(ITEM_SIZE);
  r->p = (knint*)malloc(KNINT_SIZE);
  r->w = (knint*)malloc(KNINT_SIZE);
  *(r->p) = *(other->p);
  *(r->w) = *(other->w);
  return r;
}

item_t* copyitems (int size, item_t *others) {
  item_t* r = createitems(size);
  memcpy (r->p,others->p,size*KNINT_SIZE);
  memcpy (r->w,others->w,size*KNINT_SIZE);
  return r;
}

item_t* copyhash (item_t *other) {
  item_t *hash = NULL, *ptr, *tmp;

  for ( ptr = other ; ptr != NULL ; ptr = ptr->hh.next ) {
    tmp = copyitem (ptr);
    HASH_ADD_KEYPTR ( hh, hash, tmp->w, KNINT_SIZE, tmp );
  }
  return hash;
}

item_t* joinitems(int size1, item_t *it1, int size2, item_t *it2) {
  item_t *items = createitems(size1+size2);
  memcpy (items->p, it1->p, size1*KNINT_SIZE);
  memcpy (items->w, it1->w, size1*KNINT_SIZE);
  memcpy (items->p+size1, it2->p, size2*KNINT_SIZE);
  memcpy (items->w+size1, it2->w, size2*KNINT_SIZE);
  return items;
}

void print_items ( int size , item_t *item ){
  knint *p, *w;

  for( p = item->p, w = item->w ; p < item->p+size ; p++,w++ )
    printf("(%4ld %4ld) ",*p,*w);
  puts("");
}

void print_hash (item_t *hash){
  item_t *p;
  for ( p = hash ; p != NULL ; p = p->hh.next ) {
    printf ("(%4ld %4ld) ",*(p->p),*(p->w));
  }
  puts("");
}

const size_t NODELIST_SIZE = sizeof(node_list_t),
             HEADLIST_SIZE = sizeof(head_list_t);

node_list_t* createlistnode() {
  return (node_list_t*) calloc (1, NODELIST_SIZE);
}
head_list_t* createlisthead() {
  return (head_list_t*) calloc (1, HEADLIST_SIZE);
}

void additems (head_list_t* head, int size, item_t* a) {
  node_list_t* n = createlistnode();
  n->items = a;
  n->length = size;
  head->count++;
  n->next = head->next;
  head->next = n;
}

void addnode (head_list_t *head, node_list_t *node) {
  head->count++;
  node->next = head->next;
  head->next = node;
}

void addlist (head_list_t* head, head_list_t* adjunct) {
  if ( adjunct == NULL || adjunct->next == NULL ) return;
  node_list_t* t = adjunct->next;
  int i;
  for ( i = 1 ; i < adjunct->count ; i++ ) t = t->next;
  t->next = head->next;
  head->next = adjunct->next;
  head->count += adjunct->count;
}

head_list_t* cartesian (head_list_t* set1, head_list_t* set2) {
  head_list_t *theset = createlisthead ();
  node_list_t *one, *two, *both;
  for ( one = set1->next ; one != NULL ; one = one->next ){
    for ( two = set2->next ; two != NULL ; two = two->next ){
      both = createlistnode();
      both->items = joinitems (one->length, one->items, two->length, two->items);
      both->length = one->length + two->length;
      addnode ( theset , both );
    }
  }

  return theset;
}

void print_list (head_list_t *head) {
  puts("[");
  if ( head->count > 0 ) {
    node_list_t *n;
    for ( n = head->next ; n != 0 ; n = n->next ){
      print_items (n->length, n->items);
    }
  }
  puts("]");
}

void free_list (head_list_t **head) {
  node_list_t *t = (*head)->next, *p;
  free(*head);
  *head = 0;
  while ( t != 0 ){
    p = t;
    t = t->next;
    free_items (&(p->items));
    free (p);
  }
}

/*-- task section --*/

task_t* createtask(int size, knint b){
  if( size < 1 ) return 0;
  task_t *t = (task_t*)malloc(sizeof(task_t));
  t->length = size;
  t->b = b;
  t->items = createitems (size);
  return t;
}

/*
  File's format:
  n
  c1 c2 ... cn
  w1 w2 ... wn
  b
*/
task_t* readtask(char* filename){
  task_t* task;

  FILE *file;
  if( (file = fopen(filename,"r")) == 0 ) return 0;

    knint b;
    if( fscanf(file,"%ld",&b) != 1 ) return 0;
    
    int size;
    if( fscanf(file,"%d",&size) != 1 ) return 0;
    
    task = createtask(size,b);

    item_t *head = task->items;
    knint *tmp;
    for( tmp = head->p ; tmp < head->p+size ; tmp++ )
    { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }
    for( tmp = head->w ; tmp < head->w+size ; tmp++ )
    { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }


  fclose(file);

  return task;
}
/*task_t* readtask(char* filename, int part, int groupsize){
  if( groupsize < 1 || part > groupsize ) return 0;
  task_t* task = (task_t*) malloc(sizeof(task_t));

  FILE *file;
  if( (file = fopen(filename,"r")) == 0 ) return 0;

    int size;
    if( fscanf(file,"%u",&size) != 1 ) return 0;
    size = size/groupsize + ( part == groupsize )?(size%groupsize):0;
    task->length = size;

    if( (task->items = createitems(size)) == 0 ) return 0;
    item_t *head = task->items;
    int *tmp;
    for( tmp = head->p ; tmp < head->p+size ; tmp++ )
    { if( fscanf (file,"%u", tmp) != 1 ) return 0; }
    for( tmp = head->w ; tmp < head->w+size ; tmp++ )
    { if( fscanf (file,"%u", tmp) != 1 ) return 0; }
    if( fscanf(file,"%u",&(task->b)) != 1 ) return 0;

  fclose(file);

  return task;
}*/



void free_items (item_t **headp){
  if( headp ) {
  free ((*headp)->p);
  free ((*headp)->w);
  free (*headp);
  *headp = NULL;
  }
}

void free_hash (item_t **hash){
  if( hash ) {
  item_t *p, *tmp;
  HASH_ITER (hh, *hash, p, tmp) {
    HASH_DEL (*hash, p);
    free_items (&p);
  }
  *hash = NULL;
  }
}

void free_task(task_t **p){
  if( p ) {
  free_items ( &((*p)->items) );
  free (*p);
  *p = 0;
  }
}

/*-- node section --*/

const size_t NODE_SIZE = sizeof(node_t);

node_t* createnodes (int size) {
  if( size < 1 ) return 0;
  node_t *rez = (node_t*)calloc(size,NODE_SIZE), *t;
  for ( t = rez ; t < rez + size ; t++ ) {
    t->source = -1;
  }
  return rez;
}

void print_tree (node_t *root){
  print_node ("", root);
}

void print_node (char * pre, node_t *node){
  fputs(pre,stdout);
  if( node->length < 1 ) puts("It hasn't any items");
  else {
    item_t *item = node->items;
    for ( ; item != NULL ; item = item->hh.next ) {
      printf ("(%ld %ld) ",*(item->p),*(item->w));
    }
    printf("src:%d",node->source);
    puts("");
  }
  char *lpre = (char*)malloc(strlen(pre)+4), *rpre = (char*)malloc(strlen(pre)+4);
  strcpy(lpre,pre);
  strcpy(rpre,pre);
  if( node->lnode != 0 ) print_node(strcat(lpre,"  |"), node->lnode);
  if( node->rnode != 0 ) print_node(strcat(rpre,"  |"), node->rnode);
  free(lpre); free(rpre);
}

void free_node (node_t* node){
  free_hash (&(node->items));
  free (node);
}

void free_tree (node_t *root){
  if( root->lnode != NULL ) free_tree ( root->lnode );
  if( root->rnode != NULL ) free_tree ( root->rnode );
  free_node (root);
  //(*root) = 0;
}

int value_sort (item_t *a, item_t *b) {
  if ( *(a->p) < *(b->p) ) return (int) 1;
  if ( *(a->p) > *(b->p) ) return (int) -1;
  return 0;
}
