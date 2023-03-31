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
    if (fifo->isFull) {
        return false;
    }

    memcpy(fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize, data, (size_t)fifo->itemSize);
    fifo->ptrWrite++;

    if (fifo->ptrWrite >= fifo->itemSum) {
        fifo->ptrWrite = 0;
    }
    if (fifo->ptrWrite == fifo->ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoWriteBatch fifo批量写入
// 注意:如果可写入元素数小于待写入数会直接返回失败
bool TZFifoWriteBatch(intptr_t handle, uint8_t* data, int itemNum) {
    if (TZFifoWriteableItemCount(handle) < itemNum) {
        return false;
    }

    Fifo *fifo = (Fifo *)handle;

    int num = itemNum;
    int delta = 0;
    if (fifo->ptrRead <= fifo->ptrWrite) {
        num = fifo->itemSum - fifo->ptrWrite;
        if (itemNum <= num) {
            num = itemNum;
        } else {
            delta = itemNum - num;
        }
    }

    if (num > 0) {
        memcpy(fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize, data, (size_t)(fifo->itemSize * num));
        fifo->ptrWrite += num;
        if (fifo->ptrWrite >= fifo->itemSum) {
            fifo->ptrWrite = 0;
        }
    }
    if (delta > 0) {
        memcpy(fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize, data + num, (size_t)(fifo->itemSize * delta));
        fifo->ptrWrite += delta;
        if (fifo->ptrWrite >= fifo->itemSum) {
            fifo->ptrWrite = 0;
        }
    }

    if (fifo->ptrWrite == fifo->ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoReadable 检查fifo是否可以读取
bool TZFifoReadable(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->ptrWrite == fifo->ptrRead && !fifo->isFull) {
        return false;
    }
    return true;
}

// TZFifoRead fifo读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
bool TZFifoRead(intptr_t handle, uint8_t* data, int size) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->ptrWrite == fifo->ptrRead && !fifo->isFull) {
        return false;
    }
    if (size < fifo->itemSize) {
        return false;
    }

    memcpy(data, fifo->fifoPtr + fifo->ptrRead * fifo->itemSize, (size_t)fifo->itemSize);
    fifo->ptrRead++;
    if (fifo->ptrRead >= fifo->itemSum) {
        fifo->ptrRead = 0;
    }
    fifo->isFull = false;
    return true;
}

// TZFifoReadBatch fifo批量读取
// data需提前开辟好空间,数据存储在data中.size是data开辟空间大小
// 注意:如果可读取元素数小于待读取数会直接返回失败
bool TZFifoReadBatch(intptr_t handle, uint8_t* data, int size, int itemNum) {
    if (TZFifoReadableItemCount(handle) < itemNum) {
        return false;
    }

    Fifo *fifo = (Fifo *)handle;
    if (size < fifo->itemSize * itemNum) {
        return false;
    }

    int num = itemNum;
    int delta = 0;
    if (fifo->ptrWrite <= fifo->ptrRead) {
        num = fifo->itemSum - fifo->ptrRead;
        if (itemNum <= num) {
            num = itemNum;
        } else {
            delta = itemNum - num;
        }
    }

    if (num > 0) {
        memcpy(data, fifo->fifoPtr + fifo->ptrRead * fifo->itemSize, (size_t)(fifo->itemSize * num));
        fifo->ptrRead += num;
        if (fifo->ptrRead >= fifo->itemSum) {
            fifo->ptrRead = 0;
        }
    }
    if (delta > 0) {
        memcpy(data + num, fifo->fifoPtr + fifo->ptrRead * fifo->itemSize, (size_t)(fifo->itemSize * delta));
        fifo->ptrRead += delta;
        if (fifo->ptrRead >= fifo->itemSum) {
            fifo->ptrRead = 0;
        }
    }

    fifo->isFull = false;
    return true;
}

// TZFifoReadableItemCount fifo可读的元素数
int TZFifoReadableItemCount(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->isFull) {
        return fifo->itemSum;
    } else {
        return (fifo->itemSum + fifo->ptrWrite - fifo->ptrRead) % fifo->itemSum;
    }
}

// TZFifoWriteableItemCount fifo可写的元素数
int TZFifoWriteableItemCount(intptr_t handle) {
    Fifo *fifo = (Fifo *)handle;
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

// TZFifoWrite fifo写入字节流
bool TZFifoWriteBytes(intptr_t handle, uint8_t* bytes, int size) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->isFull || size + 2 > fifo->itemSize || size > 0xffff) {
        return false;
    }

    uint8_t* ptr = fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize;
    int j = 0;
    ptr[j++] = (uint8_t)size;
    ptr[j++] = (uint8_t)(size >> 8);
    memcpy(ptr + j, bytes, (size_t)size);
    fifo->ptrWrite++;

    if (fifo->ptrWrite >= fifo->itemSum) {
        fifo->ptrWrite = 0;
    }
    if (fifo->ptrWrite == fifo->ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoRead fifo读取字节流
// bytes需提前开辟好空间,字节流存储在bytes中.size是bytes开辟空间大小
// 返回字节流的字节数.0表示读取失败
int TZFifoReadBytes(intptr_t handle, uint8_t* bytes, int size) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->ptrWrite == fifo->ptrRead && !fifo->isFull) {
        return 0;
    }

    uint8_t* ptr = fifo->fifoPtr + fifo->ptrRead * fifo->itemSize;
    int j = 0;
    int sizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (size + j > fifo->itemSize) {
        return 0;
    }
    if (size < sizeGet) {
        return 0;
    }

    memcpy(bytes, ptr + j, (size_t)sizeGet);
    fifo->ptrRead++;
    if (fifo->ptrRead >= fifo->itemSum) {
        fifo->ptrRead = 0;
    }
    fifo->isFull = false;
    return sizeGet;
}

// TZFifoWriteMix fifo写入混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流
bool TZFifoWriteMix(intptr_t handle, uint8_t* data, int dataSize, uint8_t* bytes, int bytesSize) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->isFull || dataSize + bytesSize + 4 > fifo->itemSize || dataSize > 0xffff || bytesSize > 0xffff) {
        return false;
    }

    uint8_t* ptr = fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize;
    int j = 0;
    ptr[j++] = (uint8_t)dataSize;
    ptr[j++] = (uint8_t)(dataSize >> 8);
    memcpy(ptr + j, data, (size_t)dataSize);
    j += dataSize;

    ptr[j++] = (uint8_t)bytesSize;
    ptr[j++] = (uint8_t)(bytesSize >> 8);
    memcpy(ptr + j, bytes, (size_t)bytesSize);
    j += bytesSize;

    fifo->ptrWrite++;
    if (fifo->ptrWrite >= fifo->itemSum) {
        fifo->ptrWrite = 0;
    }
    if (fifo->ptrWrite == fifo->ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoReadMix fifo读取混合结构.混合结构是:固定长度的结构体+字节流
// data是结构体,bytes是字节流.data和bytes需提前开辟好空间
// 返回字节流的字节数.0表示读取失败
int TZFifoReadMix(intptr_t handle, uint8_t *data, int dataSize, uint8_t* bytes, int bytesSize) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->ptrWrite == fifo->ptrRead && !fifo->isFull) {
        return 0;
    }

    uint8_t* ptr = fifo->fifoPtr + fifo->ptrRead * fifo->itemSize;
    int j = 0;
    int dataSizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (dataSizeGet + j > fifo->itemSize) {
        return 0;
    }
    if (dataSize < dataSizeGet) {
        return 0;
    }
    memcpy(data, ptr + j, (size_t)dataSizeGet);
    j += dataSizeGet;

    int bytesSizeGet = ptr[j] + (ptr[j + 1] << 8);
    j += 2;
    if (bytesSizeGet + j > fifo->itemSize) {
        return 0;
    }
    if (bytesSize < bytesSizeGet) {
        return 0;
    }
    memcpy(bytes, ptr + j, (size_t)bytesSizeGet);
    j += bytesSizeGet;

    fifo->ptrRead++;
    if (fifo->ptrRead >= fifo->itemSum) {
        fifo->ptrRead = 0;
    }
    fifo->isFull = false;
    return bytesSizeGet;
}
