#ifndef OPTION_H
#define OPTION_H

#include <cstdint>

// fec percentage: 5%, 10%, 20% ...
static const int OPT_SET_FEC_PERC          = 0x01;

static const int OPT_SET_PACKET_LOSS_PERC  = 0x02;

class Option
{
public:
	virtual void SetOption(int opt, int value)
	{
		switch (opt)
		{
		case OPT_SET_FEC_PERC:
			fec_perc_ = value;
			use_fec_ = (fec_perc_ > 0) ? 1 : 0;
			break;

		case OPT_SET_PACKET_LOSS_PERC:
			packet_loss_perc_ = value;
			break;

		default:
			break;
		}
	}

protected:
	uint32_t fec_perc_ = 0;
	uint32_t use_fec_ = 0;
	uint32_t packet_loss_perc_ = 0;
};

#endif