#ifndef WIRECELLALG_CELLSLICESINK
#define WIRECELLALG_CELLSLICESINK

#include "WireCellIface/ICellSliceSink.h"

namespace WireCell {
    class CellSliceSink : public ICellSliceSink {
    public:
	CellSliceSink();
	virtual ~CellSliceSink();

	virtual bool insert(const input_pointer& in);

	virtual int nin() { m_nin; }

	ICellSliceSet& slices() { return m_slices; }

    private:
	
	int m_nin;
	ICellSliceSet m_slices;
    };
}

#endif
