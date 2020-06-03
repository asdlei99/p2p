#include "RtpSource.h"
#include <random>

using namespace asio;

RtpSource::RtpSource(asio::io_service& io_service)
	: io_context_(io_service)
	, io_strand_(io_service)
	, is_alived_(false)
	, fec_decoder_(new fec::FecDecoder)
{

}

RtpSource::~RtpSource()
{
	Close();
}

bool RtpSource::Open(uint16_t rtp_port, uint16_t rtcp_port)
{
	rtp_socket_.reset(new UdpSocket(io_context_));
	rtcp_socket_.reset(new UdpSocket(io_context_));

	if (!rtp_socket_->Open("0.0.0.0", rtp_port) ||
		!rtcp_socket_->Open("0.0.0.0", rtcp_port)) {
		rtp_socket_.reset();
		rtcp_socket_.reset();
		return false;
	}

	std::weak_ptr<RtpSource> rtp_source = shared_from_this();
	rtp_socket_->Receive([rtp_source](void* data, size_t size, asio::ip::udp::endpoint& ep) {
		auto source = rtp_source.lock();
		if (!source) {
			return false;
		}

		if (size == 1) {
			source->is_alived_ = true;
			return true;
		}

		return source->OnRead(data, size);
	});

	return true;
}

bool RtpSource::Open()
{
	std::random_device rd;
	for (int n = 0; n <= 50; n++) {
		if (n == 10) {
			return false;
		}

		uint16_t rtp_port = rd() & 0xfffe;
		uint16_t rtcp_port = rtp_port + 1;

		if (Open(rtp_port, rtcp_port)) {
			break;
		}
	}
	return true;
}

void RtpSource::Close()
{
	if (rtp_socket_) {
		rtp_socket_->Close();
		rtp_socket_.reset();
	}

	if (rtcp_socket_) {
		rtcp_socket_->Close();
		rtcp_socket_.reset();
	}
}

uint16_t RtpSource::GetRtpPort() const
{
	if (rtp_socket_) {
		return rtp_socket_->GetLocalPoint().port();
	}

	return 0;
}

uint16_t RtpSource::GetRtcpPort() const
{
	if (rtcp_socket_) {
		return rtcp_socket_->GetLocalPoint().port();
	}

	return 0;
}

void RtpSource::SetPeerAddress(std::string ip, uint16_t rtp_port, uint16_t rtcp_port)
{
	peer_rtp_address_ = ip::udp::endpoint(ip::address_v4::from_string(ip), rtp_port);
	peer_rtcp_address_ = ip::udp::endpoint(ip::address_v4::from_string(ip), rtcp_port);
	KeepAlive();
}

void RtpSource::KeepAlive()
{
	char empty_packet[1] = {0};

	if (rtp_socket_) {
		rtp_socket_->Send(empty_packet, 1, peer_rtp_address_);
	}

	if (rtcp_socket_) {
		rtcp_socket_->Send(empty_packet, 1, peer_rtcp_address_);
	}
}

bool RtpSource::IsAlive()
{
	return is_alived_;
}

bool RtpSource::OnRead(void* data, size_t size)
{
	if (size < RTP_HEADER_SIZE) {
		return false;
	}

	auto packet = std::make_shared<RtpPacket>();
	packet->SetRtpHeader((uint8_t*)data, RTP_HEADER_SIZE);

	if (size > RTP_HEADER_SIZE) {
		packet->SetPayload((uint8_t*)data + RTP_HEADER_SIZE, (uint32_t)size - RTP_HEADER_SIZE);
	}

	uint8_t  mark = packet->GetMarker();
	uint16_t seq = packet->GetSeq();
	uint32_t timestamp = packet->GetTimestamp();
	//printf("mark:%d, seq:%d, size: %u\n", mark, seq, size);

	auto& packets = frames_[timestamp];
	packets[seq] = packet;

	if (frames_.size() >= 2) {
		OnFrame(frames_.begin()->first);
		frames_.erase(frames_.begin());
	}

	if (mark) {
		return OnFrame(timestamp);
	}

	return true;
}

bool RtpSource::OnFrame(uint32_t timestamp)
{
	if (frames_.empty()) {
		return false;
	}

	if (frames_.find(timestamp) == frames_.end()) {
		return false;
	}

	auto& rtp_packets = frames_[timestamp];

	int out_buf_size = rtp_packets.size() * 1500;
	std::shared_ptr<uint8_t> out_buf(new uint8_t[out_buf_size]);
	int out_data_size = 0;
	int data_index = 0;

	auto first_packet = rtp_packets.begin()->second;
	uint8_t type = first_packet->GetPayloadType();
	uint8_t extension = first_packet->GetExtension();

	if (extension) { //use fec
		fec::FecPackets out_packets;
		for (auto iter : rtp_packets) {
			auto packet = iter.second;
			std::shared_ptr<fec::FecPacket> fec_packet = std::make_shared<fec::FecPacket>();
			int payload_size = packet->GetPayload((uint8_t*)fec_packet.get(), out_buf_size);
			out_packets[fec_packet->header.fec_index] = fec_packet;
		}

		if (out_packets.size() > 0) {
			out_data_size = fec_decoder_->Decode(out_packets, out_buf.get(), out_buf_size);
			if (out_data_size <= 0) {
				out_data_size = -1;
			}
		}
	}
	else {
		for (auto iter : rtp_packets) {
			auto packet = iter.second;
			int payload_size = packet->GetPayload(out_buf.get() + data_index, 1500);
			data_index += payload_size;
			out_data_size += payload_size;
		}
	}

	frames_.erase(timestamp);

	if (out_data_size > 0 && frame_cb_) {
		return frame_cb_(out_buf, out_data_size, type, timestamp);
	}

	return true;
}
