#ifndef WIRECELLALG_CELLSLICESINK
#define WIRECELLALG_CELLSLICESINK

#include "WireCellIface/ICellSliceSink.h"

namespace WireCell {
    class CellSliceSink : public ICellSliceSink {
    public:
	CellSliceSink();
	virtual ~CellSliceSink();

	virtual bool operator()(const input_pointer& in);

	ICellSliceSet& slices() { return m_slices; }

    private:

	ICellSliceSet m_slices;
    };
}

#endif
