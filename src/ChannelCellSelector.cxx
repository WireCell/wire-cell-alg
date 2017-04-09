#include "WireCellAlg/ChannelCellSelector.h"
#include "WireCellAlg/Predicates.h"
#include "WireCellUtil/NamedFactory.h"

#include <algorithm>
#include <iostream>
#include <sstream>

WIRECELL_FACTORY(ChannelCellSelector, WireCell::ChannelCellSelector,
		 WireCell::IChannelCellSelector, WireCell::IConfigurable);

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
{
}

Configuration ChannelCellSelector::default_configuration() const
{
    Configuration cfg;
    cfg["charge_threshold"] = 0.0;
    cfg["min_wire_coinc"] = 3;
    return cfg;
}

void ChannelCellSelector::configure(const Configuration& cfg)
{
    m_qmin = get<double>(cfg, "charge_threshold", m_qmin);
    m_nmin = get<int>(cfg, "min_wire_coinc", m_nmin);
}

bool ChannelCellSelector::operator()(const input_tuple_type& intup, output_pointer& out)
{
    out = nullptr;

    IChannelSlice::pointer cslice = get<0>(intup);
    ICell::shared_vector all_cells = get<1>(intup);

    if (!cslice || !all_cells) {
	stringstream msg;
	msg <<"ChannelCellSelector: EOS\n";
	cerr << msg.str();
	return true;
    }


    ChannelCharge cc = cslice->charge(); // fixme: copy?
    if (cc.empty()) {
	out = output_pointer(new SimpleCellSlice()); // empty
	stringstream msg;
	msg << "ChannelCellSelector: at t=" << cslice->time()
	    << " no channel charge\n";
	cerr << msg.str();
	return true;
    }

    MaybeHitCell mhc(cc, m_qmin, m_nmin);
    ICell::vector cells;
    std::copy_if(all_cells->begin(), all_cells->end(), back_inserter(cells), mhc);
    if (cells.empty()) {
	stringstream msg;
	msg << "ChannelCellSelector: at t=" << cslice->time()
	    << " no hit cells with " << cc.size() << " hit channels\n";
	cerr << msg.str();

	out = output_pointer(new SimpleCellSlice()); // empty
	return true;
    }

    out = output_pointer(new SimpleCellSlice(cslice->time(), cells));

    {
	stringstream msg;
	msg << "ChannelCellSelector: at t=" << cslice->time()
	    << " cell slice with " << out->cells()->size() << "\n";
	cerr << msg.str();
    }


    return true;
}
