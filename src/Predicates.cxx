#include "WireCellAlg/Predicates.h"
#include "WireCellUtil/Quantity.h"

#include <iostream>		// debug

using namespace std;
using namespace WireCell;

MaybeHitCell::MaybeHitCell(const WireCell::ChannelCharge& cc,
			   double charge_threshold,
			   int minimum_wires)
    : cc(cc), qmin(charge_threshold), nmin(minimum_wires)
{
}

bool MaybeHitCell::operator()(const WireCell::ICell::pointer& cell) const 
{
    // std::cerr << "MaybeHitCell: cell id:" << cell->ident() 
    // 	      << " with: " << cell->wires().size() << " wires"
    // 	      << std::endl;
    int count = 0;
    for (auto wire : cell->wires()) {
	const int ch = wire->channel();
	auto hit = cc.find(ch);
	if (hit == cc.end()) {
	    //cerr << "\tskipping wire with channel #" << ch << endl;
	    continue;
	}
	if (hit->second < qmin) {
	    cerr << "\tskipping wire with channel #" << ch
	        << " low charge " << hit->second << " < " << qmin << endl;
	    continue;
	}
	// cerr << "MaybeHitCell: cell id: " << cell->ident()
	//      << " accepting wire with channel #" << ch 
	//      << " charge " << hit->second << " >= " << qmin << endl;
	++count;
    }


    // if (count > 1) {
    // 	cerr << "MaybeHitCell: cell id: " << cell->ident() << " with wires:";
    // 	for (auto wire : cell->wires()) {
    // 	    const int ch = wire->channel();
    // 	    auto hit = cc.find(ch);
    // 	    Quantity q;
    // 	    if (hit != cc.end()) {
    // 		q = hit->second;
    // 	    }
    // 	    cerr << " #" << ch << "/q=" << q;
    // 	}
    // 	cerr << endl;
    // }

    return count >= nmin;
}

