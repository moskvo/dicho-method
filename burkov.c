#include "burkov.h"

void (*dicho_tree)(node_t*, const int, item_t*) = dicho_tree_notrecursive;
node_t* (*burkovtree)(const task_t*) = optimal_dichotomic_tree;

/*
*  building optimum dichotomy tree on Burkov,Burkova's works
*/
node_t* optimal_dichotomic_tree ( const task_t *task){

  //{ finding q <= countItems - number of elements on which the maximal symmetric hierarchy must be created
    int q = find_q (task->b);
    q = (q > task->length) ? task->length : q;
  //}

  // maximum symmetric hierarchy must be created from top to down!
  // optimally sorting items
  item_t *diitems, // items for dichotomic part
         *dpitems; // items for dynamic programming branch
  prep_items(task->length, task->items, q, &diitems, &dpitems);

  // head of optimal dichotomic tree
  node_t* head = createnodes (2*task->length-1); // number of all nodes of any tree is doubled number of it's leafs minus one.

  // DP branch
  node_t *p = head;
  item_t *pl = dpitems, *tmp;
  int i;
  for ( i = 0 ; i < task->length-q ; i++ ) { // in fact p move as p = p + 2
    p->lnode = (p+1);
      p->lnode->items = NULL;
      tmp = copyitem(pl);
      HASH_ADD_KEYPTR ( hh, p->lnode->items, tmp->w, KNINT_SIZE, tmp);
      pl++;
      p->lnode->length = 1;
    p->rnode = (p+2);
    p = p->rnode;
  }

  // Dichotomic branch
  dicho_tree(p, q, diitems);

  return head;
}

// must be: q <= size
// diitems - items for dichotomic part of tree
// dpitems - items for dynamic programming part of tree
void prep_items ( const int size, item_t *list, const int q, item_t **diitems, item_t **dpitems ){
  // it's just a plug
  (*dpitems) = list;
  (*diitems) = list + (size-q); // dangerous:
}

// b(q)
knint bq[] = {0, 0, 4, 6, 12, 20, 38, 70, 140, 268, 532, 1044, 2086, 4134, 8262, 16454, 32908, 65676, 131340, 262412, 524820, 1049108, 2098196, 4195348, 8390694,
   16779302, 33558566, 67112998, 134225990, 268443718, 536887366, 1073758278, 2147516556, 4295000204, 8590000268, 17179934860, 34359869708, 68719608076, 137439215884, 274878169356};
const short bq_size = sizeof(bq) / sizeof(bq[0]);
int find_q (knint b) {
  knint *i = bq+2;
  for ( ; (i<bq+bq_size && (*i) < b ) ; i++) {}
  return (i-bq);
}

void dicho_tree_notrecursive(node_t *head, const int size, item_t *items){
  int p_size = 1, pnext_size = 2;
  node_t *p = head, *pnext, *t1, *t2;
  int *sizes = (int*)malloc(2*sizeof(int)), // size of accordingly subtree items
      *indexes = sizes + 1, // start index of items that belongs to accordingly subtree
      *tmpsizes, *tmpindexes, *ss, *ii, *ss2, *ii2;
    *sizes = size; *indexes = 0;

  // creating a maximum symmetric tree
  int dp = (int)log2f ((float)size);
  int i;
  for ( i = 0 ; i < dp ; i++ ){
    //1: pnext = (node_t*)calloc (pnext_size, NODE_SIZE); // the next level of tree
    pnext = p + p_size; // the next level of tree
    ss = (int*)malloc (2*pnext_size*sizeof(int));
    ii = ss + pnext_size;

    t2 = pnext;
    ss2 = ss;
    ii2 = ii;
    tmpsizes = sizes; // for free()
    //tmpindexes = indexes; // for free()
    int j=0,d;
    for( t1 = p ; t1 < p + p_size ; t1++ ){
      d = (*sizes) / 2;
        *ss2 = d;
        *(ss2+1) = (*sizes) - d;
        ss2 = ss2 + 2;
        *ii2 = (*indexes);
        *(ii2+1) = (*indexes) + d;
        ii2 = ii2 + 2;
        sizes++;
        indexes++;

      t1->lnode = t2++;
      t1->rnode = t2++;
      j++;
    }
    sizes = ss;
    indexes = ii;
    free (tmpsizes);
    //free(tmpindexes); commented because ss = malloc; ii = ss + ...

    p = pnext;
    p_size = pnext_size;
    pnext_size <<= 1;
  } // for i

  // hang up all remaining items (where sizes[i]==2)
  //t2 = p;
  item_t *tmp;
  pnext = p + p_size;
  for( i = 0 ; i < p_size ; i++ ){
    if( sizes[i] > 2 ) { printf("size of leaf more than 2\n"); fflush(stdout); }
    if( sizes[i] == 2 ){
      //1: pnext = (node_t*)calloc (2,NODE_SIZE);
      pnext->items = NULL;
      tmp = copyitem (&items[indexes[i]]);
      HASH_ADD_KEYPTR (hh, pnext->items, tmp->w, KNINT_SIZE, tmp);
      pnext->length = 1;
      p->lnode = pnext++;

      pnext->items = NULL;
      tmp = copyitem (&items[indexes[i]+1]);
      HASH_ADD_KEYPTR (hh, pnext->items, tmp->w, KNINT_SIZE, tmp);
      pnext->length = 1;
      p->rnode = pnext++;
    } else if( sizes[i] == 1 ){
      p->items = NULL;
      tmp = copyitem (&items[indexes[i]]);
      HASH_ADD_KEYPTR (hh, p->items, tmp->w, KNINT_SIZE, tmp);
      p->length = 1;
    } else {
      // error
      printf("size of leaf is non-positive?\n");
      fflush(stdout);
    }
    p++;
  }

}// dicho_tree_notrecursive()

/*  funcs for solving */

void treesolver(node_t* root, knint cons){
  if( root->length != 0 ) return;

  if( root->lnode->length == 0 ) treesolver (root->lnode,cons);
  if( root->rnode->length == 0 ) treesolver (root->rnode,cons);

  root->items = dichosolve(root->lnode->length, root->lnode->items,
                           root->rnode->length, root->rnode->items,
                           cons, &(root->length) );

//  print_tree(root);
}


item_t* dichosolve(int size1, item_t* first, int size2, item_t* second, knint cons, int* rezsize) {

  if( size1 == -1 && size2 == -1 ){
    *rezsize = -1;
    return 0;
  }
  if ( size1 == -1 ) {
    *rezsize = size2;
    return copyhash (second);
  }
  if ( size2 == -1 ) {
    *rezsize = size1;
    return copyhash (first);
  }

  //item_t *its = createitems0 (cons), *fp, *sp;
  item_t *its = NULL, *fp, *sp, *tmp;
  knint w, p;
  int cnt = 0; // real count of @its

  knint *wp, *pp;

  // put all elements of first table
  for ( fp = first ; fp != NULL && *(fp->w) <= cons ; fp = fp->hh.next ) {
    cnt++;
    sp = copyitem (fp);
    HASH_ADD_KEYPTR ( hh, its, sp->w, KNINT_SIZE, sp );
  }
  // put new elements of second table or replace elements having less value
  for( fp = second ; fp!= NULL && *(fp->w) <= cons ; fp = fp->hh.next ) {
    HASH_FIND (hh, its, fp->w, KNINT_SIZE, tmp);
    if( tmp == NULL ){
      cnt++;
      tmp = copyitem (fp);
      HASH_ADD_KEYPTR (hh, its, tmp->w, KNINT_SIZE, tmp);
    } else {
      *(tmp->p) = MAXINT ( *(fp->p) , *(tmp->p) );
    }
  }
  // pairwise addition
  for( fp = first ; fp != NULL ; fp = fp->hh.next ){
    for( sp = second ; sp != NULL && (p = *(fp->p) + *(sp->p),w = *(fp->w) + *(sp->w), w<=cons) ; sp = sp->hh.next ){
      HASH_FIND (hh, its, &w, KNINT_SIZE, tmp);
      if( tmp == NULL ){
        cnt++;
	tmp = copyitem ( sp );
        *(tmp->w) = w;
        *(tmp->p) = p;
        HASH_ADD_KEYPTR (hh, its, tmp->w, KNINT_SIZE, tmp);
      } else {
        *(tmp->p) = MAXINT( *(tmp->p),p );
      }
    }
  }

  // save non-zero elements in solid array
  if( cnt == 0 ){
    *rezsize = -1;
    return NULL;
  }

  *rezsize = cnt;
  return its;
}

