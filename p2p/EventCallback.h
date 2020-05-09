#ifndef EVENT_CALLBACK_H
#define EVENT_CALLBACK_H

#include <cstdint>

class EventCallback
{
public:

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

	virtual void OnFrame() { };

	/****************************************************************/

protected:
	virtual ~EventCallback() {}
};


#endif