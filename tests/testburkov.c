#include "burkov.h"

int main(int argc, char** argv){
  if ( argc < 2 ) return 1;
  task_t *task;
  if ( (task = readtask(argv[1])) == 0) { puts("Read file error"); return; }
  item_t *items = task->items;

  puts("Task size:");
  printf("%u\n",task->length);


  node_t* root;
  if( (root = burkovtree ( task )) == NULL )
  { puts("Can't build optdichotree"); }
  else {

  print_tree(root);

  treesolver (root,task->b);
  //print_tree(root);
  print_hash (root->items);
  //printf("(%d %d)\n",root->items->p[root->length-1],root->items->w[root->length-1]);

  free(root); // not free_tree, because root -> array of all nodes (see node.c: optimal_dichotomic_tree() )
  }
  free_task(&task);
  puts("All freed.\n Bye!");
  

}
