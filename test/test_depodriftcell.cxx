#include "WireCellAlg/ChannelCellSelector.h"

#include "WireCellGen/BoundCells.h"
#include "WireCellGen/PlaneDuctor.h"
#include "WireCellGen/Drifter.h"
#include "WireCellGen/Diffuser.h"
#include "WireCellGen/Digitizer.h"
#include "WireCellGen/TrackDepos.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"
#include "WireCellGen/Framer.h"

#include "WireCellRootVis/Converters.h"

#include "WireCellIface/WirePlaneId.h"
#include "WireCellIface/IWireSelectors.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Faninout.h"
#include "WireCellUtil/Testing.h"

#include "WireCellRootVis/CanvasApp.h"
#include "WireCellRootVis/Drawers.h"

#include <iostream>
#include <vector>
#include <algorithm>		// count_if

using namespace WireCell;
using namespace std;

TrackDepos make_tracks() {
    TrackDepos td;

    const double cm = 10*units::cm;
    Ray same_point(Point(cm,-cm,cm), Point(10*cm,+cm,30*cm));

    const double usec = units::microsecond;

    td.add_track(1*usec, same_point);
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    //td.add_track(1000*usec, same_point);
    //td.add_track(10000*usec, same_point);

    return td;
}

const double drift_velocity = 1.6*units::millimeter/units::microsecond;

void draw_depos(TVirtualPad& pad, const IDepoVector& orig, const std::vector<IDepoVector>& planes)
{
    pad.Divide(2,2);
    WireCellRootVis::draw2d(*pad.cd(1), orig);
    for (int ind=0; ind<3; ++ind) {
	WireCellRootVis::draw2d(*pad.cd(ind+2), planes[ind]);
    }
}

void dump_pitch(const Ray& pitch)
{
    cerr << "Pitch: " << pitch.first << " --> " << pitch.second << endl;
}

double time_offset(const Ray& pitch)
{
    return pitch.first.x()/drift_velocity;
}

int main(int argc, char *argv[])
{
    WireCellRootVis::CanvasApp app(argv[0], argc>1, 1000,1000);
    app.divide(2,2);

    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);
    dump_pitch(iwp->pitchU());
    dump_pitch(iwp->pitchV());
    dump_pitch(iwp->pitchW());


    WireCellRootVis::draw2d(app.pad(), iwp);

    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWireVector wires;
    Assert(wg.extract(wires));
    Assert(wires.size());
    WireCellRootVis::draw2d(app.pad(), wires);

    const int nwiresU = std::count_if(wires.begin(), wires.end(), select_u_wires);
    const int nwiresV = std::count_if(wires.begin(), wires.end(), select_v_wires);
    const int nwiresW = std::count_if(wires.begin(), wires.end(), select_w_wires);

    BoundCells bc;
    bc.insert(wires);
    ICellVector cells;
    bc.extract(cells);

    cerr << "Make " << cells.size() << " cells" << endl;
    WireCellRootVis::draw2d(app.pad(), cells);

    TrackDepos td = make_tracks();

    // drift

    std::vector<WireCell::Drifter*> drifters = {
	new Drifter(iwp->pitchU().first.x()),
	new Drifter(iwp->pitchV().first.x()),
	new Drifter(iwp->pitchW().first.x())
    };

    // load up drifters all the way
    IDepoVector orig_depo;
    while (true) {
	auto depo = td();
	if (!depo) { break; }
	orig_depo.push_back(depo);
	for (int ind=0; ind<3; ++ind) {
	    Assert(drifters[ind]->insert(depo));
	}
    }
    // and flush them out 
    for (int ind=0; ind<3; ++ind) {
	drifters[ind]->flush();
    }

    // diffuse 

    std::vector<Diffuser*> diffusers = {
	new Diffuser(iwp->pitchU(), tick, time_offset(iwp->pitchU()), now),
	new Diffuser(iwp->pitchV(), tick, time_offset(iwp->pitchV()), now),
	new Diffuser(iwp->pitchW(), tick, time_offset(iwp->pitchW()), now)
    };

    std::vector<IDepoVector> plane_depo(3);
    while (true) {
	int n_ok = 0;
	for (int ind=0; ind < 3; ++ind) {
	    IDepo::pointer depo;
	    if (!drifters[ind]->extract(depo)) {
		continue;
	    }
	    Assert(depo);
	    Assert(diffusers[ind]->insert(depo));
	    plane_depo[ind].push_back(depo);
	    ++n_ok;
	}
	if (n_ok == 0) {
	    break;
	}
	Assert(n_ok == 3);
    }
    for (int ind=0; ind < 3; ++ind) {
	diffusers[ind]->flush();
    }

    cerr << "Total depositions: " << orig_depo.size() << endl;
    draw_depos(app.pad(), orig_depo, plane_depo);

    // collect/induce

    std::vector<PlaneDuctor*> ductors = {
	new PlaneDuctor(WirePlaneId(kUlayer), nwiresU, tick, ray_length(iwp->pitchU()), now),
	new PlaneDuctor(WirePlaneId(kVlayer), nwiresV, tick, ray_length(iwp->pitchV()), now),
	new PlaneDuctor(WirePlaneId(kWlayer), nwiresW, tick, ray_length(iwp->pitchW()), now)
    };
    std::vector<IDiffusionVector> diffusions(3);
    while (true) {
	int n_ok = 0;
	int n_eos = 0;
	for (int ind=0; ind < 3; ++ind) {
	  IDiffusion::pointer diff;
	    if (!diffusers[ind]->extract(diff)) {
		cerr << "Diffuser #"<<ind<<" failed" << endl;
		continue;
	    }
	    ++n_ok;
	    if (!diff) {
		cerr << "Diffuser #"<<ind<<" hits EOS" << endl;
		++n_eos;
		continue;
	    }
	    diffusions[ind].push_back(diff);
	    Assert(ductors[ind]->insert(diff));
	}
	Assert(n_ok == 3);
	Assert(n_eos == 0 || n_eos == 3);
	if (n_eos == 3) {
	    cerr << "Got three EOS from diffusers" << endl;
	    break;
	}
    }

    app.pdf();

    //*** new page ***//

    app.divide(2,2);
    WireCellRootVis::draw2d(app.pad(), diffusions[0]);
    WireCellRootVis::draw2d(app.pad(), diffusions[1]);
    WireCellRootVis::draw2d(app.pad(), diffusions[2]);

    for (int ind=0; ind < 3; ++ind) {
	ductors[ind]->flush();
	//cerr << "Flushed  ductor[" << ind << "] with #inqueue=" << ductors[ind]->ninput()
	//     << " #outqueue=" << ductors[ind]->noutput() << endl;
    }

    Digitizer digitizer;
    digitizer.set_wires(wires);

    bool pd_eos[3] = {false};
    while (true) {
	IPlaneSliceVector psv(3);
	double slice_time = 0;
	int n_eos = 3;
	int n_cruns = 0;
	for (int ind=0; ind<3; ++ind) {
	    if (pd_eos[ind]) {			// already hit EOS on this ductor
		psv[ind] = ductors[ind]->eos(); // null pad
		continue;	// allow all planes to run out
	    }
	    if (!ductors[ind]->extract(psv[ind])) {
		cerr << "ductor extract #"<<ind<<" failed, interpreting as EOS"<<endl;
		pd_eos[ind] = true;
		psv[ind] = ductors[ind]->eos(); // null pad
		continue;	// allow all planes to run out
	    }
	    if (psv[ind] == ductors[ind]->eos()) {
		pd_eos[ind] = true;
		continue;
	    }
	    --n_eos;		// got one
	    n_cruns += psv[ind]->charge_runs().size();
	    slice_time = psv[ind]->time(); // for below
	}
	Assert(digitizer.insert(psv));
	if (n_eos == 3) {
	    cerr << "Got EOS from all three plane ductors" << endl;
	    break;
	}

	if (n_cruns) {
	    cerr << "PlaneSlices: \n";
	    for (int ind=0; ind<3; ++ind) {
		IPlaneSlice::WireChargeRunVector wcrv = psv[ind]->charge_runs();
		cerr << "\t" << ind << ":" << psv[ind]->time() << "/" << wcrv.size()
		     << "[0:" << wcrv[0].first << "/" << wcrv[0].second.size() << "]"
		     << " queues now: in=" << ductors[ind]->ninput()
		     << " out=" << ductors[ind]->noutput()
		     << endl;
	    }
	}

	continue;      // allow all planes to run out until all at EOS
    }
    
    digitizer.flush();

    ChannelCellSelector ccsel(0.0, 3);
    ccsel.set_cells(cells);

    int nccs = 0;
    while (true) {
	IChannelSlice::pointer csp;
	Assert(digitizer.extract(csp));
	if (csp == digitizer.eos()) {
	    cerr << "Digitizer reaches EOS" << endl;
	    break;
	}
	ChannelCharge cc = csp->charge();
	int ncharges = cc.size();
	if (ncharges) {
	    cout << "vvv CUT vvv" << endl;
	    cout << "    double cstime" << nccs << " = " << csp->time() << ";" << endl;
	    cout << "    ChannelCharge cc" << nccs << ";" << endl;
	    for (auto cq: cc) {
		cout << "    cc"<<nccs<<"["<< cq.first << "] = "
		     << "Quantity(" << cq.second.mean() << "," << cq.second.sigma() << ");" << endl;
	    }
	    cout << "^^^ CUT ^^^" << endl;
	    ++nccs;
	}
	Assert(ccsel.insert(csp));
    }

    ccsel.flush();

    while (true) {
	ICellSlice::pointer cellslice;
	Assert(ccsel.extract(cellslice));
	if (cellslice == ccsel.eos()) {
	    cerr << "ChannelCellSelector reaches EOS" << endl;
	    break;
	}
	ICellVector cellsel = cellslice->cells();
	if (cellsel.size()) {
	    cerr << "Selected " << cellsel.size() << " cells at t=" << cellslice->time() << endl;
	}
    }

    app.pdf();
    app.run();
    return 0;
}
