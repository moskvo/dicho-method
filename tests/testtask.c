#include "task.h"


int main(int argc, char** argv){
  if (argc < 2) { puts("not enough arguments"); return 1; }
  task_t *task;
  if( (task = readtask(argv[1])) == 0){ puts("Read file error"); return; }
  item_t *items = task->items;
  
  puts("Task size:");
  printf("%d\n",task->length);
  
  int *d;
  puts("First 20 items ...");
  print_items(20,items);
  
  printf("Task bound: %ld\n",task->b);
  
  free_task(&task);
  puts("Task freed.\n Bye!");
}