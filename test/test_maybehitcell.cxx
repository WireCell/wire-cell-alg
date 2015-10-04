#include "WireCellAlg/Predicates.h"
#include "WireCellIface/IChannelSlice.h"
#include "WireCellIface/ICell.h"
#include "WireCellIface/SimpleWire.h"
#include "WireCellIface/SimpleCell.h"

#include "WireCellUtil/Testing.h"

#include <algorithm>		// count_if
#include <iostream>

using namespace WireCell;
using namespace std;

int main() {
    ChannelCharge cc;

    // make up some bogus slice of charge
    cc[1] = 1;
    cc[2] = 2;
    cc[3] = 3;
    
    IWire::vector wires = {
	IWire::pointer(new SimpleWire(WirePlaneId(kUlayer), 10001, 0, 1, Ray())),
	IWire::pointer(new SimpleWire(WirePlaneId(kVlayer), 10002, 0, 2, Ray())),
	IWire::pointer(new SimpleWire(WirePlaneId(kWlayer), 10003, 0, 3, Ray())),
    };

    Assert(3 == wires.size());

    ICell * test = new SimpleCell(0, wires);
    IWire::vector blah = test->wires();
    cout << "Got " << blah.size() << " wires" << endl;

    Assert(test->wires().size() == 3);

    ICell::vector cells = {
	ICell::pointer(new SimpleCell(0, wires)),
    };

    Assert(3 == cells[0]->wires().size());

    MaybeHitCell three(cc, 0.0, 3);
    Assert(1 == count_if(cells.begin(), cells.end(), three));

    MaybeHitCell two(cc, 1.5, 2);
    Assert(1 == count_if(cells.begin(), cells.end(), two));

    MaybeHitCell high(cc, 100, 3);    
    Assert(0 == count_if(cells.begin(), cells.end(), high));

}
