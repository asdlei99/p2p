#ifndef FEC_CODEC_H_
#define FEC_CODEC_H_

#include <cstdint>
#include <memory>
#include <map>

namespace fec
{

static const int MAX_FEC_PAYLOAD_SIZE = 1392;

#pragma pack(push)
#pragma pack(4)
struct FecHeader
{
	uint64_t fec_timestamp;
	uint32_t fec_index;	
	uint16_t fec_percentage;
	uint16_t fec_payload_size;

	uint16_t is_parity_shard;
	uint16_t max_payload_size;

	uint32_t total_size;
};
#pragma pack(pop)

struct FecPacket
{
	FecHeader header;
	uint8_t   payload[MAX_FEC_PAYLOAD_SIZE];
};

typedef std::map<uint32_t, std::shared_ptr<FecPacket>> FecPackets;

class FecCodec
{
public:
	FecCodec();
	virtual ~FecCodec();

protected:

};

class FecEncoder : public FecCodec
{
public:
	FecEncoder();
	virtual ~FecEncoder();

	void SetPacketSize(uint32_t packet_size);
	void SetPercentage(uint32_t percentage);

	int Encode(uint8_t *in_data, uint32_t in_size, FecPackets& out_packets);

private:
	uint32_t fec_perc_ = 5;
	uint32_t packet_size_ ;
	uint32_t payload_size_;
};

class FecDecoder : public FecCodec
{
public:
	int Decode(FecPackets& in_packets, uint8_t *out_buf, uint32_t max_out_buf_size);
};

}


#endif