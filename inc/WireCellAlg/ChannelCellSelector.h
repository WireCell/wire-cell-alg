#ifndef WIRECELLIFACE_CHANNELCELLSELECTOR
#define WIRECELLIFACE_CHANNELCELLSELECTOR

#include "WireCellIface/IChannelCellSelector.h"
#include "WireCellIface/IConfigurable.h"

#include <deque>

namespace WireCell {

    /** An instance of WireCell::IChannelCellSelector.  
     *
     * See that class for details.
     */
    class ChannelCellSelector : public IChannelCellSelector, public IConfigurable
    {
    public:
	ChannelCellSelector(double charge_threshold = 0.0,
			    int minimum_number_of_wires = 3);
	virtual ~ChannelCellSelector() {}

	virtual bool operator()(const input_tuple_type& intup, output_pointer& out);

	virtual void configure(const WireCell::Configuration& config);
	virtual WireCell::Configuration default_configuration() const;

    private:

	double m_qmin;
	int m_nmin;
    };
}

#endif

