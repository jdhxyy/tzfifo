// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// fifo��.��fifo�����˲���д������
// Authors: jdh99 <jdh821@163.com>

#include "tzfifo.h"
#include "tzmalloc.h"
#include <string.h>

#pragma pack(1)

// fifo�ṹ
typedef struct {
    int ptrWrite;
    int ptrRead;
    bool isFull;

    // fifo�д洢��Ԫ����,�����ֽڴ�С
    int itemSum;
    // Ԫ�ش�С.��λ: �ֽ�
    int itemSize;
    uint8_t* fifoPtr;
} Fifo;

#pragma pack()

// TZFifoCreate ����fifo
// itemSum:fifo��Ԫ����.ע�ⲻ���ֽ���
// itemSize:Ԫ�ش�С.��λ: �ֽ�
// ������ֽ���Ԫ��,Ԫ�ش�СΪ�ֽ�����󳤶�+2,2���ֽ�������
// ����ǻ�ϽṹԪ��,Ԫ�ش�СΪ�ֽ���פ�ذ�����+4,4�ǽṹ�峤�Ⱥ��ֽ�������
// �����ɹ�����fifo���.������ص���0��ʾ����ʧ��
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

// TZFifoDelete ɾ��fifo
void TZFifoDelete(intptr_t handle) {
    if (handle == 0) {
        return;
    }
    Fifo *fifo = (Fifo*)handle;
    TZFree(fifo->fifoPtr);
    TZFree(fifo);
}

// TZFifoReadable ���fifo�Ƿ����д��
bool TZFifoWriteable(intptr_t handle) {	
    Fifo *fifo = (Fifo*)handle;
    return !fifo->isFull;
}

// TZFifoWrite fifoд��
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

// TZFifoWriteBatch fifo����д��
// ע��:�����д��Ԫ����С�ڴ�д������ֱ�ӷ���ʧ��
bool TZFifoWriteBatch(intptr_t handle, uint8_t* data, int itemNum) {
    if (TZFifoWriteableItemCount(handle) < itemNum) {
        return false;
    }

    Fifo *fifo = (Fifo*)handle;
    memcpy(fifo->fifoPtr + fifo->ptrWrite * fifo->itemSize, data, (size_t)(fifo->itemSize * itemNum));
    fifo->ptrWrite += itemNum;

    if (fifo->ptrWrite >= fifo->itemSum) {
        fifo->ptrWrite = 0;
    }
    if (fifo->ptrWrite == fifo->ptrRead) {
        fifo->isFull = true;
    }
    return true;
}

// TZFifoReadable ���fifo�Ƿ���Զ�ȡ
bool TZFifoReadable(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->ptrWrite == fifo->ptrRead && !fifo->isFull) {
        return false;
    }
    return true;
}

// TZFifoRead fifo��ȡ
// data����ǰ���ٺÿռ�,���ݴ洢��data��.size��data���ٿռ��С
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

// TZFifoReadBatch fifo������ȡ
// data����ǰ���ٺÿռ�,���ݴ洢��data��.size��data���ٿռ��С
// ע��:����ɶ�ȡԪ����С�ڴ���ȡ����ֱ�ӷ���ʧ��
bool TZFifoReadBatch(intptr_t handle, uint8_t* data, int size, int itemNum) {
    if (TZFifoReadableItemCount(handle) < itemNum) {
        return false;
    }

    Fifo *fifo = (Fifo*)handle;
    if (size < fifo->itemSize * itemNum) {
        return false;
    }
    memcpy(data, fifo->fifoPtr + fifo->ptrRead * fifo->itemSize, (size_t)(fifo->itemSize * itemNum));
    fifo->ptrRead += itemNum;
    if (fifo->ptrRead >= fifo->itemSum) {
        fifo->ptrRead = 0;
    }
    fifo->isFull = false;
    return true;
}

// TZFifoReadableItemCount fifo�ɶ���Ԫ����
int TZFifoReadableItemCount(intptr_t handle) {
    Fifo *fifo = (Fifo*)handle;
    if (fifo->isFull) {
        return fifo->itemSum;
    } else {
        return (fifo->itemSum + fifo->ptrWrite - fifo->ptrRead) % fifo->itemSum;
    }
}

// TZFifoWriteableItemCount fifo��д��Ԫ����
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

// TZFifoWrite fifoд���ֽ���
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

// TZFifoRead fifo��ȡ�ֽ���
// bytes����ǰ���ٺÿռ�,�ֽ����洢��bytes��.size��bytes���ٿռ��С
// �����ֽ������ֽ���.0��ʾ��ȡʧ��
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

// TZFifoWriteMix fifoд���Ͻṹ.��Ͻṹ��:�̶����ȵĽṹ��+�ֽ���
// data�ǽṹ��,bytes���ֽ���
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

// TZFifoReadMix fifo��ȡ��Ͻṹ.��Ͻṹ��:�̶����ȵĽṹ��+�ֽ���
// data�ǽṹ��,bytes���ֽ���.data��bytes����ǰ���ٺÿռ�
// �����ֽ������ֽ���.0��ʾ��ȡʧ��
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
