#include "H264File.hpp"
#include "MediaServer.h"
#include <thread>
#include <chrono>

static bool is_start_play = false;

class ServerEventCB : public EventCallback
{
private:
	/* client connect */
	virtual bool Connect(std::string token) {
		printf("[MediaServer] Client connet, token: %s \n", token.c_str());
		return true; // false: disconnect connection
	}

	/* client disconnect */
	virtual void Disconnect() {
		printf("[MediaServer] Client disconnect. \n");
	}

	/* start sending video stream to client */
	virtual void StartPlay() {
		printf("[MediaServer] Start play ...  \n");
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
	if (argc < 2) {
		printf("Usage: %s test.h264", argv[0]);
		return -1;
	}

	H264File h264_file;
	if (!h264_file.Open(argv[1])) {
		printf("Open %s failed.", argv[1]);
		return -1;
	}

	ServerEventCB server_event_cb;
	MediaServer server;
	server.SetEventCallback(&server_event_cb);
	if (server.Start("0.0.0.0", 17676)) {
		printf("[MediaServer] Server listen on 17676. \n");
	}

	int buffer_size = 1024 * 1000;
	std::shared_ptr<char> buffer(new char[buffer_size]);
	uint32_t timestamp = 0;

	while (1)
	{
		if (is_start_play) {
			bool end = false;
			int frame_size = h264_file.ReadFrame(buffer.get(), buffer_size, &end);
			if (frame_size > 0) {
				server.SendFrame((uint8_t*)buffer.get(), frame_size, 1, timestamp);
			}			
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		timestamp += 40;
	}

	getchar();
	return 0;
}