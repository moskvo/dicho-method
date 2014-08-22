#include "task.h"

int main (int argc, char** argv) {
  item_t *hash = NULL, *hash1 = NULL, *p1, *p2, *tmp;
  p1 = createitems(3);
  int i,j;
  for( p2=p1, i=0,j=10 ; i<3 ; i++, j--, p2++ ) {
    *(p2->p) = i;
    *(p2->w) = j;
    tmp = copyitem(p2);
    HASH_ADD_KEYPTR (hh,hash, tmp->w, KNINT_SIZE, tmp );
    tmp = copyitem (tmp);
    *(tmp->w) += i;
    (*(tmp->p))++;
    HASH_ADD_KEYPTR (hh,hash1, tmp->w, KNINT_SIZE, tmp);
  }
  item_t *hash2 = copyhash(hash1);
  for ( tmp = hash2 ; tmp != NULL ; tmp = tmp->hh.next ) {
    *(tmp->w) *= *(tmp->p);
  }
  tmp = createitems(1);;
  *(tmp->p) = 11;
  *(tmp->w) = 23;
  HASH_ADD_KEYPTR (hh, hash2, tmp->w, KNINT_SIZE, tmp);

  puts("hashes builded"); fflush(stdout);
  print_hash (hash);
  print_hash (hash1);
  print_hash (hash2);

  free (p1);
  free_hash (&hash);
  free_hash (&hash1);
  free_hash (&hash2);
}
