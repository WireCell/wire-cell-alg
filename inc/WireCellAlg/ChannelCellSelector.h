#ifndef WIRECELLIFACE_CHANNELCELLSELECTOR
#define WIRECELLIFACE_CHANNELCELLSELECTOR

#include "WireCellIface/IChannelCellSelector.h"
#include "WireCellIface/IBuffering.h"

#include <deque>

namespace WireCell {

    /** An instance of WireCell::IChannelCellSelector.  
     *
     * See that class for details.
     */
    class ChannelCellSelector : public IChannelCellSelector, public IBuffering
    {
    public:
	ChannelCellSelector(double charge_threshold = 0.0,
			    int minimum_number_of_wires = 3)
	    : m_qmin(charge_threshold)
	    , m_nmin(minimum_number_of_wires) { }
	virtual ~ChannelCellSelector() {}

	virtual void set_cells(const ICell::shared_vector& all_cells);

	virtual void reset();

	virtual void flush();

	virtual bool insert(const input_type& in);

	virtual bool extract(output_type& out);

    private:
	double m_qmin;
	int m_nmin;
	std::deque<output_type> m_output;
	ICell::shared_vector m_all_cells;
    };
}

#endif

