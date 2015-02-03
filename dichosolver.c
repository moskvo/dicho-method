#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define NO_DEBUG 0
#define LOW_DEBUG 1
#define MID_DEBUG 2
#define HI_DEBUG 3

#define DBGLVL HI_DEBUG

#include "dichosolver.h"

#define SIZE_MSG 100
#define P_MSG 101
#define W_MSG 102
#define B_MSG 103
#define RECONSTR_MSG 200
#define SOL_SIZE_MSG 201
#define GETCHANNEL_MSG 202
#define BRANCH_MSG_1 203
#define BRANCH_MSG_2 204
#define BRANCH_MSG_3 205

#define WAITFOR 2

#if defined(KNINT_INT)
#define MPI_KNINT MPI_INT
#elif defined(KNINT_LONG)
#define MPI_KNINT MPI_LONG
#endif

int main(int argc, char** argv){
  // mpi headers
  MPI_Init (&argc, &argv);
  int groupsize, myrank;
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

  node_t *root = NULL;

/* get task code for process zero */
if (myrank == 0)
{
  mytask = readnspread_task_parallel (argv[1], &groupsize);
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

    #if DBGLVL >= LOW_DEBUG
    printf("%d readed. b=%ld, size=%d.\n",myrank,mytask->b,mytask->length); fflush(stdout);
    #endif

    #if DBGLVL >= LOW_DEBUG
    puts("build tree..."); fflush(stdout);
    #endif
    //{ solve mytask
    if( (root = burkovtree ( mytask )) == 0 )
    { puts("Can't build optdichotree"); fflush(stdout); }
    
    #if DBGLVL >= HI_DEBUG
    print_tree (root); fflush(stdout);
    #endif

    #if DBGLVL >= LOW_DEBUG
    puts("solve local task..."); fflush(stdout);
    #endif
    treesolver (root,mytask->b);

    #if DBGLVL >= LOW_DEBUG
      printf("%d: local task solved. Solving parallel...\n",myrank); fflush(stdout);
    #endif
    //}

    #if DBGLVL >= HI_DEBUG
      print_tree(root); fflush(stdout);
    #endif

}// else

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

      #if DBGLVL >= HI_DEBUG
        if ( myrank == 0 ) { 
        	puts("-"); 
        	if ( root->rnode != NULL && root->lnode != NULL && root->rnode->length > 0 && root->lnode->length > 0 ) {
        		printf ("root length = %d, received length = %d\n", root->rnode->length, root->lnode->length);
        	}
        	fflush(stdout);
        }
      #endif
      // solve them all
      #if DBGLVL >= MID_DEBUG
        printf("process %d are solving received tree..\n", myrank); fflush(stdout);
      #endif
      treesolver (root, mytask->b);
      #if DBGLVL >= MID_DEBUG
        printf("process %d solved received tree.\n", myrank); fflush(stdout);
      #endif
    }
  }

  // if process zero is ended previous code then complete solution is obtained, print it
  if ( myrank == 0 ) {
    if ( root->length == -1 ) { puts("length == -1"); fflush(stdout); }
    else {
      if ( root->items == NULL ) puts ("Wwarning! There's no items!");
      if ( root->items->p == NULL ) puts("Wwarning! There's no solutions!");
      // sorting
      #if DBGLVL >= MID_DEBUG
        puts("sorting..."); fflush(stdout);
      #endif
      HASH_SORT ( root->items, value_sort );
      // print it
      printf ( "knapsack: (p=%ld w=%ld)\n", *(root->items->p), *(root->items->w) ); //fflush(stdout);
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
      item_t *fp;
      for ( fp = root->items, pp = p, pw = w ; fp != NULL ; fp = fp->hh.next, pp++, pw++ ){
        *pp = *(fp->p);
        *pw = *(fp->w);
      }
      // send p and w
      MPI_Isend( p, root->length, MPI_KNINT, to, P_MSG, MPI_COMM_WORLD, reqp);
      //printf("%d send: ",myrank); print_items(root->length, root->items);
      MPI_Isend( w, root->length, MPI_KNINT, to, W_MSG, MPI_COMM_WORLD, reqw);
      MPI_Waitall(2,reqp,statp);
      free(p); free(w);
    }
    MPI_Wait(reqsz,statsz);

  } // else of if myrank == 0

  // we get the optimal knapsack value and weight,
  //  now we must reconstruct elements of it. All sets of elements that leading to optimal knapsack
  #if DBGLVL >=LOW_DEBUG
    printf("%d: solution reconstruction\n",myrank); fflush(stdout);
  #endif

  int size;
  knint weight;
  // process zero run the reconstruction
  if ( myrank == 0 ){
    //element = root->length-1;
    // reconstruction() return head to list of all sets of solutions.
    #if DBGLVL >= LOW_DEBUG
      printf("%d:size of top node: %d\n",myrank,root->length);
    #endif
    
    // run solve
    bud_t *branch = createbud0 ();
    reconstruction ( root, *(root->items->w), 0, branch);
    free (branch);
    
    
    // receive all things
    solnode_t *soltree = createsolnode0();
    int count, flag = 0;
    knint *pair_pw = (knint*) malloc (2 * KNINT_SIZE);
    #if DBGLVL >= MID_DEBUG
    	printf("master waiting for solution items\n"); fflush(stdout);
    #endif
    MPI_Probe ( MPI_ANY_SOURCE, BRANCH_MSG_1, MPI_COMM_WORLD, statb );
    do {
	MPI_Recv (&count, 1, MPI_INT, statb->MPI_SOURCE, BRANCH_MSG_1, MPI_COMM_WORLD, statb);
        #if DBGLVL >= LOW_DEBUG
          printf("master receive answer from: %d\n",statb->MPI_SOURCE); fflush(stdout);
        #endif

    	int *branchpath = (int*) malloc (count*sizeof(int)), *br;
    	// branch sending as array (level1,branch1),(level2,branch2),... , where pair (l,b) is point of fork in tree of alternative solutions
    	MPI_Recv (branchpath, count, MPI_INT, statb->MPI_SOURCE, BRANCH_MSG_2, MPI_COMM_WORLD, statw);
    	MPI_Recv (pair_pw, 2, MPI_KNINT, statb->MPI_SOURCE, BRANCH_MSG_3, MPI_COMM_WORLD, statw);
        #if DBGLVL >= MID_DEBUG
          printf("master receive item (%ld,%ld).\n",*pair_pw,*(pair_pw+1)); fflush(stdout);
        #endif
    	
    	solnode_t *node = soltree;
    	// find or insert in tree the branch which fit branchpath
    	  for ( br = branchpath ; br < branchpath+count ; br = br+2) {
		solnode_t *elem;
		HASH_FIND (hh, node->childs, br+1, sizeof(int), elem);
		if ( elem == NULL ) {
			node = addsolchild (node, *br, *(br+1));
		} else {
			node = elem;
		}
	  }
    	free (branchpath);

	// add item pair;
        #if DBGLVL >= MID_DEBUG
          printf("add item pair\n"); fflush(stdout);
        #endif
    	addsolitem (node, pair_pw, pair_pw+1);
    	
    	int patience = -1;
    	flag = 0;
        #if DBGLVL >= MID_DEBUG
          printf("master waiting for solution items\n"); fflush(stdout);
        #endif
	MPI_Iprobe ( MPI_ANY_SOURCE, BRANCH_MSG_1, MPI_COMM_WORLD, &flag, statb );
        #if DBGLVL >= MID_DEBUG
          char s[MPI_MAX_ERROR_STRING];
          int len;
          MPI_Error_string(statb->MPI_ERROR,s,&len);
          printf("master receive error: %s\n", s); fflush(stdout);
        #endif
  	while ( flag == 0 && patience < WAITFOR ) {
    		sleep(1);
		MPI_Iprobe ( MPI_ANY_SOURCE, BRANCH_MSG_1, MPI_COMM_WORLD, &flag, statb );
    		patience++;
    	} 
    	
    } while ( flag != 0 );
    free (pair_pw);
    
    // print solutions
    #if DBGLVL >= LOW_DEBUG
        printf ("Master are printing solutions\n"); fflush(stdout);
    #endif
       print_solutions (soltree); fflush (stdout);
       
       free_solnodes (soltree);


    // what is it? like quit signal
    //quit_signal();
    int i;
    knint *killsig = (knint*) malloc(groupsize*KNINT_SIZE);
    for( i = 0 ; i < groupsize ; i++ ){
      *(killsig+i) = -1;
      MPI_Isend ((killsig+i), 1, MPI_KNINT, i, RECONSTR_MSG, MPI_COMM_WORLD, reqsz);
    }
    free (killsig);
    
  }
  // other processes are waiting for reconstruction signal
  else {
    // get seeked weight
    MPI_Recv (&weight, 1, MPI_KNINT, MPI_ANY_SOURCE, RECONSTR_MSG, MPI_COMM_WORLD, statb);
    if( weight > -1 ){
      #if DBGLVL >= LOW_DEBUG
        printf ("%d: i receive 'reconstruction' message for weight: %ld\n",myrank,weight); fflush(stdout);
      #endif

      int branch_cnt, *p, level; 

      // get level
      MPI_Recv (&level, 1, MPI_INT, MPI_ANY_SOURCE, RECONSTR_MSG, MPI_COMM_WORLD, statb);
      
      // get branch
      MPI_Recv (&branch_cnt, 1, MPI_INT, MPI_ANY_SOURCE, RECONSTR_MSG, MPI_COMM_WORLD, statb);
      p = (int*) malloc (branch_cnt*sizeof(int));
      MPI_Recv (p, branch_cnt, MPI_INT, MPI_ANY_SOURCE, RECONSTR_MSG, MPI_COMM_WORLD, statb);

      bud_t *branch = createbud (branch_cnt, p);

      #if DBGLVL >= LOW_DEBUG
        printf ("%d: i run reconstruction\n",myrank); fflush(stdout);
      #endif
      reconstruction(root, weight, level, branch);

    }// if element > -1
  }// else of myrank == 0

  #if DBGLVL >= LOW_DEBUG
  printf ("%d: finalizing...\n",myrank); fflush (stdout);
  #endif

  #if DBGLVL >= LOW_DEBUG
  puts ("free tree"); fflush(stdout);
  #endif
  node_t *t;
  for( ; cnt > 0 ; cnt-- ){
    t = root;
    root = root->lnode;
    free_node (t);
  }
  free (root);//free_tree (root);

  #if DBGLVL >= LOW_DEBUG
    puts ("free some var"); fflush(stdout);
  #endif
  free_task(&mytask);
  free(msgreq);
  free(msgstat);

  #if DBGLVL >= LOW_DEBUG
  	printf("%d: ok\n", myrank); fflush (stdout);
  #endif
  // mpi tails
  MPI_Finalize ();
  return 0;
} // main()

node_t* receive_brother(int from, node_t *root, int *cnt, MPI_Status *stat){
  node_t *head = (root == NULL) ? createnodes(1) : createnodes(2), *thead = (root==NULL)?(head):(head+1);

  MPI_Recv(&(thead->length), 1, MPI_INT, from, SIZE_MSG, MPI_COMM_WORLD, stat);

  if ( thead->length > 0 ) {
    knint *p = (knint*)malloc(thead->length*KNINT_SIZE), *w = (knint*)malloc(thead->length*KNINT_SIZE), *pp, *ww;
    MPI_Recv(p, thead->length, MPI_KNINT, from, P_MSG, MPI_COMM_WORLD, stat);
    MPI_Recv(w, thead->length, MPI_KNINT, from, W_MSG, MPI_COMM_WORLD, stat);
    thead->items = NULL;
    item_t *it = createitems(1);
    for ( pp = p, ww = w ; pp < p + thead->length ; pp++, ww++ ) {
      *(it->p) = *pp;
      *(it->w) = *ww;
      HASH_ADD_KEYPTR (hh, thead->items, it->w, KNINT_SIZE, it);
      it = copyitem (it);
    }
    thead->source = from;
    free(p); free(w);
  } else {
    thead->source = -1;
  }

  (*cnt)++;

  if ( root != NULL ) {
      head->lnode = root;
      head->rnode = thead;
      root->hnode = head;
      thead->hnode = head;
  }

  return head;
}

/*
    return all of the sets of elements, whose choice lend to "weight" weight
     return list of some amount of arrays, not hashes.
*/
void reconstruction(node_t *root, knint weight, int level, bud_t* branch){
  int branch_num = 1;
  // elem = element on top of root with 'weight'=weight
  item_t *elem;
  HASH_FIND (hh, root->items, &weight, KNINT_SIZE, elem);
  knint elem_p = *(elem->p), elem_w = *(elem->w);

  MPI_Request r,rp,rw, *req=&r, *reqp=&rp, *reqw=&rw;
  MPI_Status s,s2, *stat = &s,*stat2 = &s2;

  // if root is not leaf
  if ( root->lnode != NULL && root->rnode != NULL ){

    knint *lp, *lw, *rp, *rw;

    item_t *tmp, *tm2;
    int rsize = root->rnode->length,  lsize = root->lnode->length;

    // look for simple match in left subtree
    HASH_FIND (hh, root->lnode->items, &elem_w, KNINT_SIZE, tmp);
    if ( tmp != NULL && *(tmp->p) == elem_p ){
      //free (rez);
      //rez = reconstruction (root->lnode, elem_w, rank);
      reconstruction (root->lnode, elem_w, level, branch);
      branch_num++;
    }

    // look for simple match in right subtree
    HASH_FIND (hh, root->rnode->items, &elem_w, KNINT_SIZE, tmp);
    if ( tmp != NULL && *(tmp->p) == elem_p ) {
      if ( branch_num > 1 ) {
      	reconstruction (root->rnode, elem_w, level, budoff(branch,level,branch_num));
      } else {
      	reconstruction (root->rnode, elem_w, level, branch);      	      
      }
      branch_num++;
    }

    // look for total match
    if ( (lsize != -1) && (rsize != -1) ) {
      head_list_t *lhead = createlisthead(), *rhead = createlisthead();
      for ( tmp = root->lnode->items ; /*(*(tmp->w) <= elem_w) && cause not sorted! */tmp != NULL ; tmp = tmp->hh.next ) {
        for ( tm2 = root->rnode->items ; /*(*(tmp->w)+*(tm2->w) <= elem_w) && not sorted! */tm2 != NULL ; tm2 = tm2->hh.next ) {
          if ( (*(tmp->w) + *(tm2->w) == elem_w) && (*(tmp->p) + *(tm2->p)) == elem_p ) {
            bud_t *nb = ( branch_num < 2 ) ? branch : budoff(branch,level,branch_num);
            reconstruction (root->lnode, *(tmp->w), level+1, nb);
            reconstruction (root->rnode, *(tm2->w), level+1, nb);
            branch_num++;
          }
        } // for rnode
      } // for lnode
    }// if total match
  } 
  // if root is leaf
  else if ( root->lnode == NULL && root->rnode == NULL ){
    // if we get this node from other process
    if ( root->source > -1 ) {
        #if DBGLVL >= MID_DEBUG
          printf ("-> %d\n",root->source); fflush (stdout);
        #endif
        // send element
      	MPI_Ssend (&elem_w, 1, MPI_KNINT, root->source, RECONSTR_MSG, MPI_COMM_WORLD);
        int *p = (int*)malloc ((2*branch->count)*sizeof(int)), *t, *pt;
      	// save to p all branch
      	memcpy (p, branch->oldbranch, 2*branch->oldcount*sizeof(int));
      	branch_t *br;
      	pt = p + 2*branch->count - 1;
      	for ( br = branch->next ; br != NULL ; br = br->next ) {
      	  *pt = br->branch; pt--;
      	  *pt = br->level; pt--;
      	}
      	// send level      	
      	MPI_Ssend (&level, 1, MPI_INT, root->source, RECONSTR_MSG, MPI_COMM_WORLD);
      	// send branch
      	branch_num = 2*branch->count; // size of array p
      	MPI_Ssend (&branch_num, 1, MPI_INT, root->source, RECONSTR_MSG, MPI_COMM_WORLD);
      	MPI_Ssend (p, branch_num, MPI_INT, root->source, RECONSTR_MSG, MPI_COMM_WORLD);
      	free (p);      
    } else { // if we reach leaf then send branch to 0 process
        int *p = (int*)malloc ((2*branch->count)*sizeof(int)), *t, *pt;
        // save to p all branch
        memcpy (p, branch->oldbranch, 2*branch->oldcount*sizeof(int));
        branch_t *br;
        pt = p + 2*branch->count - 1;
        for ( br = branch->next ; br != NULL ; br = br->next ) {
          *pt = br->branch; pt--;
          *pt = br->level; pt--;
        }
        // add unique key
        int count = 2 * branch->count;
        // send branch
        MPI_Ssend (&count, 1, MPI_INT, 0, BRANCH_MSG_1, MPI_COMM_WORLD);
        MPI_Ssend (p, count, MPI_INT, 0, BRANCH_MSG_2, MPI_COMM_WORLD);
      
        // send element
        knint *e = (knint*) malloc (2*KNINT_SIZE);
        e[0] = *(elem->p);
        e[1] = *(elem->w);
        #if DBGLVL >= MID_DEBUG
          printf ("Send to master (%ld,%ld)\n",e[0],e[1]); fflush (stdout);
        #endif
        MPI_Ssend (e, 2, MPI_KNINT, 0, BRANCH_MSG_3, MPI_COMM_WORLD);
        free (p);
        free (e);
    }
  } else {
    puts("reconstruction(): one child null and other not null :/");
  }
  #if DBGLVL >= HI_DEBUG
    //puts("-"); print_tree(root); fflush(stdout);
  #endif
  return ;
}


/*
    read the task from "filename" file and parallel divide it within group of "groupsize" size
*/
task_t* readnspread_task_parallel(char* filename, int *groupsize){
    task_t *mytask;
    MPI_Request *reqs = (MPI_Request*)malloc((*groupsize)*sizeof(MPI_Request));
    MPI_Status *stats = (MPI_Status*)malloc((*groupsize)*sizeof(MPI_Status));

    FILE *file;
    if( (file = fopen(filename,"r")) == 0 ) return 0;

    int i;
    knint b;
  // puts("send b"); fflush(stdout);
    if( fscanf(file,"%ld",&b) != 1 ) return 0;
    MPI_Bcast (&b, 1, MPI_KNINT, 0, MPI_COMM_WORLD);
    //MPI_Waitall ((*groupsize)-1,reqs+1,stats+1);

  // puts("send size"); fflush(stdout);
    int size;
    if( fscanf(file,"%d",&size) != 1 ) return 0;
    // printf("Size=%d\n",size); fflush(stdout);
      // if groupsize > size then to half groupsize
      int oldgroup = *groupsize;
      if( size < *groupsize ) {  *groupsize = size / 2;  }
    int onesize = size / (*groupsize-1), rest,
        *sizes = (int*)malloc((*groupsize)*sizeof(int)), *psz;

    rest = (size % (*groupsize-1)); // residue elements
    // i(rank 0) am not in the group of solving base tasks
    for( i = 1, psz = sizes+1 ; i < *groupsize ; i++, psz++ ){
      (*psz) = onesize + (rest>0?1:0);
      MPI_Send (psz, 1, MPI_INT, i, SIZE_MSG, MPI_COMM_WORLD);
      rest--;
    }

  // puts("read p"); fflush(stdout);
    knint **ps = (knint**) malloc ((*groupsize)*sizeof(knint*)),
          **ws = (knint**) malloc ((*groupsize)*sizeof(knint*)), **tmps, **tmws;
    for ( i = 1,tmps=ps+1,tmws=ws+1 ; i < *groupsize ; i++,tmps++,tmws++ ) {
      *tmps = (knint*)malloc(sizes[i]*KNINT_SIZE);
      *tmws = (knint*)malloc(sizes[i]*KNINT_SIZE);
    }

      int proc = 1, num = 0;
      for( i = 0 ; i < size ; i++ ) {
        if( fscanf (file,"%ld", ps[proc]+num) != 1 ) return 0;
        proc++;
        if( proc == *groupsize ){
          proc = 1;
          num++;
        }
      }
  // puts("sending p"); fflush(stdout);
    knint *p;
      for( i = 1 ; i < *groupsize ; i++ ) {
        MPI_Isend (ps[i], sizes[i], MPI_KNINT, i, P_MSG, MPI_COMM_WORLD, reqs+i);
      }

  // puts("mytask"); fflush(stdout);
      mytask = createtask(0,b);

  // puts("read w"); fflush(stdout);
      proc = 1;
      num = 0;
      for( i = 0 ; i < size ; i++ ) {
        if( fscanf (file,"%ld", ws[proc]+num) != 1 ) return 0;
        proc++;
        if( proc == *groupsize ){
          proc = 1;
          num++;
        }
      }
  // sending w
      MPI_Waitall ((*groupsize)-1,reqs+1,stats+1);
      for( i = 1 ; i < *groupsize ; i++ ) {
        MPI_Isend (ws[i], sizes[i], MPI_KNINT, i, W_MSG, MPI_COMM_WORLD, reqs+i);
      }

  // puts("close unnecessary workers"); fflush(stdout);
    //MPI_Waitall ((*groupsize)-1,reqs+1,stats+1);
    rest = -1;
    for( i = *groupsize ; i < oldgroup ; i++ ){
      MPI_Isend (&rest,1,MPI_INT, i, SIZE_MSG, MPI_COMM_WORLD, reqs+i);
    }

    for( i = 1 ; i < *groupsize ; i++ ) { free(ps[i]); free(ws[i]); }
    free (ps); free (ws);

    fclose(file);
    free (reqs);
    free (stats);
    free (sizes);
    return mytask;
} // readnspread_task_parallel()

size_t BRANCH_SIZE = sizeof(branch_t);
size_t BUD_SIZE = sizeof(bud_t);

bud_t* createbud0 () {
  bud_t *bud = (bud_t*)malloc (BUD_SIZE);
  bud->count = bud->oldcount = 0;
  bud->next = 0;
  bud->oldbranch = 0;
  return bud;
}
bud_t* createbud (int oldc, int *oldb) {
  bud_t *bud = createbud0 ();
  bud->count = oldc;
  bud->oldbranch = oldb;
  bud->oldcount = oldc;
  return bud;
}
bud_t* lightcopybud (bud_t *bud) {
  bud_t *nb = createbud(bud->oldcount, bud->oldbranch);
  nb->count = bud->count;
  nb->next = bud->next;
  return nb;
}

bud_t* budoff(bud_t *bud, int level, int branch) {
  bud_t *newbud = lightcopybud (bud);
  grow (newbud, level, branch);
  return newbud;
}

void grow (bud_t* bud, int level, int branch) {
  branch_t *tbranch = (branch_t*)malloc (BRANCH_SIZE);
  tbranch->level = level;
  tbranch->branch = branch;

  tbranch->next = bud->next;
  bud->next = tbranch;
  bud->count++;
}

