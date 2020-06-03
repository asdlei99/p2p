#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>

//#define CATCH_CONFIG_RUNNER

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
private:
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

class ClientEventCB : public EventCallback
{
private:
	int OnFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp) { 
		//printf("frame size:%u, type:%u, timestamp:%u\n", size, type, timestamp);
		if (last_timestamp_ > 0 && (last_timestamp_+1) != timestamp) {
			printf("frame(%u) loss !!! \n", last_timestamp_ + 1);
		}
		last_timestamp_ = timestamp;
		return 0;
	};

	uint32_t last_timestamp_ = 0;
};

int main(int argc, char* argv[])
{
	ServerEventCB server_event_cb;
	ClientEventCB client_event_cb;

	MediaServer server;
	server.SetOption(OPT_SET_FEC_PERC, 15); /* ÉèÖÃFEC±ÈÀý */ 
	//server.SetOption(OPT_SET_PACKET_LOSS_PERC, 5); /* ¶ª°ü²âÊÔ */ 
	server.SetEventCallback(&server_event_cb);
	server.Start("0.0.0.0", 17676);
	
	MediaClient client;
	client.SetEventCallback(&client_event_cb);
	client.Connect("127.0.0.1", 17676);

	int buffer_size = 1024 * 100;
	std::shared_ptr<uint8_t> buffer(new uint8_t[buffer_size]);
	uint32_t timestamp = 0;

	while (1)
	{
		if (!client.IsConnected()) {
			break;
		}

		if (is_start_play) {
			server.SendFrame(buffer.get(), buffer_size, 1, timestamp++);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	getchar();
	return 0;
}

#endif