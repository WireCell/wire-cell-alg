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
IPlaneDuctor::pointer make_ductor(const Ray& pitch,
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

    return IPlaneDuctor::pointer(new PlaneDuctor(wpid, nwires, tick, pitch_distance, t0, wire_zero_dist));
}


void dump(CanvasApp& app, const IWireParameters& iwp)
{

    dump_pitch(iwp.pitchU());
    dump_pitch(iwp.pitchV());
    dump_pitch(iwp.pitchW());
    WireCellRootVis::draw2d(app.clear(), iwp);
    app.pdf();
}


void dump(CanvasApp& app, const IWire::vector& wires)
{
    WireCellRootVis::draw2d(app.clear(), wires);
    app.pdf();
}
void dump(CanvasApp& app, const ICell::vector& cells)
{
    cerr << "Make " << cells.size() << " cells" << endl;
    WireCellRootVis::draw2d(app.pad(), cells);
}

void dump(CanvasApp& app, const IDepo::vector& depos)
{
    WireCellRootVis::draw2d(app.clear(), depos);
}

void dump(CanvasApp& app, const IDiffusion::vector& diffs)
{
    WireCellRootVis::draw2d(app.clear(), diffs);
    app.pdf();
}

void dump(CanvasApp& app, const IPlaneSlice::vector& psv)
{
}
void dump(CanvasApp& app, const IChannelSlice::vector& csv)
{
}

void dump(CanvasApp& app, const ICellSlice::vector& cell_slices)
{
    WireCellRootVis::draw3d(app.clear(), cell_slices);
    app.pdf();
    for (auto cs : cell_slices) {
	WireCellRootVis::draw2d(app.clear(), *cs->cells());
	app.pdf();
    }
}

IWireParameters::pointer do_wireparameters()
{
    IWireParameters::pointer iwp(new WireParams);
    return iwp;
}

IWire::shared_vector do_wires(IWireParameters::pointer iwp)
{
    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWire::shared_vector wires;
    Assert(wg.extract(wires));
    Assert(wires);
    Assert(!wires->empty());
    return wires;
}

ICell::shared_vector do_cells(IWire::shared_vector wires)
{
    BoundCells bc;
    Assert(bc.insert(wires));
    ICell::shared_vector cells;
    Assert(bc.extract(cells));
    Assert(cells);
    Assert(!cells->empty());
    return cells;
}

IDepo::shared_vector do_deposition()
{
    TrackDepos td = make_tracks();
    IDepo::vector* depos = new IDepo::vector;
    while (true) {
	auto depo = td();
	if (!depo) { break; }
	depos->push_back(depo);
    }
    return IDepo::shared_vector(depos);
}

/// Run a generic action on a vector of Action::input_type producing a
/// vector of Action::output_type.  The vector of input may be
/// optionally nullptr-terminated.  The resulting vector of output
/// will not be.
template<typename Action>
std::shared_ptr<std::vector<typename Action::output_type> >
do_vector_action(Action& action, const std::vector<typename Action::input_type>& input)
{
    for (auto in : input) {
	if (!in) {break;}	// EOS was explicitly included in input
	Assert(action.insert(in));
    }
    Assert(action.insert(nullptr)); // flush with EOS
    
    typedef std::vector<typename Action::output_type> vector_type;
    vector_type* ret = new vector_type;
    while (true) {
	typename Action::output_type out;
	Assert(action.extract(out));
	if (!out) {		// eos
	    break;
	}
	ret->push_back(out);
    }
    return std::shared_ptr<vector_type>(ret);
}

IDepo::shared_vector do_drift(const IDepo::vector& activity, double to_x)
{
    Drifter drifter(to_x);
    return do_vector_action(drifter, activity);
}

IDiffusion::shared_vector do_diffusion(const IDepo::vector& depos, const Ray& pitch, double tick, double now) 
{
    Diffuser diffuser(pitch, tick, time_offset(pitch), now);
    return do_vector_action(diffuser, depos);
}

IPlaneSlice::shared_vector do_ductor(const IDiffusion::vector& diffused,
				     const IWire::shared_vector& wires,
				     const Ray& pitch,
				     WirePlaneLayer_t layer, double tick, double now)
{
    IPlaneDuctor::pointer ductor = make_ductor(pitch, layer, wires, tick, now);
    return do_vector_action(*ductor, diffused);
}

IChannelSlice::shared_vector do_digitizer(const IWire::shared_vector& wires,
					  const std::vector<IPlaneSlice::shared_vector> psvs_byplane)
{
    Digitizer digitizer;
    digitizer.set_wires(wires);

    // repackage and fully load up
    int islice = 0;
    while (true) {
	IPlaneSlice::vector* psv = new IPlaneSlice::vector(3);
	int n_filled = 0;
	for (int ind=0; ind<3; ++ind) {
	    if (islice < psvs_byplane[ind]->size()) {
		(*psv)[ind] = psvs_byplane[ind]->at(islice);
		++n_filled;
	    }
	}
	++islice;
	if (!n_filled) {
	    break;
	}
	
	Assert(digitizer.insert(IPlaneSlice::shared_vector(psv)));
    }
    Assert(digitizer.insert(nullptr));

    IChannelSlice::vector* frame = new IChannelSlice::vector;
    while (true) {
	IChannelSlice::pointer csp;
	Assert(digitizer.extract(csp));
	if (!csp) {
	    break;
	}
	frame->push_back(csp);
    }
    return IChannelSlice::shared_vector(frame);
}


ICellSlice::shared_vector do_channelcellselector(const ICell::shared_vector& cells,
						 const IChannelSlice::shared_vector& frame)
{
    ChannelCellSelector ccsel(0.0, 3);
    ccsel.set_cells(cells);

    // load up
    for (auto cs : *frame) {
	Assert(ccsel.insert(cs));
    }
    Assert(ccsel.insert(nullptr));

    ICellSlice::vector* cell_slices = new ICellSlice::vector;
    while (true) {
	ICellSlice::pointer cellslice;
	Assert(ccsel.extract(cellslice));
	if (!cellslice) {
	    break;
	}
	cell_slices->push_back(cellslice);
    }
    return ICellSlice::shared_vector(cell_slices);
}


int main(int argc, char *argv[])
{
    CanvasApp app(argv[0], argc>1, 1000,1000);
    ExecMon em(argv[0]);

    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp = do_wireparameters();
    dump(app, *iwp);
    cerr << em("made wire pa rams") << endl;


    IWire::shared_vector wires = do_wires(iwp);    
    dump(app, *wires);
    cerr << em("made wires") << endl;

    ICell::shared_vector cells = do_cells(wires);
    dump(app, *cells);
    cerr << em("made cells") << endl;

    IDepo::shared_vector activity = do_deposition();
    dump(app, *activity);
    cerr << em("made activity") << endl;

    std::vector<IDepo::shared_vector> plane_depos = {
	do_drift(*activity, iwp->pitchU().first.x()),
	do_drift(*activity, iwp->pitchV().first.x()),
	do_drift(*activity, iwp->pitchW().first.x())
    };
    for (auto depo : plane_depos) {
	dump(app, *depo);
    }
    cerr << em("drifted") << endl;

    // diffuse 
    std::vector<IDiffusion::shared_vector> diffused = {
	do_diffusion(*plane_depos[0], iwp->pitchU(), tick, now),
	do_diffusion(*plane_depos[1], iwp->pitchV(), tick, now),
	do_diffusion(*plane_depos[2], iwp->pitchW(), tick, now)
    };
    for (auto diff : diffused) {
	dump(app, *diff);
    }
    cerr << em("diffused") << endl;

    // collect/induce
    std::vector<IPlaneSlice::shared_vector> psvs_byplane = {
	do_ductor(*diffused[0], wires, iwp->pitchU(), kUlayer, tick, now),
	do_ductor(*diffused[1], wires, iwp->pitchV(), kVlayer, tick, now),
	do_ductor(*diffused[2], wires, iwp->pitchW(), kWlayer, tick, now)
    };
    for (auto psv : psvs_byplane) {
	dump(app, *psv);
    }
    cerr << em("sliced") << endl;

    IChannelSlice::shared_vector frame = do_digitizer(wires, psvs_byplane);
    dump(app, *frame);
    cerr << em("digitized") << endl;

    ICellSlice::shared_vector cell_slices = do_channelcellselector(cells, frame);
    dump(app, *cell_slices);
    cerr << em("selected cells") << endl;

    cerr << em.summary() << endl;

    app.run();
    return 0;
}
