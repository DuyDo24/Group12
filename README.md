## Assignment 2: Parallel Merge Sort

* Authors: Duy Do, Zixi Khoo, Aidan Chan
* Group name: 12

## Overview

This program sorts a large array of randomly generated integers using a multi-threaded (parallel) merge sort built on the pthread library. The array is split in half repeatedly, with a new thread handling each half, until a user-specified cutoff level is reached; below that level each thread sorts its subarray with an ordinary single-threaded recursive merge sort. Parent threads then join their children and merge the sorted halves back together.

## Manifest

* `mergesort.c` - our implementation of `merge()`, `my_mergesort()`, `buildArgs()` and `parallel_mergesort()`.
* `mergesort.h` - provided header with the function prototypes, the `struct argument` definition and the shared globals `A`, `B` and `cutoff` (unmodified).
* `test-mergesort.c` - provided test program that generates a random array, calls our sort, times it and checks the result is sorted (unmodified).
* `Makefile` - builds the `test-mergesort` executable.
* `README.md` - the project description.

## Building the project

From the project directory run:

    make

This compiles `mergesort.c` and `test-mergesort.c` with gcc and links against the pthread library, producing the `test-mergesort` executable. Run `make clean` to remove the object files and executable.

`test-mergesort.c` includes `<error.h>`, which exists on Linux but not on macOS. The project builds as-is on Linux (and Gradescope); to build on a Mac, temporarily comment out that include line.

## Features and usage

    ./test-mergesort <input size> <cutoff level> <seed>

* `input size`: number of elements to sort (must be at least 2).
* `cutoff level`: how many levels of the thread tree to create. `0` runs the plain single-threaded merge sort; a cutoff of `c` produces `2^c` leaf threads that each sort one part of the array.
* `seed`: random seed used to generate the array; the same seed always gives the same input.

Example:

    $ ./test-mergesort 100000000 4 1234
    Sorting 100000000 elements took 0.98 seconds.

How it works:

* `buildArgs()` allocates a `struct argument` holding the `left` and `right` indices and the current `level`. This is needed because a thread's start routine can only take a single `void *` argument.
* `parallel_mergesort()` checks whether the current level has reached the cutoff. If it has, it calls `my_mergesort()` on its range. Otherwise it builds arguments for the left and right halves, creates a thread for each with `pthread_create()`, waits for both with `pthread_join()`, frees the two argument structs and merges the halves with `merge()`.
* `my_mergesort()` is a standard recursive merge sort, and `merge()` merges two adjacent sorted ranges of `A` into `B`, then copies the result back into `A`.

Concurrency and safety:

* No mutexes are needed. Sibling threads always work on separate, non-overlapping index ranges of `A` and `B`, so no two threads ever touch the same element at the same time.
* The only synchronisation required is that a parent calls `pthread_join()` on both children before it calls `merge()`, so it never merges halves that are not yet sorted.
* Each parent frees its children's argument structs after joining them; the top-level struct is freed by the test program.
* Errors are handled without losing correctness. If `buildArgs()` fails to allocate memory, or `pthread_create()` fails (for example, the OS refuses to create more threads), the current thread sorts that range itself with `my_mergesort()` instead, and only threads that were actually created are joined.
* The midpoint is computed as `left + (right - left) / 2` to avoid integer overflow on very large arrays.

## Testing

All testing used the provided `test-mergesort` program, which reports `sorting failed!!!!` if the output array is not in non-decreasing order.

In testing, we ran every combination of:

* array sizes: 2, 3, 4, 5, 17, 100, 999, 1024, 65537 and 1000003 (including the minimum size, odd sizes, powers of two and sizes just above them)
* cutoff levels: 0, 1, 2, 3, 4, 5, 6, 8 and 10 (including cutoffs larger than the array size needs)
* seeds: 1, 42, 1234 and 99999

All 360 runs sorted correctly.

**Thread-creation failure**

 Running `./test-mergesort 1000000 16 1234` asks for tens of thousands of threads, more than the OS will allow, so some `pthread_create()` calls fail. Before we added error handling this produced `sorting failed!!!!`; with the fallback to `my_mergesort()` it now sorts correctly (slowly, because of the thread-creation overhead).

Before (ignored the return value of `pthread_create()`):

```c
pthread_create(&leftThread, NULL, parallel_mergesort, leftArg);
pthread_create(&rightThread, NULL, parallel_mergesort, rightArg);

pthread_join(leftThread, NULL);
pthread_join(rightThread, NULL);
```

If a create failed, that half of the array was never sorted, and `pthread_join()` was called on a thread that did not exist (undefined behaviour).

After (falls back to sorting in the current thread, and only joins threads that were created):

```c
/* create a thread for each half then if creation fails then sort that half in this thread instead */
int leftCreated = (pthread_create(&leftThread, NULL, parallel_mergesort, leftArg) == 0);
if (!leftCreated)
    my_mergesort(left, mid);

int rightCreated = (pthread_create(&rightThread, NULL, parallel_mergesort, rightArg) == 0);
if (!rightCreated)
    my_mergesort(mid + 1, right);

if (leftCreated)
    pthread_join(leftThread, NULL);
if (rightCreated)
    pthread_join(rightThread, NULL);
```

| Test (1,000,000 elements, cutoff 16) | Result |
|---|---|
| Before | `sorting failed!!!!` |
| After | sorted correctly |

**Memory.** 

We checked for leaks with the macOS `leaks` tool at cutoffs 0, 3 and 8 on 100,000 elements, and found 0 leaks in every case.

**Performance.**

Sorting n = 100,000,000 elements with seed 1234 on an Apple Macbook Air M4 (10 cores):

| Cutoff | Time (s) | Speedup vs cutoff 0 |
|-------:|---------:|--------------------:|
| 0      | 5.18     | 1.00x               |
| 1      | 2.82     | 1.84x               |
| 2      | 1.63     | 3.18x               |
| 3      | 1.09     | 4.75x               |
| 4      | 0.98     | 5.29x               |
| 5      | 0.98     | 5.29x               |
| 6      | 0.98     | 5.29x               |
| 7      | 0.98     | 5.29x               |
| 8      | 0.98     | 5.29x               |

This is well above the required speedup of 2. The gain levels off from cutoff 4, which gives 16 leaf threads, more than the machine's 10 cores. Beyond that point extra threads cannot run at the same time, and the final merges (the top-level one runs on a single thread) dominate the running time.

## Known Bugs

No known bugs found. Very high cutoffs (around 16 and above) still sort correctly but are slower than low cutoffs, because creating tens of thousands of threads costs more time than it saves.

## Reflection and Self Assessment

<!--Discuss the issues you encountered during development and testing. What
problems did you have? What did you have to research and learn on your own?
What kinds of errors did you get? How did you fix them?

What parts of the project did you find challenging? Is there anything that
finally "clicked" for you in the process of working on this project? How well
did the development and testing process go for you?-->

### Aidan Chan

My main contribution was making `parallel_mergesort()` robust and tidying the code for submission. Our first version sorted correctly in normal tests, but when I reviewed it I found it never checked whether `buildArgs()` or `pthread_create()` succeeded, therefore it was missing NULL checks. Thus, I added three checks, including return early if the argument pointer is `NULL`, fall back to `my_mergesort()` if either `malloc()` in `buildArgs()` fails, and fall back to sorting a half in the current thread if `pthread_create()` fails, only joining threads that were actually created. When testing with 1,000,000 elements and cutoff 16, the OS refuses to create some of the tens of thousands of threads requested, and our original code printed `sorting failed!!!!`. After the fix it sorts correctly.

I was also responsible for writing most of the README, which required a fundamental understanding of all the code written. 

I made several mistakes while adding the `pthread_create()` check:

* I first named the success flags `left` and `right`. These clashed with the existing `left` and `right` index variables, so the compiler reported `redefinition of 'left'`. I learned that C does not let you redeclare a variable in the same scope, and that reusing a name would have overwritten the array indices even if it had compiled.
* After renaming the flags to `leftCreated` and `rightCreated`, I missed the two `if` statements guarding `pthread_join()`, which still tested `left` and `right`. This compiled without warnings but sorting failed for most inputs. Because `left` is 0 for the leftmost child at every level, the parent skipped joining it, merged before the child had finished sorting, and freed the child's argument while it might still be in use. This was an example of a race condition, and it showed me why `pthread_join()` must happen before `merge()`, and that a program can compile cleanly and still be wrong.

Overall, testing went well once the bugs were fixed. All 360 correctness runs passed, there were no memory leaks, and we got about a 5.3x speedup on 100 million elements. The biggest lesson for me was that concurrency bugs often do not crash the program, they just produce wrong results. Therefore, testing with many sizes, cutoffs and seeds is essential.




## Sources Used

GeeksforGeeks (2026) Merge Sort, GeeksforGeeks, accessed 26 October 2026. https://www.geeksforgeeks.org/dsa/merge-sort/

GeeksforGeeks (2024) Merge Sort using Multi-threading, GeeksforGeeks, accessed 26 October 2026. https://www.geeksforgeeks.org/dsa/merge-sort-using-multi-threading/

