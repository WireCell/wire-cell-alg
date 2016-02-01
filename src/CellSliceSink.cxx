#include "WireCellAlg/CellSliceSink.h"
#include "WireCellUtil/NamedFactory.h"


#include <iostream>
#include <sstream>

WIRECELL_FACTORY(CellSliceSink, WireCell::CellSliceSink, WireCell::ICellSliceSink);

using namespace std;

using namespace WireCell;
CellSliceSink::CellSliceSink()
{
}
CellSliceSink::~CellSliceSink()
{
}

bool CellSliceSink::operator()(const input_pointer& in)
{
    if (!in) {
	return true;
    }
    m_slices.insert(in);
    auto cells = in->cells();
    return true;
}

	
