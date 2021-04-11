# tzfifo

## 介绍
tzfifo是标准fifo，可以存储任意类型的数据。类型是满了不能写入。

tzfifo依赖内存管理包tzmalloc,使用cip可以安装依赖的包:[cip：C/C++包管理工具](https://blog.csdn.net/jdh99/article/details/115590099)


## 开源
- [github上的项目地址](https://github.com/jdhxyy/tzfifo)
- [gitee上的项目地址](https://gitee.com/jdhxyy/tzfifo)

## API
```c
// TZFifoCreate 创建fifo
// itemSum:fifo中元素数.注意不是字节数
// itemSize:元素大小.单位: 字节
// 如果是字节流元素,元素大小为字节流最大长度+2,2是字节流长度
// 如果是混合结构元素,元素大小为字节流驻地啊长度+4,4是结构体长度和字节流长度
// 创建成功返回fifo句柄.如果返回的是0表示创建失败
intptr_t TZFifoCreate(int mid, int itemSum, int itemSize);

// TZFifoDelete 删除fifo
void TZFifoDelete(intptr_t handle);

// TZFifoReadable 检查fifo是否可以写入
bool TZFifoWriteable(intptr_t handle);

// TZFifoWrite fifo写入
bool TZFifoWrite(intptr_t handle, uint8_t* data);

// TZFifoWriteBatch fifo批量写入
// 注意:如果可写入元素数小于待写入数会直接返回失败
bool TZFifoWriteBatch(intptr_t handle, uint8_t* data, int itemNum);

// TZFifoReadable 检查fifo是否可以读取
bool TZFifoReadable(intptr_t handle);

// TZFifoRead fifo读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
bool TZFifoRead(intptr_t handle, uint8_t* data, int size);

// TZFifoReadBatch fifo批量读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
// 注意:如果可读取元素数小于待读取数会直接返回失败
bool TZFifoReadBatch(intptr_t handle, uint8_t* data, int size, int itemNum);

// TZFifoReadableItemCount fifo可读的元素数
int TZFifoReadableItemCount(intptr_t handle);

// TZFifoWriteableItemCount fifo可写的元素数
int TZFifoWriteableItemCount(intptr_t handle);

// TZFifoWrite fifo写入字节流
bool TZFifoWriteBytes(intptr_t handle, uint8_t* bytes, int size);

// TZFifoRead fifo读取字节流
// bytes需提前开辟好空间,字节流存储在bytes中.size是bytes开辟空间大小
// 返回字节流的字节数.0表示读取失败
int TZFifoReadBytes(intptr_t handle, uint8_t* bytes, int size);

// TZFifoWriteMix fifo写入混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流
bool TZFifoWriteMix(intptr_t handle, uint8_t* data, int dataSize, uint8_t* bytes, int bytesSize);

// TZFifoReadMix fifo读取混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流.data和bytes需提前开辟好空间
// 返回字节流的字节数.0表示读取失败
int TZFifoReadMix(intptr_t handle, uint8_t *data, int dataSize, uint8_t* bytes, int bytesSize);
```

## 测试
```c
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
```

输出:
```text
mid:0 tag:fifoTest total:102400 used:0 mallocNum:0 freeNum:0
-------------------->testCase0
可写：9 可读：1
可写：8 可读：2
可写：7 可读：3
可写：6 可读：4
可写：5 可读：5
可写：4 可读：6
可写：3 可读：7
可写：2 可读：8
可写：1 可读：9
可写：0 可读：10
read:0 100
read:1 101
read:2 102
read:3 103
read:4 104
read:5 105
read:6 106
read:7 107
read:8 108
read:9 109
fifo2可写：97 可读：3
read:jdh
-------------------->testCase1
read str:4 jdh
-------------------->testCase2
read mix:4 5 6 jdh
mid:0 tag:fifoTest total:102400 used:0 mallocNum:8 freeNum:8
```
