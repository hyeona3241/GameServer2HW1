#include "ServerTypes.h"

// -------------------- BufferPool ���� --------------------

// ���� ũ�� ���� Ǯ ������: �̸� ������ ������ŭ ���۸� �Ҵ��ص�
BufferPool::BufferPool(size_t blockSize, size_t initialCount) : blockSize_(blockSize) {
    for (size_t i = 0; i < initialCount; ++i)
        pool_.push_back(new char[blockSize_]);
}

// ���� Ǯ �Ҹ���: �����ִ� ���� ��� ����
BufferPool::~BufferPool() {
    for (auto* p : pool_) delete[] p;
}

// ���� �Ӵ�: Ǯ�� ���� ���۰� ������ ��ȯ, ������ ���� �Ҵ�
char* BufferPool::Allocate() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (pool_.empty()) return new char[blockSize_];
    char* buf = pool_.back(); pool_.pop_back();
    return buf;
}

// ���� �ݳ�: ����� ���� ���۸� Ǯ�� �ٽ� ����
void BufferPool::Release(char* buf) {
    std::lock_guard<std::mutex> lock(mtx_);
    pool_.push_back(buf);
}