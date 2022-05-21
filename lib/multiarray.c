
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct MultiArray_T {
    int32_t length;
    void* ptr;
} MultiArray;

MultiArray* newMultiArray(int32_t len, void* ptrTo)
{
    MultiArray* array = calloc(1, sizeof(MultiArray));
    array->length = len;
    array->ptr = ptrTo;
    return array;
}

MultiArray* multiArray(int32_t n, int32_t size, int32_t* dimList)
{
    int len = dimList[0];

    if (n == 1) {
        void* content = calloc(len, size);
        return newMultiArray(len, content);
    }

    void** span = malloc(len * sizeof(void*));

    for (int i = 0; i < len; i++) {
        span[i] = multiArray(n - 1, size, &dimList[1]);
    }

    MultiArray array = { .length = len, .ptr = span };
    return newMultiArray(len, span);
}

/*
void freeMultiArray(void** array, int32_t n, int32_t* dimList) {
    if (n == 1) {
        free(array);
    } else {
        int dim = dimList[0];

        for (int i = 0; i < dim; i++) {
            freeMultiArray(array[i], n - 1, &dimList[1]);
        }
    }
} */

/*
int main() {
    int x[2][3];
    int** y;

    printf("x: %lu\n", sizeof(x));
    printf("y: %lu\n", sizeof(y));

    
    int d[] = {2, 3, 4};
    int*** m = multiArray(3, sizeof(int), d);

    int a = 0;
    for (int i = 0; i < d[0]; i++) {
        for (int j = 0; j < d[1]; j++) {
            for (int k = 0; k < d[2]; k++) {
                m[i][j][k] = a++;
            }
        }
    }

    for (int i = 0; i < d[0]; i++) {
        for (int j = 0; j < d[1]; j++) {
            for (int k = 0; k < d[2]; k++) {
                printf("(%d, %d, %d) : %d\n", i, j, k, m[i][j][k]);
            }
        }
    }

    freeMultiArray(m, 3, d);
    
}

*/