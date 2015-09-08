#ifndef WIRECELL_PREDICATES
#define WIRECELL_PREDICATES

#include "WireCellIface/IChannelSlice.h"
#include "WireCellIface/ICell.h"

namespace WireCell {

    /** A predicate returning true when a cell is associated with some
     * number of wires above some threshold.
     */
    struct MaybeHitCell {
	const WireCell::ChannelCharge& cc;
	const int nmin;
	const double qmin;

	MaybeHitCell(const WireCell::ChannelCharge& cc,
		     double charge_threshold=0.0,
		     int minimum_wires=3);

	/// Return if at least the minimum number of wires are above charge threshold.
	bool operator()(const WireCell::ICell::pointer& cell) const;
    };


}

#endif
