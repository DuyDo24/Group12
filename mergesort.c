/**
 * This file implements parallel mergesort.
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h> /* for memcpy */
#include <stdlib.h> /* for malloc */
#include "mergesort.h"

/* this function will be called by mergesort() and also by parallel_mergesort(). */
void merge(int leftstart, int leftend, int rightstart, int rightend) {
    int i = leftstart;
    int j = rightstart;
    int k = leftstart;

    while (i <= leftend && j <= rightend) {
        if (A[i] <= A[j]) {
            B[k] = A[i];
            i++;
        } else {
            B[k] = A[j];
            j++;
        }
        k++;
    }

    while (i <= leftend) {
        B[k] = A[i];
        i++;
        k++;
    }

    while (j <= rightend) {
        B[k] = A[j];
        j++;
        k++;
    }

    for (i = leftstart; i <= rightend; i++) {
        A[i] = B[i];
    }
}

/* this function will be called by parallel_mergesort() as its base case. */
void my_mergesort(int left, int right){
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);

    merge(left, mid, mid + 1, right);
}

/* we build the argument for the parallel_mergesort function. */
struct argument * buildArgs(int left, int right, int level){
    struct argument *args = malloc(sizeof(struct argument));

    if (args == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    args->left = left;
    args->right = right;
    args->level = level;

    return args;
}

/* this function will be called by the testing program. */
void * parallel_mergesort(void *arg){

    if (arg == NULL)
        return NULL;

    struct argument *a = (struct argument *)arg;
    int left = a->left;
    int right = a->right;
    int level = a->level;

    if (left >= right)
        return NULL;

    if (level >= cutoff) {
        my_mergesort(left, right);
        return NULL;
    }

    int mid = left + (right - left) / 2;

    struct argument *leftArg = buildArgs(left, mid, level + 1);
    struct argument *rightArg = buildArgs(mid + 1, right, level + 1);

    if (leftArg == NULL || rightArg == NULL) {
        free(leftArg);
        free(rightArg);
        my_mergesort(left, right);
        return NULL;
    }

    pthread_t leftThread, rightThread;

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

    free(leftArg);
    free(rightArg);

    merge(left, mid, mid + 1, right);

    return NULL;
}
