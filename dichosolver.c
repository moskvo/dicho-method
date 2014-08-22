#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#include "burkov.h"

#define SIZE_MSG 100
#define P_MSG 101
#define W_MSG 102
#define B_MSG 103
#define RECONSTR_MSG 200
#define SOL_SIZE_MSG 201

#ifdef KNINT_INT
#define MPI_KNINT MPI_INT
#else
#define MPI_KNINT MPI_LONG
#endif


node_t* receive_brother(int, node_t*, int*, MPI_Status*);
head_list_t* reconstruction(node_t *root, int element);
task_t* readnspread_task(char*, int*);

int value_sort (item_t *a, item_t *b);

int main(int argc, char** argv){
  // mpi headers
  int groupsize, myrank;
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &groupsize);
  MPI_Comm_rank (MPI_COMM_WORLD, &myrank);

  if ( argc < 2 ){
    MPI_Finalize ();
    printf("Task #%d say: not enough arguments: filename needed\n",myrank);
    exit(1);
  }

  task_t *mytask;

  MPI_Request *msgreq = (MPI_Request*)malloc(4*sizeof(MPI_Request)), *reqsz = msgreq, *reqp = msgreq+1, *reqw = msgreq+2, *reqb = msgreq+3;
  MPI_Status *msgstat = (MPI_Status*)malloc(4*sizeof(MPI_Status)), *statsz = msgstat, *statp = msgstat+1, *statw = msgstat+2, *statb = msgstat+3;

/* get task code for process zero */
if (myrank == 0)
{
  mytask = readnspread_task (argv[1], &groupsize);

}

/* get task code for process one */
else {
    // get b
    knint b;
    MPI_Bcast (&b,1,MPI_KNINT,0,MPI_COMM_WORLD);

    // get size of elements
    int size;
    MPI_Recv (&size,1,MPI_INT,0,SIZE_MSG,MPI_COMM_WORLD, statsz);
    if( size < 1 ){
      free (msgreq);
      free (msgstat);
      MPI_Finalize ();
      return 0;
    }

    // get values and weights of elements
    mytask = createtask (size,b);
    MPI_Irecv (mytask->items->p,size,MPI_KNINT,0,P_MSG,MPI_COMM_WORLD, reqp);
    MPI_Irecv (mytask->items->w,size,MPI_KNINT,0,W_MSG,MPI_COMM_WORLD, reqw);
    MPI_Waitall (2,reqp,statp);
}

  //printf("%d readed. b=%ld, size=%d. solving...\n",myrank,mytask->b,mytask->length); fflush(stdout);

  //{ solve mytask
  node_t *root;
  if( (root = burkovtree ( mytask )) == 0 )
  { puts("Can't build optdichotree"); fflush(stdout); }

  treesolver (root,mytask->b);
  //}

  //printf("%d:\n",myrank);
  //print_tree(root); fflush(stdout);  //printf("%d: local task solved. Solving parallel...\n", myrank); fflush(stdout);

  // get elements(solutions) from other processes and solve again with it
  int frontier = groupsize, group;
  int cnt = 0;

  while ( frontier > 1 && myrank < frontier ){
    group = frontier;
    frontier = group / 2;

    if( myrank < frontier ){
      root = receive_brother (myrank+frontier, root, &cnt, statsz);
      if( (myrank + 1 == frontier) && (group % 2 == 1) ){
        root = receive_brother (group-1, root, &cnt, statsz);
      }

      //if ( myrank == 0 ) { puts("-"); fflush(stdout); if(root->rnode->length > 0) print_items (root->rnode->length, root->rnode->items); else puts("0");/*print_tree(root);*/ fflush(stdout); }
      // solve them all
      treesolver(root,mytask->b);
      //if ( myrank == 0 ) { puts("par solved."); fflush(stdout); }
    }
  }

  // if process zero is ended previous code then complete solution is obtained, print it
  if ( myrank == 0 ) {
    if ( root->length == -1 ) { puts("length == -1"); fflush(stdout); }
    else {
      // sorting
      
      // print it
      printf("(p=%d w=%d)\n",root->items->p[root->length-1],root->items->w[root->length-1]); fflush(stdout);
    }

  }
  // other processes must send their solutions to appropiate process
  else {
    int to = myrank - frontier;
    if( (myrank+1 == group) && (group % 2 == 1) ) to = frontier - 1;
    // the number of solution elements
    MPI_Isend(&(root->length), 1, MPI_INT, to, SIZE_MSG, MPI_COMM_WORLD, reqsz);
    if( root->length > 0 ){
      knint *p = (knint*)malloc(root->length*KNINT_SIZE), *w = (knint*)malloc(root->length*KNINT_SIZE), *pp, *pw;
      for ( fp = root->items, pp = p, pw = w ; fp != NULL ; fp = fp->hh.next, pp++, pw++ ){
        *pp = *(fp->p);
        *pw = *(fp->w);
      }
      // send p and w
      MPI_Isend( p, root->length, MPI_KNINT,to,P_MSG, MPI_COMM_WORLD, reqp);
      //printf("%d send: ",myrank); print_items(root->length, root->items);
      MPI_Isend( w, root->length, MPI_KNINT,to,W_MSG, MPI_COMM_WORLD, reqw);
      MPI_Waitall(2,reqp,statp);
    }
    MPI_Wait(reqsz,statsz); free(p); free(w);

  }

  printf("%d: solution reconstruction\n",myrank); fflush(stdout);
  // we get the optimal knapsack value and weight,
  //  now we must reconstruct elements of it. All sets of elements that leading to optimal knapsack

  int size;
  knint weight;
  head_list_t *solution;
  // process zero run the reconstruction
  if ( myrank == 0 ){
    //element = root->length-1;
    // reconstruction() return head to list of all sets of solutions.
    solution = reconstruction ( root, *(root->items->w) );

    puts("0: print first solution."); fflush(stdout);
    print_items (solution->next->length, solution->next->items);
    // what is it? like quit signal
    int i;
    knint killsig = -1;
    for( i = 1; i < groupsize ; i++ ){
      MPI_Isend (&killsig, 1, MPI_KNINT, i, RECONSTR_MSG, MPI_COMM_WORLD, reqsz);
    }
  }
  // other processes are waiting for reconstruction signal
  else {
    MPI_Recv (&weight, 1, MPI_KNINT, MPI_ANY_SOURCE, RECONSTR_MSG, MPI_COMM_WORLD, statb);
    if( weight > -1 ){
      printf("%d: i receive 'reconstruction' message for weight: %ld\n",myrank,weight); fflush(stdout);
      //print_tree(root); fflush(stdout);
      solution = reconstruction(root, weight);
      printf("%d: i sending reconstructed message in size %d\n",myrank,solution->count);
      print_list (solution); fflush(stdout);
      MPI_Isend (&(solution->count), 1, MPI_INT, statb->MPI_SOURCE, SOL_SIZE_MSG, MPI_COMM_WORLD,reqb);
      node_list_t *i;
      //int ct = 0;
      for ( i = solution->next ; i != NULL ; i = i->next ){
        if ( i->length < 1 ) { printf("%d: node's length = %d\n",myrank,i->length); fflush(stdout); }
        MPI_Isend (&(i->length), 1, MPI_INT, statb->MPI_SOURCE, SIZE_MSG, MPI_COMM_WORLD, reqsz);
        MPI_Isend (i->items->p, i->length, MPI_KNINT, statb->MPI_SOURCE, P_MSG, MPI_COMM_WORLD, reqp);
        MPI_Isend (i->items->w, i->length, MPI_KNINT, statb->MPI_SOURCE, W_MSG, MPI_COMM_WORLD, reqw);
        MPI_Waitall (3,reqsz,statsz);
        //ct++;
        //printf("%d: send %d\n",myrank,ct);
      }

    }// if element > -1
  }// else of myrank == 0

  // finalizing...
  printf ("%d: finalizing...",myrank);

  //free solution(s)
  if ( weight > -1 )  free_list (&solution);

  //free tree
  node_t *t;
  for( ; cnt > 0 ; cnt-- ){
    t = root;
    root = root->lnode;
    free_node (t);
  }
  free_tree (root);

  free_task(&mytask);
  free(msgreq);
  free(msgstat);

  puts("ok"); fflush (stdout);
  // mpi tails
  MPI_Finalize ();
  return 0;
} // main()

node_t* receive_brother(int from, node_t *root, int *cnt, MPI_Status *stat){
  int size;
  node_t *head = createnodes(2), *thead = head+1;
  MPI_Recv(&(thead->length), 1, MPI_INT, from, SIZE_MSG, MPI_COMM_WORLD, stat);
  if ( thead->length > 0 ) {
    knint *p = (knint*)malloc(thead->length*KNINT_SIZE), *w = (knint*)malloc(thead->length*KNINT_SIZE), *pp *ww;
    MPI_Recv(p, thead->length, MPI_KNINT, from, P_MSG, MPI_COMM_WORLD, stat);
    MPI_Recv(w, thead->length, MPI_KNINT, from, W_MSG, MPI_COMM_WORLD, stat);
    thead->items = NULL;
    item_t *it = createitems(1);
    for ( pp = p, ww = w ; pp < p + thead->length ; pp++, ww++ ) {
      *(it->p) = *pp;
      *(it->w) = *ww;
      HASH_ADD_KEYPTR (hh, thead->items, it->w, KNINT_SIZE, it);
      it = copyitem(it);
    }
    thead->source = from;
    free(p); free(w);
  } else {
    thead->source = -1;
  }

  (*cnt)++;

  head->lnode = root;
  head->rnode = thead;

  return head;
}

/*
    return all of the sets of elements, whose choice lend to "weight" weight
     return list of some amount of arrays, not hashes.
*/
head_list_t* reconstruction(node_t *root, knint weight){
  head_list_t *rez = createlisthead(), *thead;
  item_t *elem;
  HASH_FIND (hh, root->items, &weight, KNINT_SIZE, elem);
  knint elem_p = *(elem->p), elem_w = *(elem->w);

  MPI_Request r,rp,rw, *req=&r, *reqp=&rp, *reqw=&rw;
  MPI_Status s,s2, *stat = &s,*stat2 = &s2;

  // if root is not leaf
  if ( root->lnode != 0 && root->rnode != 0 ){

    knint *lp, *lw, *rp, *rw;

    item_t *ritems = root->rnode->items, *litems = root->lnode->items, *tmp, *tm2;
    int      rsize = root->rnode->length,  lsize = root->lnode->length;

    int lelem, relem;

    HASH_FIND (hh, root->lnode->items, &elem_w, KNINT_SIZE, tmp);
    if ( tmp != NULL && *(tmp->p) == elem_p ){
      free (rez);
      rez = reconstruction (root->lnode, &elem_w);
    }

    HASH_FIND (hh, root->rnode->items, &elem_w, KNINT_SIZE, tmp);
    if ( tmp != NULL && *(tmp->p) == elem_p ) {
      thead = reconstruction (root->rnode, &elem_w);
      addlist ( rez, thead );
      thead->next = NULL;
      free_list (&thead);
    }

    if ( (lsize != -1) && (rsize != -1) ) {
      head_list_t *lhead, *rhead;
      for ( tmp = root->lnode->items ; (*(tmp->w) <= elem_w) && tmp != NULL ; tmp = tmp->hh.next ) {
        for ( tm2 = root->rnode->items ; ( (*(tmp->w)+*(tm2->w) <= elem_w) && tm2 != NULL ; tm2 = tm2->hh.next ) {
          if ( ( *(tmp->w) + *(tm2->w) == elem_w ) && ( *(tmp->p) + *(tm2->p)) == elem_p ) ) {
            lhead = reconstruction (root->lnode, *(tmp->w));
            rhead = reconstruction (root->rnode, *(tm2->w));
            thead = cartesian ( lhead, rhead );
            //print_list(lhead);
            //print_list(rhead);
            //print_list(cart);fflush(stdout);
            addlist ( rez, thead );
            thead->next = 0;
            free_list ( &thead );
            free_list ( &lhead );
            free_list ( &rhead );
          }
        } // for rnode
      } // for lnode
    }// if
  } else if ( root->lnode == NULL && root->rnode == NULL ){
    // if we get this node from other process
    if ( root->source > -1 ) {
      MPI_Isend (&elem_w, 1, MPI_KNINT, root->source, RECONSTR_MSG, MPI_COMM_WORLD, req);
      node_list_t *node;
      int rezcount;
      MPI_Recv (&rezcount, 1, MPI_INT, root->source, SOL_SIZE_MSG, MPI_COMM_WORLD, stat);
      int i;
      for ( i = 0 ; i < rezcount ; i++ ){
        node = createlistnode();
        MPI_Recv (&(node->length), 1, MPI_INT, root->source, SIZE_MSG, MPI_COMM_WORLD, stat);
        item_t* items = createitems (node->length);
        MPI_Irecv (items->p, node->length, MPI_KNINT, root->source, P_MSG, MPI_COMM_WORLD, reqp);
        MPI_Irecv (items->w, node->length, MPI_KNINT, root->source, W_MSG, MPI_COMM_WORLD, reqw);
        MPI_Wait(reqp,stat);
        MPI_Wait(reqw,stat);
        node->items = items;
        addnode ( rez, node );
      }
      printf("solution from %d received\n",root->source); fflush(stdout);
    } else { // if we reach leaf
      additems ( rez, 1, copyitem(elem) );
    }
  } else {
    puts("reconstruction(): one child null and other not null :/");
  }
  //puts("-"); print_tree(root); puts("+"); print_list(rez); fflush(stdout);
  return rez;
}

/*
    read the task from "filename" file and divide it within group of "groupsize" size
*/
task_t* readnspread_task(char* filename, int *groupsize){
    task_t *mytask;
    MPI_Request *reqs = (MPI_Request*)malloc((*groupsize)*sizeof(MPI_Request));
    MPI_Status *stats = (MPI_Status*)malloc((*groupsize)*sizeof(MPI_Status));

    FILE *file;
    if( (file = fopen(filename,"r")) == 0 ) return 0;

    int i;
    knint b;
  // send b
    if( fscanf(file,"%ld",&b) != 1 ) return 0;
    MPI_Bcast (&b, 1, MPI_KNINT, 0, MPI_COMM_WORLD);
    //MPI_Waitall ((*groupsize)-1,reqs+1,stats+1);

  // send size
    int size;
    if( fscanf(file,"%d",&size) != 1 ) return 0;
    int oldgroup = *groupsize;
    if( size < *groupsize ) *groupsize = size / 2;
    int onesize = size / *groupsize, rest, add, *sizes = (int*)malloc((*groupsize)*sizeof(int)), *psz;

    rest = size % *groupsize; // residue elements
    add = (rest>0)?1:0; // flag: add 1 to size of elements to current task while residue isn't 0
    for( i = 1, psz = sizes ; i < *groupsize ; i++, psz++ ){
      *psz = onesize + add;
      MPI_Isend (psz, 1, MPI_INT, i, SIZE_MSG, MPI_COMM_WORLD, reqs+i);
      rest--;
      add = (rest>0)?1:0;
    }
    free(sizes);

  // send p
    int onesizebyte = (onesize+1)*KNINT_SIZE;
    knint *p = (knint*)malloc(4*onesizebyte), *p2=p+onesize+1, *w=p2+onesize+1, *w2=w+onesize+1, *tmp, *tmp2;

      rest = size % *groupsize;
      add = (rest>0)?1:0;
      for( tmp = p ; tmp < p+onesize+add ; tmp++ )
        { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }
      MPI_Isend (p, onesize+add, MPI_KNINT, 1, P_MSG, MPI_COMM_WORLD, reqs);
      rest--;
      add = (rest>0)?1:0;

      for( i=2 ; i < *groupsize ; i++ ){
        for( tmp = p2 ; tmp < p2+onesize+add ; tmp++ )
          { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }
        MPI_Wait(reqs, stats);
        memcpy (p,p2,onesizebyte);
        MPI_Isend (p, onesize+add, MPI_KNINT, i, P_MSG, MPI_COMM_WORLD, reqs);
        rest--;
        add = (rest>0)?1:0;
      }
      for( tmp = p2 ; tmp < p2+onesize+add ; tmp++ )
          { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }

      // mytask
      mytask = createtask(onesize,b);
      memcpy (mytask->items->p,p2,mytask->length*KNINT_SIZE);

    // send w
      rest = size % *groupsize;
      add = (rest>0)?1:0;
      for( tmp = w ; tmp < w+onesize+add ; tmp++ )
        { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }
      MPI_Isend (w, onesize+add, MPI_KNINT, 1, W_MSG, MPI_COMM_WORLD, reqs);
      rest--;
      add = (rest>0)?1:0;

      for( i=2 ; i < *groupsize ; i++ ){
        for( tmp = w2 ; tmp < w2+onesize+add ; tmp++ )
          { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }
        MPI_Wait(reqs, stats);
        memcpy (w,w2,onesizebyte);
        MPI_Isend (w, onesize+add, MPI_KNINT, i, W_MSG, MPI_COMM_WORLD, reqs);
        rest--;
        add = (rest>0)?1:0;
      }

      for( tmp = w2 ; tmp < w2+onesize+add ; tmp++ )
          { if( fscanf (file,"%ld", tmp) != 1 ) return 0; }

      memcpy (mytask->items->w,w2,mytask->length*KNINT_SIZE);

  // close unnecessary workers
    rest = -1;
    for( i=*groupsize ; i < oldgroup ; i++ ){
      MPI_Isend (&rest,1,MPI_INT, i, SIZE_MSG, MPI_COMM_WORLD, reqs+i);
    }

    fclose(file);
    free (reqs);
    free (p);
    return mytask;
} // readnspread_task()

int value_sort (item_t *a, item_t *b) {
  if ( *(a->p) < *(b->p) ) return (int) -1;
  if ( *(a->p) > *(b->p) ) return (int) 1;
  return 0;
}
