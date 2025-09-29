#include "ServerTypes.h"

// -------------------- BufferPool 구현 --------------------

// 고정 크기 버퍼 풀 생성자: 미리 지정된 개수만큼 버퍼를 할당해둠
BufferPool::BufferPool(size_t blockSize, size_t initialCount) : blockSize_(blockSize) {
    for (size_t i = 0; i < initialCount; ++i)
        pool_.push_back(new char[blockSize_]);
}

// 버퍼 풀 소멸자: 남아있는 버퍼 모두 해제
BufferPool::~BufferPool() {
    for (auto* p : pool_) delete[] p;
}

// 버퍼 임대: 풀에 남은 버퍼가 있으면 반환, 없으면 새로 할당
char* BufferPool::Allocate() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (pool_.empty()) return new char[blockSize_];
    char* buf = pool_.back(); pool_.pop_back();
    return buf;
}

// 버퍼 반납: 사용이 끝난 버퍼를 풀에 다시 넣음
void BufferPool::Release(char* buf) {
    std::lock_guard<std::mutex> lock(mtx_);
    pool_.push_back(buf);
}