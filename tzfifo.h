// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// fifo��.��fifo�����˲���д������
// Authors: jdh99 <jdh821@163.com>
// ��ffo֧��3�����͵����ݽṹ:
// 1.�ṹ���д,TZFifoWrite/TZFifoRead
// 2.�ֽ�����д,TZFifoWriteBytes/TZFifoReadBytes
// 3.�ṹ��+�ֽ�����Ͻṹ��д,TZFifoWriteMix/TZFifoReadMix

#ifndef TZFIFO_H
#define TZFIFO_H

#include <stdint.h>
#include <stdbool.h>

// TZFifoCreate ����fifo
// itemSum:fifo��Ԫ����.ע�ⲻ���ֽ���
// itemSize:Ԫ�ش�С.��λ: �ֽ�
// ������ֽ���Ԫ��,Ԫ�ش�СΪ�ֽ�����󳤶�+2,2���ֽ�������
// ����ǻ�ϽṹԪ��,Ԫ�ش�СΪ�ֽ�����󳤶�+4,4�ǽṹ�峤�Ⱥ��ֽ�������
// �����ɹ�����fifo���.������ص���0��ʾ����ʧ��
intptr_t TZFifoCreate(int mid, int itemSum, int itemSize);

// TZFifoDelete ɾ��fifo
void TZFifoDelete(intptr_t handle);

// TZFifoReadable ���fifo�Ƿ����д��
bool TZFifoWriteable(intptr_t handle);

// TZFifoWrite fifoд��
bool TZFifoWrite(intptr_t handle, uint8_t* data);

// TZFifoWriteBatch fifo����д��
// ע��:�����д��Ԫ����С�ڴ�д������ֱ�ӷ���ʧ��
bool TZFifoWriteBatch(intptr_t handle, uint8_t* data, int itemNum);

// TZFifoReadable ���fifo�Ƿ���Զ�ȡ
bool TZFifoReadable(intptr_t handle);

// TZFifoRead fifo��ȡ
// data����ǰ���ٺÿռ�,���ݴ洢��data��.size��data���ٿռ��С
bool TZFifoRead(intptr_t handle, uint8_t* data, int size);

// TZFifoReadBatch fifo������ȡ
// data����ǰ���ٺÿռ�,���ݴ洢��data��.size��data���ٿռ��С
// ע��:����ɶ�ȡԪ����С�ڴ���ȡ����ֱ�ӷ���ʧ��
bool TZFifoReadBatch(intptr_t handle, uint8_t* data, int size, int itemNum);

// TZFifoReadableItemCount fifo�ɶ���Ԫ����
int TZFifoReadableItemCount(intptr_t handle);

// TZFifoWriteableItemCount fifo��д��Ԫ����
int TZFifoWriteableItemCount(intptr_t handle);

// TZFifoWrite fifoд���ֽ���
bool TZFifoWriteBytes(intptr_t handle, uint8_t* bytes, int size);

// TZFifoRead fifo��ȡ�ֽ���
// bytes����ǰ���ٺÿռ�,�ֽ����洢��bytes��.size��bytes���ٿռ��С
// �����ֽ������ֽ���.0��ʾ��ȡʧ��
int TZFifoReadBytes(intptr_t handle, uint8_t* bytes, int size);

// TZFifoWriteMix fifoд���Ͻṹ.��Ͻṹ��:�̶����ȵĽṹ��+�ֽ���
// data�ǽṹ��,bytes���ֽ���
bool TZFifoWriteMix(intptr_t handle, uint8_t* data, int dataSize, uint8_t* bytes, int bytesSize);

// TZFifoReadMix fifo��ȡ��Ͻṹ.��Ͻṹ��:�̶����ȵĽṹ��+�ֽ���
// data�ǽṹ��,bytes���ֽ���.data��bytes����ǰ���ٺÿռ�
// �����ֽ������ֽ���.0��ʾ��ȡʧ��
int TZFifoReadMix(intptr_t handle, uint8_t *data, int dataSize, uint8_t* bytes, int bytesSize);

#endif
