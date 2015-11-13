#ifndef WIRECELLIFACE_CHANNELCELLSELECTOR
#define WIRECELLIFACE_CHANNELCELLSELECTOR

#include "WireCellIface/IChannelCellSelector.h"

#include <deque>

namespace WireCell {

    /** An instance of WireCell::IChannelCellSelector.  
     *
     * See that class for details.
     */
    class ChannelCellSelector : public IChannelCellSelector
    {
    public:
	ChannelCellSelector(double charge_threshold = 0.0,
			    int minimum_number_of_wires = 3)
	    : m_qmin(charge_threshold)
	    , m_nmin(minimum_number_of_wires) { }
	virtual ~ChannelCellSelector() {}

	virtual void set_cells(const ICell::shared_vector& all_cells);

	virtual void reset();

	virtual bool insert(const input_pointer& in);

	virtual bool extract(output_pointer& out);

    private:
	void flush();

	const double m_qmin;
	const int m_nmin;
	std::deque<output_pointer> m_output;
	ICell::shared_vector m_all_cells;
    };
}

#endif

