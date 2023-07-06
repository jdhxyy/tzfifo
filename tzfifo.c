// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// fifo库.本fifo是满了不能写入类型
// Authors: jdh99 <jdh821@163.com>

#include "tzfifo.h"
#include "tzmalloc.h"
#include <string.h>

#pragma pack(1)

// fifo结构
typedef struct {
    int ptrWrite;
    int ptrRead;
    bool isFull;

    // fifo中存储的元素数,不是字节大小
    int itemSum;
    // 元素大小.单位: 字节
    int itemSize;
    uint8_t* fifoPtr;
} Fifo;

#pragma pack()

static void getSnapshot(Fifo *fifo, Fifo* shot);

// getWriteableItemCount fifo可写的元素数
static int getWriteableItemCount(Fifo *fifo);

// getReadableItemCount fifo可读的元素数
static int getReadableItemCount(Fifo* fifo);

// TZFifoCreate 创建fifo
// itemSum:fifo中元素数.注意不是字节数
// itemSize:元素大小.单位: 字节
// 如果是字节流元素,元素大小为字节流最大长度+2,2是字节流长度
// 如果是混合结构元素,元素大小为结构体和字节流总长度+4,4是结构体长度和字节流长度
// 创建成功返回fifo句柄.如果返回的是0表示创建失败
intptr_t TZFifoCreate(int mid, int itemSum, int itemSize) {
    if (mid  < 0 || itemSum <= 0 || itemSum <= 0) {
        return 0;
    }
    Fifo* fifo = (Fifo*)TZMalloc(mid, sizeof(Fifo));
    if (fifo == NULL) {
        return 0;
    }
    fifo->itemSum = itemSum;
    fifo->itemSize = itemSize;
    fifo->ptrWrite = 0;
    fifo->ptrRead = 0;
    fifo->isFull = false;
    fifo->fifoPtr = TZMalloc(mid, itemSum * itemSize);
    if (fifo->fifoPtr == NULL) {
        TZFree(fifo);
        return 0;
    }
    return (intptr_t)fifo;
}

// TZFifoDelete 删除fifo
void TZFifoDelete(intptr_t handle) {
    if (handle == 0) {
        return;
    }
    Fifo *fifo = (Fifo*)handle;
    TZFree(fifo->fifoPtr);
    TZFree(fifo);
}

// TZFifoReadable 检查fifo是否可以写入
bool TZFifoWriteable(intptr_t handle) {	
    Fifo *fifo = (Fifo*)handle;
    return !fifo->isFull;
}

// TZFifoWrite fifo写入
bool TZFifoWrite(intptr_t handle, uint8_t* data) {
    Fifo *fifo = (Fifo*)handle;

    // 利用快照保存当前指针,防止多线程冲突导致变化
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.isFull) {
        return false;
    }

    memcpy(shot.fifoPtr + shot.ptrWrite * shot.itemSize, data, (size_t)shot.itemSize);
    shot.ptrWrite++;

    if (shot.ptrWrite >= shot.itemSum) {
        shot.ptrWrite = 0;
    }

    // 快照更新
    fifo->ptrWrite = shot.ptrWrite;

    // 如果快照的读指针与真实读指针不一致,则发生读取事件,fifo肯定并没有满
    if (fifo->ptrRead == shot.ptrRead && shot.ptrWrite == shot.ptrRead) {
        fifo->isFull = true;
    }

    return true;
}

static void getSnapshot(Fifo *fifo, Fifo* shot) {
    shot->itemSum = fifo->itemSum;
    shot->itemSize = fifo->itemSize;
    shot->fifoPtr = fifo->fifoPtr;
    
    for (;;) {
        shot->isFull = fifo->isFull;
        shot->ptrWrite = fifo->ptrWrite;
        shot->ptrRead = fifo->ptrRead;

        if (shot->isFull == fifo->isFull && shot->ptrWrite == fifo->ptrWrite && shot->ptrRead == fifo->ptrRead) {
            return;
        }
    }
}

// TZFifoWriteBatch fifo批量写入
// 注意:如果可写入元素数小于待写入数会直接返回失败
bool TZFifoWriteBatch(intptr_t handle, uint8_t* data, int itemNum) {
    Fifo *fifo = (Fifo *)handle;
    // 利用快照保存当前指针,防止多线程冲突导致变化
    Fifo shot;
    getSnapshot(fifo, &shot);
    
    if (getWriteableItemCount(&shot) < itemNum) {
        return false;
    }

    int num = itemNum;
    int delta = 0;
    if (shot.ptrRead <= shot.ptrWrite) {
        num = shot.itemSum - shot.ptrWrite;
        if (itemNum <= num) {
            num = itemNum;
        } else {
            delta = itemNum - num;
        }
    }

    if (num > 0) {
        memcpy(shot.fifoPtr + shot.ptrWrite * shot.itemSize, data, (size_t)(shot.itemSize * num));
        shot.ptrWrite += num;
        if (shot.ptrWrite >= shot.itemSum) {
            shot.ptrWrite = 0;
        }
    }
    if (delta > 0) {
        memcpy(shot.fifoPtr + shot.ptrWrite * shot.itemSize, data + num, (size_t)(shot.itemSize * delta));
        shot.ptrWrite += delta;
        if (shot.ptrWrite >= shot.itemSum) {
            shot.ptrWrite = 0;
        }
    }

    // 快照更新
    fifo->ptrWrite = shot.ptrWrite;

    // 如果快照的读指针与真实读指针不一致,则发生读取事件,fifo肯定并没有满
    if (fifo->ptrRead == shot.ptrRead && shot.ptrWrite == shot.ptrRead) {
        fifo->isFull = true;
    }

    return true;
}

// getWriteableItemCount fifo可写的元素数
static int getWriteableItemCount(Fifo *fifo) {
    if (fifo->isFull) {
        return 0;
    } else {
        if (fifo->ptrWrite == fifo->ptrRead) {
            return fifo->itemSum;
        } else {
            return (fifo->itemSum + fifo->ptrRead - fifo->ptrWrite) % fifo->itemSum;
        }
    }
}

// TZFifoReadable 检查fifo是否可以读取
bool TZFifoReadable(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.ptrWrite == shot.ptrRead && !shot.isFull) {
        return false;
    }
    return true;
}

// TZFifoRead fifo读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
bool TZFifoRead(intptr_t handle, uint8_t* data, int size) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.ptrWrite == shot.ptrRead && !shot.isFull) {
        return false;
    }
    if (size < shot.itemSize) {
        return false;
    }

    memcpy(data, shot.fifoPtr + shot.ptrRead * shot.itemSize, (size_t)shot.itemSize);
    shot.ptrRead++;
    if (shot.ptrRead >= shot.itemSum) {
        shot.ptrRead = 0;
    }

    // 快照更新
    fifo->ptrRead = shot.ptrRead;

    fifo->isFull = false;
    return true;
}

// TZFifoReadBatch fifo批量读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
// 注意:如果可读取元素数小于待读取数会直接返回失败
bool TZFifoReadBatch(intptr_t handle, uint8_t* data, int size, int itemNum) {
    Fifo *fifo = (Fifo *)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (getReadableItemCount(&shot) < itemNum) {
        return false;
    }

    if (size < shot.itemSize * itemNum) {
        return false;
    }

    int num = itemNum;
    int delta = 0;
    if (shot.ptrWrite <= shot.ptrRead) {
        num = shot.itemSum - shot.ptrRead;
        if (itemNum <= num) {
            num = itemNum;
        } else {
            delta = itemNum - num;
        }
    }

    if (num > 0) {
        memcpy(data, shot.fifoPtr + shot.ptrRead * shot.itemSize, (size_t)(shot.itemSize * num));
        shot.ptrRead += num;
        if (shot.ptrRead >= shot.itemSum) {
            shot.ptrRead = 0;
        }
    }
    if (delta > 0) {
        memcpy(data + num, shot.fifoPtr + shot.ptrRead * shot.itemSize, (size_t)(shot.itemSize * delta));
        shot.ptrRead += delta;
        if (shot.ptrRead >= shot.itemSum) {
            shot.ptrRead = 0;
        }
    }

    // 快照更新
    fifo->ptrRead = shot.ptrRead;

    fifo->isFull = false;
    return true;
}

// getReadableItemCount fifo可读的元素数
static int getReadableItemCount(Fifo* fifo) {
    if (fifo->isFull) {
        return fifo->itemSum;
    } else {
        return (fifo->itemSum + fifo->ptrWrite - fifo->ptrRead) % fifo->itemSum;
    }
}

// TZFifoReadableItemCount fifo可读的元素数
int TZFifoReadableItemCount(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);
    return getReadableItemCount(&shot);
}

// TZFifoWriteableItemCount fifo可写的元素数
int TZFifoWriteableItemCount(intptr_t handle) {
    Fifo *fifo = (Fifo *)handle;
    return getWriteableItemCount(fifo);
}

// TZFifoWrite fifo写入字节流
bool TZFifoWriteBytes(intptr_t handle, uint8_t* bytes, int size) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.isFull || size + 2 > shot.itemSize || size > 0xffff) {
        return false;
    }

    uint8_t* ptr = shot.fifoPtr + shot.ptrWrite * shot.itemSize;
    int j = 0;
    ptr[j++] = (uint8_t)size;
    ptr[j++] = (uint8_t)(size >> 8);
    memcpy(ptr + j, bytes, (size_t)size);
    shot.ptrWrite++;

    if (shot.ptrWrite >= shot.itemSum) {
        shot.ptrWrite = 0;
    }

    // 快照更新
    fifo->ptrWrite = shot.ptrWrite;

    // 如果快照的读指针与真实读指针不一致,则发生读取事件,fifo肯定并没有满
    if (fifo->ptrRead == shot.ptrRead && shot.ptrWrite == shot.ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoRead fifo读取字节流
// bytes需提前开辟好空间,字节流存储在bytes中.size是bytes开辟空间大小
// 返回字节流的字节数.0表示读取失败
int TZFifoReadBytes(intptr_t handle, uint8_t* bytes, int size) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.ptrWrite == shot.ptrRead && !shot.isFull) {
        return 0;
    }

    uint8_t* ptr = shot.fifoPtr + shot.ptrRead * shot.itemSize;
    int j = 0;
    int sizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (size + j > shot.itemSize) {
        return 0;
    }
    if (size < sizeGet) {
        return 0;
    }

    memcpy(bytes, ptr + j, (size_t)sizeGet);
    shot.ptrRead++;
    if (shot.ptrRead >= shot.itemSum) {
        shot.ptrRead = 0;
    }

    // 快照更新
    fifo->ptrRead = shot.ptrRead;

    fifo->isFull = false;
    return sizeGet;
}

// TZFifoWriteMix fifo写入混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流
bool TZFifoWriteMix(intptr_t handle, uint8_t* data, int dataSize, uint8_t* bytes, int bytesSize) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.isFull || dataSize + bytesSize + 4 > shot.itemSize || dataSize > 0xffff || bytesSize > 0xffff) {
        return false;
    }

    uint8_t* ptr = shot.fifoPtr + shot.ptrWrite * shot.itemSize;
    int j = 0;
    ptr[j++] = (uint8_t)dataSize;
    ptr[j++] = (uint8_t)(dataSize >> 8);
    memcpy(ptr + j, data, (size_t)dataSize);
    j += dataSize;

    ptr[j++] = (uint8_t)bytesSize;
    ptr[j++] = (uint8_t)(bytesSize >> 8);
    memcpy(ptr + j, bytes, (size_t)bytesSize);
    j += bytesSize;

    shot.ptrWrite++;
    if (shot.ptrWrite >= shot.itemSum) {
        shot.ptrWrite = 0;
    }

    // 快照更新
    fifo->ptrWrite = shot.ptrWrite;

    // 如果快照的读指针与真实读指针不一致,则发生读取事件,fifo肯定并没有满
    if (fifo->ptrRead == shot.ptrRead && shot.ptrWrite == shot.ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoReadMix fifo读取混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流.data和bytes需提前开辟好空间
// 返回字节流的字节数.0表示读取失败
int TZFifoReadMix(intptr_t handle, uint8_t *data, int dataSize, uint8_t* bytes, int bytesSize) {
    Fifo *fifo = (Fifo*)handle;
    Fifo shot;
    getSnapshot(fifo, &shot);

    if (shot.ptrWrite == shot.ptrRead && !shot.isFull) {
        return 0;
    }

    uint8_t* ptr = shot.fifoPtr + shot.ptrRead * shot.itemSize;
    int j = 0;
    int dataSizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (dataSizeGet + j > shot.itemSize) {
        return 0;
    }
    if (dataSize < dataSizeGet) {
        return 0;
    }
    memcpy(data, ptr + j, (size_t)dataSizeGet);
    j += dataSizeGet;

    int bytesSizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (bytesSizeGet + j > shot.itemSize) {
        return 0;
    }
    if (bytesSize < bytesSizeGet) {
        return 0;
    }
    memcpy(bytes, ptr + j, (size_t)bytesSizeGet);
    j += bytesSizeGet;

    shot.ptrRead++;
    if (shot.ptrRead >= shot.itemSum) {
        shot.ptrRead = 0;
    }

    // 快照更新
    fifo->ptrRead = shot.ptrRead;

    fifo->isFull = false;
    return bytesSizeGet;
}

// TZFifoClear 清空FIFO
void TZFifoClear(intptr_t handle) {
    if (handle == 0) {
        return;
    }
    Fifo *fifo = (Fifo*)handle;
    fifo->ptrWrite = 0;
    fifo->ptrRead = 0;
    fifo->isFull = false;
}
