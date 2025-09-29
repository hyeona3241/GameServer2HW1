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

	// 송신 경로(택1)
	// (A) 콘솔을 sendLoop에서 직접 getline()하여 바로 send
	// (B) 외부에서 push → sendLoop가 큐를 비우며 send
	std::mutex              sendMtx_;
	std::condition_variable sendCv_;
	std::queue<std::string> sendQ_;

	// 수신 버퍼
	char rxBuf_[NetConfig::BUFFER_SIZE ];

public:
	bool init();
	bool createSocket();
	bool connectToServer();

	void run();
	void stop();

private:
	void receiveLoop();
	void sendLoop();

};

