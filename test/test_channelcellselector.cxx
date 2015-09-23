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
    double cstime0 = 16000;
    ChannelCharge cc0;
    cc0[100065] = Quantity(1.5177,1.23195);
    cc0[100066] = Quantity(1.5177,1.23195);
    cc0[100067] = Quantity(9.85414,3.13913);
    cc0[100068] = Quantity(0.120784,0.34754);
    cc0[200065] = Quantity(8.58565,2.93013);
    cc0[200066] = Quantity(4.43263,2.10538);
    cc0[300049] = Quantity(1.52723,1.23581);
    cc0[300050] = Quantity(2.50183,1.58172);
    cc0[300051] = Quantity(8.61691,2.93546);
    cc0[300052] = Quantity(0.380652,0.61697);


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

    WireCellRootVis::draw2d(app.pad(), cellsel);

    app.pdf();
    app.run();
    return 0;
}
