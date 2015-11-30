#include "WireCellAlg/CellSliceSink.h"

#include <iostream>
#include <sstream>

using namespace std;

using namespace WireCell;
CellSliceSink::CellSliceSink()
    : m_nin(0)
{
}
CellSliceSink::~CellSliceSink()
{
}

bool CellSliceSink::insert(const input_pointer& in)
{
    ++m_nin;
    stringstream msg;
    msg << "CellSliceSink #" << m_nin << ": ";
    if (!in) {
	msg << "EOS\n";
	cerr << msg.str();
	return true;
    }
    
    m_slices.insert(in);

    msg << "@ " << in->time() << " with ";
    auto cells = in->cells();
    if (cells) {
	msg << in->cells()->size() << "\n";
    }
    else {
	msg << "no cells\n";
    }
    cerr << msg.str();
    return true;
}

	
