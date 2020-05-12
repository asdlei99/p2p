#ifndef EVENT_CALLBACK_H
#define EVENT_CALLBACK_H

#include <cstdint>
#include <string>

class EventCallback
{
protected:

	/************************* Server Event *************************/

	/* client connect */
	virtual bool Connect(std::string token) { return true; };

	/* client disconnect */
	virtual void Disconnect() { };

	/* start sending video stream to client */
	virtual void StartPlay() {};

	/* stop sending video stream to client */
	virtual void StopPlay() {};

	/* encode the next picture as an IDR frame. */
	virtual void RequestKeyFrame() {};

	/* change encoder bitrate */
	virtual void ChangeBitrate() {};

	/****************************************************************/


	/************************* Client Event *************************/

	virtual int OnFrame(uint8_t* data, uint32_t size, uint8_t type, uint32_t timestamp) { return 0; };

	/****************************************************************/

protected:
	friend class MediaServer;
	friend class MediaClient;

	virtual ~EventCallback() {}
};


#endif