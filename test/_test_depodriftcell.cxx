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

    const double cm = units::cm;
    Ray same_point(Point(cm,-cm,cm), Point(10*cm,+cm,30*cm));

    const double usec = units::microsecond;

    td.add_track(10*usec, Ray(Point(cm,0,0), Point(2*cm,0,0)));
    td.add_track(20*usec, Ray(Point(cm,0,0), Point(cm,0,cm)));
    td.add_track(30*usec, Ray(Point(cm,-cm,-cm), Point(cm,cm,cm)));
    td.add_track(10*usec, same_point);
    td.add_track(100*usec, same_point);
    //td.add_track(1000*usec, same_point);
    //td.add_track(10000*usec, same_point);

    Assert(td.depos().size() > 1);

    return td;
}

const double drift_velocity = 1.6*units::millimeter/units::microsecond;

void draw_depos(TVirtualPad& pad, const IDepo::vector& orig, const std::vector<IDepo::vector>& planes)
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

// prepare a plane ductor
PlaneDuctor* make_ductor(const Ray& pitch,
			 WirePlaneLayer_t layer,
			 const IWire::shared_vector& wires,
			 double tick, double t0=0.0)
{
    WirePlaneId wpid(layer);
    cerr<<"make_ductor(pitch=" << pitch << ",wpid=" << wpid << ")" << endl;

    const double pitch_distance = ray_length(pitch);
    const Vector pitch_unit = ray_unit(pitch);

    // get this planes wires sorted by index
    IWire::vector plane_wires;
    std::copy_if(wires->begin(), wires->end(),
		 back_inserter(plane_wires), select_uvw_wires[wpid.index()]);
    std::sort(plane_wires.begin(), plane_wires.end(), ascending_index);

    // number of wires and location of wire zero measured in pitch coordinate.
    const int nwires = plane_wires.size();
    IWire::pointer wire_zero = plane_wires[0];
    const Point to_wire = wire_zero->center() - pitch.first;
    const double wire_zero_dist = pitch_unit.dot(to_wire);

    // cerr << "Wire0 for plane=" << wpid.ident() << " distance=" << pitch_distance <<  " index=" << wire_zero->index() << endl;
    // cerr << "\twire ray=" << wire_zero->ray() << endl;
    // cerr << "\twire0 center=" << wire_zero->center() << endl;
    // cerr << "\tto wire=" << to_wire << endl;
    // cerr << "\tpitch = " << pitch_unit << endl;
    // cerr << "\twire0 dist=" << wire_zero_dist << endl;

    return new PlaneDuctor(wpid, nwires, tick, pitch_distance, t0, wire_zero_dist);
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


    WireCellRootVis::draw2d(app.pad(), *iwp);

    WireGenerator wg;
    IWire::shared_vector wires;
    Assert(wg(iwp, wires));
    Assert(wires);
    Assert(wires->size());
    WireCellRootVis::draw2d(app.pad(), *wires);

    BoundCells bc;
    ICell::shared_vector cells;
    Assert(bc(wires, cells));
    Assert(cells);

    cerr << "Make " << cells->size() << " cells" << endl;
    WireCellRootVis::draw2d(app.pad(), *cells);

    TrackDepos td = make_tracks();

    // drift

    std::vector<WireCell::Drifter*> drifters = {
	new Drifter(iwp->pitchU().first.x()),
	new Drifter(iwp->pitchV().first.x()),
	new Drifter(iwp->pitchW().first.x())
    };

    // load up drifters all the way
    IDepo::vector orig_depo;
    while (true) {
	IDepo::pointer depo;
	Assert(td.extract(depo));
	for (int ind=0; ind<3; ++ind) {
	    Assert(drifters[ind]->insert(depo));
	}
	if (depo) {
	    cerr << "Deposition: " << depo->time() << " --> " << depo->pos() << endl;
	    orig_depo.push_back(depo);
	    continue;
	}
	cerr << "No Deposition\n";
	break;
    }
    Assert(orig_depo.size() > 0);


    // diffuse 

    std::vector<Diffuser*> diffusers = {
	new Diffuser(iwp->pitchU(), tick, time_offset(iwp->pitchU()), now),
	new Diffuser(iwp->pitchV(), tick, time_offset(iwp->pitchV()), now),
	new Diffuser(iwp->pitchW(), tick, time_offset(iwp->pitchW()), now)
    };

    std::vector<IDepo::vector> plane_depo(3);
    bool flow[3] = {true,true,true};
    while (flow[0] || flow[1] || flow[2]) {
	for (int ind=0; ind < 3; ++ind) {
	    if (!flow[ind]) {
		//cerr << "Skip drifter " << ind << " at EOS" << endl; 
		continue;
	    }
	    IDepo::pointer depo;
	    Assert(drifters[ind]->extract(depo));
	    Assert(diffusers[ind]->insert(depo));	    
	    if (!depo) {
		cerr << "EOS from drifter " << ind << endl;
		flow[ind] = false;
		continue;
	    }
	    plane_depo[ind].push_back(depo); // save for plotting
	}
    }

    cerr << "Total depositions: " << orig_depo.size() << endl;
    draw_depos(app.pad(), orig_depo, plane_depo);

    // collect/induce

    std::vector<PlaneDuctor*> ductors = {
	make_ductor(iwp->pitchU(), kUlayer, wires, tick, now),
	make_ductor(iwp->pitchV(), kVlayer, wires, tick, now),
	make_ductor(iwp->pitchW(), kWlayer, wires, tick, now)
    };

    std::vector<IDiffusion::vector> diffusions(3);
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

    assert(diffusions[0].size() > 0);

    app.pdf();

    //*** new page ***//

    app.divide(2,2);
    WireCellRootVis::draw2d(app.pad(), diffusions[0]);
    WireCellRootVis::draw2d(app.pad(), diffusions[1]);
    WireCellRootVis::draw2d(app.pad(), diffusions[2]);

    Digitizer digitizer;
    digitizer.set_wires(wires);

    ChannelCellSelector ccsel(0.0, 3);
    ccsel.set_cells(cells);

    bool pd_eos[3] = {false};
    while (true) {
	IPlaneSlice::vector psv(3);
	double slice_time = 0;
	int n_eos = 3;
	int n_cruns = 0;
	for (int ind=0; ind<3; ++ind) {
	    if (pd_eos[ind]) {			// already hit EOS on this ductor
		psv[ind] = nullptr;
		continue;	// allow all planes to run out
	    }
	    if (!ductors[ind]->extract(psv[ind])) {
		cerr << "ductor extract #"<<ind<<" failed, interpreting as EOS"<<endl;
		pd_eos[ind] = true;
		continue;	// allow all planes to run out
	    }
	    if (!psv[ind]) {
		pd_eos[ind] = true;
		continue;
	    }
	    --n_eos;		// got one
	    n_cruns += psv[ind]->charge_runs().size();
	    slice_time = psv[ind]->time(); // for below
	}

	IPlaneSlice::shared_vector plane_slice_vector;
	if (n_eos == 3) {
	    cerr << "EOS from all wire planes\n";
	}
	else {
	    plane_slice_vector = IPlaneSlice::shared_vector(new IPlaneSlice::vector(psv));
	}

	IChannelSlice::pointer csp;
	bool ok = digitizer(plane_slice_vector, csp);
	Assert(ok);
	if (!plane_slice_vector) {
	    Assert(!csp);
	}
			    
	ICellSlice::pointer cellslice;
	Assert(ccsel(csp, cellslice));

	if (!csp) {
	    Assert(!cellslice);
	    cerr << "ChannelCellSelector: EOS\n";
	}
	

	if (!cellslice) {
	    cerr << "EOS at end of DFP graph\n";
	    break;
	    // if this was not last in the chain, then we would forward it
	}

	ICell::shared_vector cellsel = cellslice->cells();
	if (!cellsel) {
	    cerr << "Got empty cell slice\n";
	    continue;
	}

	if (cellsel->size()) {
	    cerr << "Selected " << cellsel->size() << " cells at t=" << cellslice->time() << endl;
	}

	continue;      // allow all planes to run out until all at EOS
    }


    


    app.pdf();
    app.run();
    return 0;
}
