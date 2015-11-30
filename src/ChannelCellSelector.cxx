#include "WireCellAlg/ChannelCellSelector.h"
#include "WireCellAlg/Predicates.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace WireCell;
using namespace std;

// fixme: move into iface/Simple
class SimpleCellSlice : public ICellSlice {
    double m_time;
    ICell::shared_vector m_cells;
public:
    SimpleCellSlice() : m_time(0) {}
    SimpleCellSlice(double t, const ICell::vector& c)
	: m_time(t), m_cells(new ICell::vector(c)) { }
    virtual ~SimpleCellSlice() {}
    
    virtual double time() const { return m_time; }

    virtual ICell::shared_vector cells() const { return m_cells; }
};

ChannelCellSelector::ChannelCellSelector(double charge_threshold,
					 int minimum_number_of_wires)
    : m_qmin(charge_threshold)
    , m_nmin(minimum_number_of_wires)
    , m_count(0)
{
}

void ChannelCellSelector::set_cells(const ICell::shared_vector& all_cells)
{
    m_all_cells = all_cells;
}

bool ChannelCellSelector::operator()(const input_pointer& in, output_pointer& out)
{
    ++m_count;
    out = nullptr;
    if (m_all_cells->empty()) {
	cerr << "ChannelCellSelector: " << m_count << " no cells\n";
	return false;
    }

    if (!in) {
	stringstream msg;
	msg <<"ChannelCellSelector: " << m_count << " " << "nullptr input\n";
	cerr << msg.str();
	return true;
    }



    ChannelCharge cc = in->charge(); // fixme: copy?
    if (cc.empty()) {
	out = output_pointer(new SimpleCellSlice()); // empty
	stringstream msg;
	msg << "ChannelCellSelector: "  << m_count << " at t=" << in->time()
	    << " no channel charge\n";
	cerr << msg.str();
	return true;
    }

    MaybeHitCell mhc(cc, m_qmin, m_nmin);
    ICell::vector cells;
    std::copy_if(m_all_cells->begin(), m_all_cells->end(), back_inserter(cells), mhc);
    if (cells.empty()) {
	stringstream msg;
	msg << "ChannelCellSelector: "  << m_count << " at t=" << in->time()
	    << " no hit cells with " << cc.size() << " hit channels\n";
	cerr << msg.str();

	out = output_pointer(new SimpleCellSlice()); // empty
	return true;
    }

    out = output_pointer(new SimpleCellSlice(in->time(), cells));

    {
	stringstream msg;
	msg << "ChannelCellSelector: "  << m_count << " at t=" << in->time()
	    << " cell slice with " << out->cells()->size() << "\n";
	cerr << msg.str();
    }


    return true;
}
