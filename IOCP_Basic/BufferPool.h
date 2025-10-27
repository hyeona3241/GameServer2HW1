#pragma once
#include <Utility.h>

// ���� ũ�� ���� Ǯ(���� ����ȭ)
// ������ ���� ���۸� �� ��� Ǯ���� �Ӵ�/�ݳ�
class BufferPool {
public:
    BufferPool(size_t blockSize, size_t initialCount);
    ~BufferPool();
    char* Allocate();    // ���� �Ӵ�
    void Release(char* buf); // ���� �ݳ�
private:
    std::mutex mtx_;             // ���� ���� ��ȣ�� ���ؽ�
    std::vector<char*> pool_;    // ���� ������ ����Ʈ
    size_t blockSize_;           // ���� ũ��
};