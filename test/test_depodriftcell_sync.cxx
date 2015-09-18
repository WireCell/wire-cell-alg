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
#include "WireCellUtil/ExecMon.h"

#include "WireCellRootVis/CanvasApp.h"
#include "WireCellRootVis/Drawers.h"


#include <iostream>
#include <vector>
#include <algorithm>		// count_if

using namespace WireCell;
using namespace std;
using WireCellRootVis::CanvasApp;

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

// prepare a plane ductor
IPlaneDuctor::pointer make_ductor(const Ray& pitch,
				  WirePlaneLayer_t layer,
				  const IWireVector& wires,
				  double tick, double t0=0.0)
{
    WirePlaneId wpid(layer);
    cerr<<"make_ductor(pitch=" << pitch << ",wpid=" << wpid << ")" << endl;

    const double pitch_distance = ray_length(pitch);
    const Vector pitch_unit = ray_unit(pitch);

    // get this planes wires sorted by index
    IWireVector plane_wires;
    std::copy_if(wires.begin(), wires.end(),
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

    return IPlaneDuctor::pointer(new PlaneDuctor(wpid, nwires, tick, pitch_distance, t0, wire_zero_dist));
}


void dump(CanvasApp& app, IWireParameters::pointer iwp)
{

    dump_pitch(iwp->pitchU());
    dump_pitch(iwp->pitchV());
    dump_pitch(iwp->pitchW());
    WireCellRootVis::draw2d(app.clear(), iwp);
    app.pdf();
}


void dump(CanvasApp& app, const IWireVector& wires)
{
    WireCellRootVis::draw2d(app.clear(), wires);
    app.pdf();
}
void dump(CanvasApp& app, const ICellVector& cells)
{
    cerr << "Make " << cells.size() << " cells" << endl;
    WireCellRootVis::draw2d(app.pad(), cells);
}

void dump(CanvasApp& app, const IDepoVector& depos)
{
    WireCellRootVis::draw2d(app.clear(), depos);
}

void dump(CanvasApp& app, const IDiffusionVector& diffs)
{
    WireCellRootVis::draw2d(app.clear(), diffs);
    app.pdf();
}

void dump(CanvasApp& app, const IPlaneSliceVector& psv)
{
}
void dump(CanvasApp& app, const IChannelSliceVector& csv)
{
}

void dump(CanvasApp& app, const ICellSliceVector& cell_slices)
{
    WireCellRootVis::draw3d(app.clear(), cell_slices);
    app.pdf();
    for (auto cs : cell_slices) {
	WireCellRootVis::draw2d(app.clear(), cs->cells());
	app.pdf();
    }
}

IWireParameters::pointer do_wireparameters()
{
    IWireParameters::pointer iwp(new WireParams);
    return iwp;
}

IWireVector do_wires(IWireParameters::pointer iwp)
{
    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWireVector wires;
    Assert(wg.extract(wires));
    Assert(!wires.empty());
    return wires;
}

ICellVector do_cells(const IWireVector& wires)
{
    BoundCells bc;
    Assert(bc.insert(wires));
    ICellVector cells;
    Assert(bc.extract(cells));
    Assert(!cells.empty());
    return cells;
}

IDepoVector do_deposition()
{
    TrackDepos td = make_tracks();
    IDepoVector depos;
    while (true) {
	auto depo = td();
	if (!depo) { break; }
	depos.push_back(depo);
    }
    return depos;
}

IDepoVector do_drift(const IDepoVector& activity, double to_x)
{
    Drifter drifter(to_x);


    for (auto depo : activity) { // fully load
	Assert(drifter.insert(depo));
    }

    // fully process
    drifter.flush();

    // fully drain
    IDepoVector drifted;
    while (true) {
	IDepo::pointer depo;
	Assert(drifter.extract(depo));
	if (depo == drifter.eos()) {
	    break;
	}
	drifted.push_back(depo);
    }

    return drifted;
}

IDiffusionVector do_diffusion(const IDepoVector& depos, const Ray& pitch, double tick, double now)
{
    Diffuser diffuser(pitch, tick, time_offset(pitch), now);

    for (auto depo : depos) {	// fully load
	Assert(diffuser.insert(depo));
    }

    // fully process
    diffuser.flush();

    // fully drain
    IDiffusionVector diffused;
    while (true) {
	IDiffusion::pointer diff;
	Assert(diffuser.extract(diff));
	if (diff == diffuser.eos()) {
	    break;
	}
	diffused.push_back(diff);
    }
    return diffused;
}

IPlaneSliceVector do_ductor(const IWireVector& wires,
			    const IDiffusionVector& diffused, const Ray& pitch,
			    WirePlaneLayer_t layer, double tick, double now)
{
    IPlaneDuctor::pointer ductor = make_ductor(pitch, layer, wires, tick, now);

    for (auto diff : diffused) { // fully load
	Assert(ductor->insert(diff));
    }

    // fully process
    ductor->flush();

    // fully drain;
    IPlaneSliceVector psv;
    while (true) {
	IPlaneSlice::pointer ps;
	Assert(ductor->extract(ps));
	if (ps == ductor->eos()) {
	    break;
	}
	psv.push_back(ps);
    }
    return psv;	
}

IChannelSliceVector do_digitizer(const IWireVector& wires,
				 const std::vector<IPlaneSliceVector> psvs_byplane)
{
    Digitizer digitizer;
    digitizer.set_wires(wires);

    // repackage and fully load up
    int islice = 0;
    while (true) {
	IPlaneSliceVector psv(3);
	int n_filled = 0;
	for (int ind=0; ind<3; ++ind) {
	    if (islice < psvs_byplane[ind].size()) {
		psv[ind] = psvs_byplane[ind][islice];
		++n_filled;
	    }
	}
	++islice;
	if (!n_filled) {
	    break;
	}
	Assert(digitizer.insert(psv));
    }

    digitizer.flush();

    IChannelSliceVector frame;
    while (true) {
	IChannelSlice::pointer csp;
	Assert(digitizer.extract(csp));
	if (csp == digitizer.eos()) {
	    break;
	}
	frame.push_back(csp);
    }
    return frame;
}


ICellSliceVector do_channelcellselector(const ICellVector& cells, const IChannelSliceVector& frame)
{
    ChannelCellSelector ccsel(0.0, 3);
    ccsel.set_cells(cells);

    // load up
    for (auto cs : frame) {
	Assert(ccsel.insert(cs));
    }

    ccsel.flush();

    ICellSliceVector cell_slices;
    while (true) {
	ICellSlice::pointer cellslice;
	Assert(ccsel.extract(cellslice));
	if (cellslice == ccsel.eos()) {
	    break;
	}
	cell_slices.push_back(cellslice);
    }
    return cell_slices;
}


int main(int argc, char *argv[])
{
    CanvasApp app(argv[0], argc>1, 1000,1000);
    ExecMon em(argv[0]);

    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp = do_wireparameters();
    dump(app, iwp);
    cerr << em("made wire pa rams") << endl;


    IWireVector wires = do_wires(iwp);    
    dump(app, wires);
    cerr << em("made wires") << endl;

    ICellVector cells = do_cells(wires);
    dump(app, cells);
    cerr << em("made cells") << endl;

    IDepoVector activity = do_deposition();
    dump(app, activity);
    cerr << em("made activity") << endl;

    std::vector<IDepoVector> plane_depos = {
	do_drift(activity, iwp->pitchU().first.x()),
	do_drift(activity, iwp->pitchV().first.x()),
	do_drift(activity, iwp->pitchW().first.x())
    };
    for (auto depo : plane_depos) {
	dump(app, depo);
    }
    cerr << em("drifted") << endl;

    // diffuse 
    std::vector<IDiffusionVector> diffused = {
	do_diffusion(plane_depos[0], iwp->pitchU(), tick, now),
	do_diffusion(plane_depos[1], iwp->pitchV(), tick, now),
	do_diffusion(plane_depos[2], iwp->pitchW(), tick, now)
    };
    for (auto diff : diffused) {
	dump(app, diff);
    }
    cerr << em("diffused") << endl;

    // collect/induce
    std::vector<IPlaneSliceVector> psvs_byplane = {
	do_ductor(wires, diffused[0], iwp->pitchU(), kUlayer, tick, now),
	do_ductor(wires, diffused[1], iwp->pitchV(), kVlayer, tick, now),
	do_ductor(wires, diffused[2], iwp->pitchW(), kWlayer, tick, now)
    };
    for (auto psv : psvs_byplane) {
	dump(app, psv);
    }
    cerr << em("sliced") << endl;

    IChannelSliceVector frame = do_digitizer(wires, psvs_byplane);
    dump(app, frame);
    cerr << em("digitized") << endl;

    ICellSliceVector cell_slices = do_channelcellselector(cells, frame);
    dump(app, cell_slices);
    cerr << em("selected cells") << endl;

    cerr << em.summary() << endl;

    app.run();
    return 0;
}
