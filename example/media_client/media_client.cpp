#include <thread>
#include <chrono>
#include "MediaClient.h"

class ClientEventCB : public EventCallback
{
private:
	int OnFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp) {
		printf("frame size:%u, type:%u, timestamp:%u\n", size, type, timestamp);
		return 0;
	};
};

int main(int argc, char* argv[])
{
	ClientEventCB client_event_cb;

	MediaClient client;
	client.SetEventCallback(&client_event_cb);
	client.Connect("127.0.0.1", 17676);

	while (1)
	{
		if (!client.IsConnected()) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	getchar();
	return 0;
}