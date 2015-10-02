#include "WireCellAlg/ChannelCellSelector.h"
#include "WireCellAlg/Predicates.h"

#include <algorithm>
#include <iostream>

using namespace WireCell;
using namespace std;

// fixme: move into iface/Simple
class SimpleCellSlice : public ICellSlice {
    double m_time;
    ICell::shared_vector m_cells;
public:
    SimpleCellSlice(double t, const ICell::vector& c)
	: m_time(t), m_cells(new ICell::vector(c)) { }
    virtual ~SimpleCellSlice() {}
    
    virtual double time() const { return m_time; }

    virtual ICell::shared_vector cells() const { return m_cells; }
};

void ChannelCellSelector::set_cells(const ICell::shared_vector& all_cells)
{
    m_all_cells = all_cells;
}

void ChannelCellSelector::reset()
{
    m_output.clear();		// pew pew
}
void ChannelCellSelector::flush()
{
    m_output.push_back(nullptr);
}
bool ChannelCellSelector::insert(const input_type& in)
{
    if (!in) {
	flush();
	return true;
    }

    if (m_all_cells->empty()) {
	return false;
    }

    ChannelCharge cc = in->charge(); // fixme: copy?
    if (cc.empty()) {
	return true;
    }

    MaybeHitCell mhc(cc, m_qmin, m_nmin);
    ICell::vector cells;
    std::copy_if(m_all_cells->begin(), m_all_cells->end(), back_inserter(cells), mhc);
    if (cells.empty()) {
	cerr << "ChannelCellSelector: no cells found at t=" << in->time() << endl;
	return true;
    }

    m_output.push_back(ICellSlice::pointer(new SimpleCellSlice(in->time(), cells)));

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

