// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// fifo库.本fifo是满了不能写入类型
// Authors: jdh99 <jdh821@163.com>
// 本ffo支持3种类型的数据结构:
// 1.结构体读写,TZFifoWrite/TZFifoRead
// 2.字节流读写,TZFifoWriteBytes/TZFifoReadBytes
// 3.结构体+字节流混合结构读写,TZFifoWriteMix/TZFifoReadMix

#ifndef TZFIFO_H
#define TZFIFO_H

#include <stdint.h>
#include <stdbool.h>

// TZFifoCreate 创建fifo
// itemSum:fifo中元素数.注意不是字节数
// itemSize:元素大小.单位: 字节
// 如果是字节流元素,元素大小为字节流最大长度+2,2是字节流长度
// 如果是混合结构元素,元素大小为结构体和字节流总长度+4,4是结构体长度和字节流长度
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

#endif
