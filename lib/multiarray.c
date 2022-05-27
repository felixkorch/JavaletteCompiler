
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct MultiArray_T {
    int32_t length;
    void* ptr;
} MultiArray;

MultiArray* newMultiArray(int32_t len, void* ptrTo)
{
    MultiArray* array = malloc(sizeof(MultiArray));
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