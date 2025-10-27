#pragma once
#include <vector>
#include <cstdint>
#include "PacketDef.h"
#include "RingBuffer.h"

class PacketFramer
{
public:
    static constexpr size_t kHeaderSize = sizeof(PacketHeader);
    static constexpr size_t kMaxPacket = 32 * 1024; // ��� ����(�ʿ�� ����)

    enum class Result {
        Ok,         // ��Ŷ �ϳ� ���������� pop �� (out�� ���+�ٵ� ���� ä��)
        NeedMore,   // ���� �����Ͱ� �� �ʿ���(��� or �ٵ� �̵���)
        Malformed   // ������ ���/������(�������� ���� ����/�α� ����)
    };

    // �����ۿ��� "�ϼ��� ��Ŷ(���+�ٵ�)" 1���� ���� out�� ��´�.
    Result Pop(RingBuffer& rb, std::vector<char>& out) const noexcept {
        if (rb.readable() < kHeaderSize) return Result::NeedMore;

        // ��� �̸�����(�Һ� X)
        PacketHeader h{};
        if (!rb.peek(&h, kHeaderSize)) return Result::NeedMore;

        // ��� ��ȿ�� �˻�
        if (h.size < kHeaderSize || h.size > kMaxPacket) {
            return Result::Malformed;
        }

        // ��Ŷ ��ü(���+�ٵ�)�� ��� �����ߴ��� Ȯ��
        if (rb.readable() < h.size) return Result::NeedMore;

        // ��°�� �о� out�� ���(�Һ�)
        out.resize(h.size);
        const size_t got = rb.read(out.data(), h.size);
        if (got != h.size) return Result::Malformed; // �α� �߰��ϱ�

        return Result::Ok;
    }
};

