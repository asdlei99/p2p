#ifndef RTP_SINK_H
#define RTP_SINK_H

#include "UdpSocket.h"
#include "RtpPacket.hpp"
#include "option.hpp"
#include "fec/fec.h"
#include <random>

class RtpSink : public std::enable_shared_from_this<RtpSink>, public Option
{
public:
	RtpSink& operator=(const RtpSink&) = delete;
	RtpSink(const RtpSink&) = delete;
	RtpSink(asio::io_service& io_service);
	virtual ~RtpSink();

	bool Open(uint16_t rtp_port, uint16_t rtcp_port);
	bool Open();
	void Close();

	void SetPeerAddress(std::string ip, uint16_t rtp_port, uint16_t rtcp_port);

	bool SendFrame(std::shared_ptr<uint8_t> data, uint32_t size, uint8_t type, uint32_t timestamp);

	uint16_t GetRtpPort()  const;
	uint16_t GetRtcpPort() const;

private:
	void HandleFrame(std::shared_ptr<uint8_t> data, uint32_t size, uint8_t type, uint32_t timestamp);
	void SendRtp(std::shared_ptr<uint8_t>& data, uint32_t& size, uint8_t& type, uint32_t& timestamp);
	void SendRtpOverFEC(std::shared_ptr<uint8_t>& data, uint32_t& size, uint8_t& type, uint32_t& timestamp);

	asio::io_context& io_context_;
	asio::io_context::strand io_strand_;

	std::unique_ptr<UdpSocket> rtp_socket_;
	std::unique_ptr<UdpSocket> rtcp_socket_;
	asio::ip::udp::endpoint peer_rtp_address_;
	asio::ip::udp::endpoint peer_rtcp_address_;

	std::unique_ptr<RtpPacket> rtp_packet_;
	std::unique_ptr<fec::FecEncoder> fec_encoder_;

	std::random_device rand;

	uint32_t packet_seq_ = 0;
	uint32_t packet_size_ = 1024;
};

#endif
