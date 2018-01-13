# Project 4. MIPS Data Cache Simulator
Skeleton developed by CMU and KAIST,
modified for AJOU SCE212

## Instructions
You may modify any c and header files including `sce212sim.c`, `util.c` , `cache.c`, and `cache.h` but please try to write your code for caches within cache.c and cache.h. 
In order to build your code you need to have `run.c` and `run.h` which you have written for Project3.
You are banned to modify files that are related to grading( files under grading\_input and grading\_output ). 

### 1. sce212sim.c 
i
This file contains the main/essential functions of the simulator.
The function for preparing your cache, `setupCache()`, is called from the main function.
If you decided to modify the parameters of `setupCache()`, be sure to also modify the corresponding part in this file.

### 2. util.h

Several additional printing functions have been added to `util.h` from Project3.
If you want to declare and implement additional functions, feel free to do so.

### 3. util.c

This file contains the implementation of additional functions such as `xdump()` and `cdump()`
If you want to modify the functions given in `util.c`, you are free to do so.
However, make sure your modified function prints the exactly same format as the reference simulator!

### 4. cache.h

This file contains `setupCache()` and `setCacheMissPenalty()`
Please declare additional functions that are required by your data cache, in this file.

### 5. cache.c 

This file contains the basic code for `setupCache()` and `setCacheMissPenalty()
(Feel free to modify the code if you need to)
You must implement additional function for your cache in this file.
