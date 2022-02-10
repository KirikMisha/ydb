#pragma once

#include "iface.h"

#include <ydb/core/tablet_flat/flat_mem_warm.h> 
#include <ydb/core/tablet_flat/flat_mem_iter.h> 
#include <ydb/core/tablet_flat/flat_row_state.h> 
#include <ydb/core/tablet_flat/flat_row_scheme.h> 
#include <ydb/core/tablet_flat/test/libs/table/test_make.h> 

namespace NKikimr {
namespace NTable {
namespace NPerf {

    class TDoMem : public IPerfTable {
    public:
        TDoMem(ILogger *logger, const TMass &mass)
            : Table(NTest::TMake(mass).Mem())
            , Nulls(Table->Scheme->Keys)
            , Remap(TRemap::Full(*Table->Scheme))
        {
            if (auto logl = logger->Log(NKikiSched::ELnLev::INF1)) {
                const auto raw = Table->GetUsedMem();
                const auto waste = Table->GetWastedMem();

                logl
                    << "mem-tree has " << Table->GetOpsCount() << " ops"
                    << " in " << NKikiSched::NFmt::TLarge(raw) << "b"
                    << ", " <<  NKikiSched::NFmt::TLarge(waste) << "b"
                    << " waste";
            }
        }

        void Seek(TRawVals key_, ESeek seek) override
        {
            const TCelled key(key_, *Table->Scheme->Keys, false);

            Iter = TMemIt::Make(*Table, Table->Immediate(), key, seek, Nulls, &Remap, nullptr);
        }

        ui64 Scan(ui64 items, TSponge &aggr) override
        {
            for (; items-- && Iter->IsValid(); Iter->Next())
                Apply(aggr);

            return items + 1;
        }

        void Once(TRawVals key, TSponge &aggr) override
        {
            Seek(key, ESeek::Exact);

            if (Iter->IsValid())
                Apply(aggr);
        }

    private:
        void Apply(TSponge &aggr) noexcept
        {
            State.Reset(Remap.Nulls());

            const auto key = Iter->GetKey();

            for (auto &pin: Remap.KeyPins())
                State.Set(pin.Pos, ECellOp::Set, key.Columns[pin.Key]); 

            Iter->Apply(State, /* committed */ {});
            aggr(State);
        }

    private:
        TIntrusivePtr<TMemTable> Table; 
        TIntrusiveConstPtr<TKeyNulls> Nulls;
        TRemap Remap;
        TAutoPtr<TMemIt> Iter;
        TRowState State;
    };

}
}
}
