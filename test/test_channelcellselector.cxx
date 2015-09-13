#include "WireCellAlg/ChannelCellSelector.h"

#include "WireCellGen/BoundCells.h"
#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/Digitizer.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/Framer.h"

#include "WireCellIface/WirePlaneId.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Faninout.h"
#include "WireCellUtil/Testing.h"

#include <iostream>
#include <boost/signals2.hpp>
#include <typeinfo>
#include <vector>

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;

    const double cm = units::cm;
    Ray same_point(Point(cm,-cm,cm), Point(cm,+cm,+cm));

    const double usec = units::microsecond;

    td.add_track(1*usec, same_point);
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    td.add_track(1000*usec, same_point);
    td.add_track(10000*usec, same_point);

    return td;
}


int main()
{
    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);

    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWireVector wires;
    Assert(wg.extract(wires));
    Assert(wires.size());

    BoundCells bc;
    bc.insert(wires);
    ICellVector cells;
    bc.extract(cells);

    TrackDepos td = make_tracks();

    // drift

    std::vector<WireCell::Drifter*> drifters = {
	new Drifter(iwp->pitchU().first.x()),
	new Drifter(iwp->pitchV().first.x()),
	new Drifter(iwp->pitchW().first.x())
    };

    // load up drifters all the way
    while (true) {
	auto depo = td();
	if (!depo) { break; }
	for (int ind=0; ind<3; ++ind) {
	    Assert(drifters[ind]->insert(depo));
	}
    }
    // and flush them out 
    for (int ind=0; ind<3; ++ind) {
	drifters[ind]->flush();
    }

    AssertMsg(false, "Finish this test");

    // diffuse + collect/induce

    // std::vector<PlaneDuctor*> ductors = {
    // 	new PlaneDuctor(WirePlaneId(kUlayer), iwp->pitchU(), tick, now),
    // 	new PlaneDuctor(WirePlaneId(kVlayer), iwp->pitchV(), tick, now),
    // 	new PlaneDuctor(WirePlaneId(kWlayer), iwp->pitchW(), tick, now)
    // };
    // while (true) {
    // 	int n_ok = 0;
    // 	for (int ind=0; ind < 3; ++ind) {
    // 	    IDepo::pointer depo;
    // 	    if (!drifters[ind]->extract(depo)) {
    // 		continue;
    // 	    }
    // 	    Assert(depo);
    // 	    Assert(ductors[ind]->insert(depo));
    // 	    ++n_ok;
    // 	}
    // 	if (n_ok == 0) {
    // 	    break;
    // 	}
    // 	Assert(n_ok == 3);
    // }
    // for (int ind=0; ind < 3; ++ind) {
    // 	ductors[ind]->flush();
    // }

    // Digitizer digitizer;
    // digitizer.set_wires(wires);

    // while (true) {
    // 	IPlaneSliceVector psv(3);
    // 	int n_ok = 0;
    // 	int n_eos = 0;
    // 	for (int ind=0; ind<3; ++ind) {
    // 	    if (!ductors[ind]->extract(psv[ind])) {
    // 		cerr << "ductor #"<<ind<<"failed"<<endl;
    // 		continue;
    // 	    }
    // 	    ++n_ok;
    // 	    if (psv[ind] == ductors[ind]->eos()) {
    // 		++n_eos;
    // 	    }
    // 	}
    // 	if (n_ok == 0) {
    // 	    cerr << "Got no channel slices from plane ductors" << endl;
    // 	    break;
    // 	}
    // 	Assert(n_ok == 3);

    // 	Assert(n_eos == 0 || n_eos == 3);
    // 	if (n_eos == 3) {
    // 	    cerr << "Got three EOS from plane ductors" << endl;
    // 	    break;
    // 	}

    // 	Assert(digitizer.insert(psv));
    // }
    
    // digitizer.flush();

    // ChannelCellSelector ccsel(0.0, 3);
    // ccsel.set_cells(cells);

    // while (true) {
    // 	IChannelSlice::pointer csp;
    // 	Assert(digitizer.extract(csp));
    // 	if (csp == digitizer.eos()) {
    // 	    cerr << "Digitizer reaches EOS" << endl;
    // 	    break;
    // 	}
    // 	Assert(ccsel.insert(csp));
    // }

    // ccsel.flush();

    // while (true) {
    // 	ICellSlice::pointer cellslice;
    // 	Assert(ccsel.extract(cellslice));
    // 	if (cellslice == ccsel.eos()) {
    // 	    cerr << "ChannelCellSelector reaches EOS" << endl;
    // 	    break;
    // 	}
    // 	ICellVector cellsel = cellslice->cells();

    // 	cerr << cellsel.size() << " cells at t=" << cellslice->time() << endl;
    // }

    return 0;
}
