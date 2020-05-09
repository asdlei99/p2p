#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "asio.hpp"

#define CATCH_CONFIG_RUNNER

#ifdef CATCH_CONFIG_RUNNER

#include "unittest/unittest.hpp"
int main(int argc, char* argv[]) 
{
	Catch::Session session;
	session.run(argc, argv);
	getchar();
	return 0;
}

#else

#include "MediaServer.h"
#include "MediaClient.h"

static bool is_start_play = false;

class ServerEventCB : public EventCallback
{
public:
	/* client connect */
	virtual bool Connect(std::string token) {
		return true; // false: disconnect connection
	}

	/* client disconnect */
	virtual void Disconnect() {

	}

	/* start sending video stream to client */
	virtual void StartPlay() {
		is_start_play = true;
	}

	/* stop sending video stream to client */
	virtual void StopPlay() {

	}

	/* encode the next picture as an IDR frame. */
	virtual void RequestKeyFrame() {

	}

	/* change encoder bitrate */
	virtual void ChangeBitrate() {

	}
};

int main(int argc, char* argv[])
{
	ServerEventCB server_event_cb;

	MediaServer server;
	server.SetEventCallback(&server_event_cb);
	server.Start("0.0.0.0", 17676);
	
	MediaClient client;
	client.Connect("127.0.0.1", 17676);

	std::shared_ptr<uint8_t> buffer(new uint8_t[1024*1024]);
	uint32_t timestamp = 0;

	while (1)
	{
		if (!client.IsConnected()) {
			break;
		}

		if (is_start_play) {
			//server.SendFrame(buffer.get(), 1024*1024, 1, timestamp++);
		}

		Sleep(100);
	}

	getchar();
	return 0;
}

#endif