#include <stdio.h>
#include <task.h>

int main(){
  head_list_t *head = createlisthead(), *tail;
  item_t *items = createitems(1);
  *(items->p) = 1;
  *(items->w) = 10;
  additems(head, 1, items);
  
  items = createitems(2);
  *(items->p) = 2;
  *(items->p+1) = 3;
  *(items->w) = 9;
  *(items->w+1) = 8;
  node_list_t *node = createlistnode();
  node->items = items;
  node->length = 2;
  addnode(head, node);
  
  tail = createlisthead();
  items = createitems(3);
  *(items->p) = 4;
  *(items->p+1) = 5;
  *(items->p+2) = 6;
  *(items->w) = 7;
  *(items->w+1) = 6;
  *(items->w+2) = 5;
  additems(tail,3,items);
  addlist(head,tail);
  
  printf("list size is %d\n",head->count);
  print_list(head);
  
  tail->next = 0;
  puts("free head"); fflush(stdout);
  free_list(&head);
  puts("free tail"); fflush(stdout);
  free_list(&tail);
  
}