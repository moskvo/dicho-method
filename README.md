dicho-method
============
The knapsack problem:  
1. values (p), weights (w) and the knapsack capacity (c) are nonnegative integers.  
2. the problem is to maximize sum(pi\*xi) while sum(wi\*xi)<=c, over xi belonging to {0,1}.

Command to compile:  
mpicc -o solver dichosolver.c burkov.c task.c -I./headers -lm

Command to run:  
srun -n90 solver knaproblem.txt

Format of input file:  
\<knapsack size\>  
\<number of items\>  
\<items' values\>  
\<items' weights\>  
