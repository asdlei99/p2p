#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H
 
#include <mutex>
#include <thread>
#include <memory>
#include <queue>
#include "ENetServer.h"
#include "MediaSession.h"
#include "EventCallback.h"
#include "ByteArray.hpp"
#include "asio.hpp"

class MediaServer
{
public:
	MediaServer();
	virtual ~MediaServer();

	void SetEventCallback(EventCallback* event_cb);

	bool Start(const char* ip, uint16_t port);
	void Stop();

	int SendFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp=0);

private:
	void EventLoop();
	void OnMessage(uint32_t cid, const char* message, uint32_t len);
	uint32_t OnActive(uint32_t cid, ByteArray& message);
	uint32_t OnSetup(uint32_t cid, ByteArray& message);
	uint32_t OnPlay(uint32_t cid, ByteArray& message);
	
	std::mutex mutex_;
	bool is_started_ = false;

	std::shared_ptr<std::thread> event_thread_;
	ENetServer event_server_;

	asio::io_service io_service_;
	std::unique_ptr<asio::io_service::work> io_service_work_;
	std::unique_ptr<std::thread> io_service_thread_;

	typedef std::shared_ptr<MediaSession> MediaSessionPtr;
	std::map<uint32_t, MediaSessionPtr> media_sessions_;

	EventCallback* event_cb_;

	std::mutex queue_mutex_;
	std::queue<std::shared_ptr<ByteArray>> frame_queue_;

	static const int kMaxFrameLength = 512;
	static const int kMaxConnectios = 2;
};

#endif