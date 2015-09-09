#include "WireCellAlg/ChannelCellSelector.h"
#include "WireCellAlg/Predicates.h"

#include <algorithm>
#include <iostream>

using namespace WireCell;
using namespace std;

// fixme: move into iface/Simple
class SimpleCellSlice : public ICellSlice {
    double m_time;
    ICellVector m_cells;
public:
    SimpleCellSlice(double t, const ICellVector& c)
	: m_time(t), m_cells(c) { }
    virtual ~SimpleCellSlice() {}
    
    virtual double time() const { return m_time; }

    virtual ICellVector cells() const { return m_cells; }
};

void ChannelCellSelector::set_cells(const ICellVector& all_cells)
{
    m_all_cells = all_cells;
}

void ChannelCellSelector::reset()
{
    m_output.clear();		// pew pew
}
void ChannelCellSelector::flush()
{
    m_output.push_back(eos());
}
bool ChannelCellSelector::insert(const input_type& in)
{
    if (m_all_cells.empty()) {
	return false;
    }

    ChannelCharge cc = in->charge(); // fixme: copy?
    if (cc.empty()) {
	return true;
    }

    MaybeHitCell mhc(cc, m_qmin, m_nmin);
    ICellVector cells;
    std::copy_if(m_all_cells.begin(), m_all_cells.end(), back_inserter(cells), mhc);

    m_output.push_back(ICellSlice::pointer(new SimpleCellSlice(in->time(), cells)));
    cerr << "ChannelCellSelector: found " << cells.size() << " cells at t="
	 << in->time() << endl;

    return true;
}
bool ChannelCellSelector::extract(output_type& out)
{
    if (m_output.empty()) {
	return false;
    }
    out = m_output.front();
    m_output.pop_front();
    return true;
}

