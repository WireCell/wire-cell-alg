#include "WireCellAlg/ChannelCellSelector.h"

#include "WireCellGen/BoundCells.h"
#include "WireCellGen/WireParams.h"
#include "WireCellGen/WireGenerator.h"

#include "WireCellRootVis/Converters.h"

#include "WireCellIface/WirePlaneId.h"
#include "WireCellIface/SimpleChannelSlice.h"

#include "WireCellUtil/Point.h"
#include "WireCellUtil/Testing.h"

#include "WireCellRootVis/CanvasApp.h"
#include "WireCellRootVis/Drawers.h"

#include <iostream>

using namespace WireCell;
using namespace std;

int main(int argc, char *argv[])
{
    WireCellRootVis::CanvasApp app(argv[0], argc>1, 1000,1000);
    app.divide(2,2);

    const double tick = 2.0*units::microsecond; 
    const int nticks_per_frame = 100;
    double now = 0.0*units::microsecond;

    IWireParameters::pointer iwp(new WireParams);


    WireCellRootVis::draw2d(app.pad(), iwp);

    WireGenerator wg;
    Assert(wg.insert(iwp));

    IWireVector wires;
    Assert(wg.extract(wires));
    Assert(wires.size());
    WireCellRootVis::draw2d(app.pad(), wires);

    BoundCells bc;
    bc.insert(wires);
    ICellVector cells;
    bc.extract(cells);

    cerr << "Make " << cells.size() << " cells" << endl;
    WireCellRootVis::draw2d(app.pad(), cells);

    ChannelCellSelector ccsel(0.0, 3);
    ccsel.set_cells(cells);

    
    // get this numbers by running test_depodriftcell
    double cstime0 = 62000;
    ChannelCharge cc0;
    cc0[100000] = Quantity(0.116779,0.0);
    cc0[100001] = Quantity(0.618444,0.0);
    cc0[100002] = Quantity(0.264777,0.0);
    cc0[200000] = Quantity(0.265864,0.0);
    cc0[200001] = Quantity(0.615872,0.0);
    cc0[200002] = Quantity(0.118264,0.0);
    cc0[300000] = Quantity(0.0374535,0.0);
    cc0[300001] = Quantity(0.462547,0.0);
    cc0[300002] = Quantity(0.462547,0.0);
    cc0[300003] = Quantity(0.0374535,0.0);


    IChannelSlice::pointer csp1(new SimpleChannelSlice(cstime0, cc0));
    ChannelCharge cc = csp1->charge();
    Assert(!cc.empty());
    Assert(ccsel.insert(csp1));
    ccsel.flush();

    ICellSlice::pointer cellslice;
    Assert(ccsel.extract(cellslice));
    Assert (cellslice != ccsel.eos());
    ICellVector cellsel = cellslice->cells();
    Assert (!cellsel.empty());
    cerr << "Selected " << cellsel.size() << " cells at t=" << cellslice->time() << endl;

    app.pdf();
    app.run();
    return 0;
}
