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
			    int minimum_number_of_wires = 3);
	virtual ~ChannelCellSelector() {}

	virtual void set_cells(const ICell::shared_vector& all_cells);

	virtual bool operator()(const input_pointer& in, output_pointer& out);

	virtual int nin() { return m_count; }
	virtual int nout() { return m_count; }

    private:

	const double m_qmin;
	const int m_nmin;
	ICell::shared_vector m_all_cells;
	int m_count;
    };
}

#endif

