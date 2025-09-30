#pragma once

#include "Utility.h"

class EchoClient
{
private:
	// 상태
	SOCKET sock_ = INVALID_SOCKET;
	sockaddr_in serverAddr_{};
	WSADATA wsaData_{};
	std::atomic<bool> running_{ false };

	// 수신 버퍼
	char rxBuf_[NetConfig::BUFFER_SIZE ];

	std::string nickname_;

public:
	bool init();
	bool createSocket();
	bool connectToServer();

	void run();
	void stop();

private:
	void receiveLoop();
	void sendLoop();

public:
	void setNickname(const std::string& n) { nickname_ = n; }

private:
	bool sendBytesAll(const char* data, size_t len);
	bool sendPacket(const IPacket& pkt);
	static uint64_t nowUnixMillis();

};

