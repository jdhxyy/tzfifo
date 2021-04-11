#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tzfifo.h"
#include "tzmalloc.h"

#define RAM_INTERVAL 0

#pragma pack(1)

struct _Test {
    uint8_t a;
    uint16_t b;
};

#pragma pack()

static int gMid = -1;

static void printMallocInfo(void);
static void testCase0(void);
static void testCase1(void);
static void testCase2(void);

int main(void) {
    TZMallocLoad(RAM_INTERVAL, 10, 100 * 1024, malloc(100 * 1024));
    gMid = TZMallocRegister(RAM_INTERVAL, "fifoTest", 100 * 1024);

    printMallocInfo();
    testCase0();
    testCase1();
    testCase2();
    printMallocInfo();
}

static void printMallocInfo(void) {
    TZMallocUser* user;
    int num = TZMallocGetUserNum(RAM_INTERVAL);
    for (int i = 0; i < num; i++) {
        user = TZMallocGetUser(i);
        printf("mid:%d tag:%s total:%d used:%d mallocNum:%d freeNum:%d\n", i,
               user->Tag, user->Total, user->Used, user->MallocNum, user->FreeNum);
    }
}

static void testCase0(void) {
    printf("-------------------->testCase0\n");

    // fifo1操作
    struct _Test arr;
    intptr_t fifo1 = TZFifoCreate(gMid, 10, sizeof(struct _Test));
    for (uint8_t i = 0; i < 10; i++) {
        arr.a = i;
        arr.b = 100 + i;
        if (TZFifoWriteable(fifo1)) {
            TZFifoWrite(fifo1, (void *)&arr);
            printf("可写：%d 可读：%d\n", TZFifoWriteableItemCount(fifo1), TZFifoReadableItemCount(fifo1));
        }
    }

    while (1) {
        if (TZFifoReadable(fifo1) == false) {
            break;
        }
        TZFifoRead(fifo1, (void *)&arr, sizeof(struct _Test));
        printf("read:%d %d\n", arr.a, arr.b);
    }

    // fifo2操作
    intptr_t fifo2 = TZFifoCreate(gMid, 100, 1);
    char str_arr[100] = {0};
    memcpy(str_arr, "jdh", 3);
    TZFifoWriteBatch(fifo2, (uint8_t*)str_arr, 3);

    printf("fifo2可写：%d 可读：%d\n", TZFifoWriteableItemCount(fifo2), TZFifoReadableItemCount(fifo2));

    str_arr[0] = 0;
    TZFifoReadBatch(fifo2, (uint8_t*)str_arr, 100, TZFifoReadableItemCount(fifo2));
    printf("read:%s\n", str_arr);
    TZFifoDelete(fifo1);
    TZFifoDelete(fifo2);

    return;
}

static void testCase1(void) {
    printf("-------------------->testCase1\n");

    intptr_t fifo = TZFifoCreate(gMid, 10, 100);
    TZFifoWriteBytes(fifo, (uint8_t*)"jdh", strlen("jdh") + 1);
    char arr[10] = {0};
    int num = TZFifoReadBytes(fifo, (uint8_t*)arr, 10);
    printf("read str:%d %s\n", num, arr);
    TZFifoDelete(fifo);
}

static void testCase2(void) {
    printf("-------------------->testCase2\n");

    intptr_t fifo = TZFifoCreate(gMid, 10, 100);
    struct _Test t1;
    t1.a = 5;
    t1.b = 6;
    TZFifoWriteMix(fifo, (uint8_t*)&t1, sizeof(struct _Test), (uint8_t*)"jdh", strlen("jdh") + 1);

    struct _Test t2;
    char arr[10] = {0};
    int num = TZFifoReadMix(fifo, (uint8_t*)&t2, sizeof(struct _Test), (uint8_t*)arr, 10);
    printf("read mix:%d %d %d %s\n", num, t2.a, t2.b, arr);
    TZFifoDelete(fifo);
}
