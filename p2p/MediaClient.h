#ifndef MEDIA_CLIENT_H
#define MEDIA_CLIENT_H

#include <mutex>
#include <atomic>
#include <queue>
#include "ByteArray.hpp"
#include "ENetClient.h"
#include "RtpSource.h"
#include "EventCallback.h"

class MediaClient
{
public:
	MediaClient();
	virtual ~MediaClient();

	void SetEventCallback(EventCallback* event_cb);

	bool Connect(const char* ip, uint16_t port, uint32_t timeout_msec = 5000);
	void Close();
	bool IsConnected();

private:
	bool Start();
	void Stop();
	void PollEvent(bool run_once = false, uint32_t timeout_msec = 100);
	void StartPlay();
	void PlayStream();
	void OnFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp);
	bool OnMessage(const char* message, uint32_t len);
	void SendActive();
	void SendSetup();
	void SendPlay();

	std::mutex mutex_;
	bool is_started_ = false;

	std::shared_ptr<std::thread> event_thread_;
	ENetClient event_client_;

	asio::io_service io_service_;
	std::unique_ptr<asio::io_service::work> io_service_work_;
	std::unique_ptr<std::thread> io_service_thread_;

	std::mutex queue_mutex_;
	std::queue<std::shared_ptr<ByteArray>> frame_queue_;

	EventCallback* event_cb_;

	std::shared_ptr<RtpSource> rtp_source_;
	std::atomic_bool is_active_;
	std::atomic_bool is_setup_;
	std::atomic_bool is_start_play_;
	std::atomic_bool is_play_;
};

#endif