#ifndef RTP_SOURCE_H
#define RTP_SOURCE_H

#include "UdpSocket.h"
#include "RtpPacket.hpp"
#include <map>

class RtpSource : public std::enable_shared_from_this<RtpSource>
{
public:
	typedef std::function<bool(std::shared_ptr<uint8_t> data, size_t size, uint8_t type, uint32_t timestamp)> FrameCB;

	RtpSource& operator=(const RtpSource&) = delete;
	RtpSource(const RtpSource&) = delete;
	RtpSource(asio::io_service& io_service);
	virtual ~RtpSource();

	bool Open(uint16_t rtp_port, uint16_t rtcp_port);
	bool Open();
	void Close();

	void SetFrameCallback(FrameCB cb)
	{ frame_cb_ = cb; }

	void SetPeerAddress(std::string ip, uint16_t rtp_port, uint16_t rtcp_port);
	void KeepAlive();
	bool IsAlive();

	uint16_t GetRtpPort()  const;
	uint16_t GetRtcpPort() const;

private:
	bool OnRead(void* data, size_t size);
	bool OnFrame(uint32_t timestamp);

	asio::io_context& io_context_;
	asio::io_context::strand io_strand_;

	std::unique_ptr<UdpSocket> rtp_socket_;
	std::unique_ptr<UdpSocket> rtcp_socket_;
	asio::ip::udp::endpoint peer_rtp_address_;
	asio::ip::udp::endpoint peer_rtcp_address_;

	typedef std::map<int, std::shared_ptr<RtpPacket>> RtpPackets;
	std::map<uint32_t, RtpPackets> frames_;

	FrameCB frame_cb_;

	bool is_alived_;
};

#endif