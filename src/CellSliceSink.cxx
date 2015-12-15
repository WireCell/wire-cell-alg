#include "WireCellAlg/CellSliceSink.h"

#include <iostream>
#include <sstream>

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

	
