#pragma once
#include "defs.h"
#include <ydb/core/base/tabletid.h> 
#include <ydb/core/base/tablet_types.h> 
#include <ydb/core/protos/tx.pb.h> 
#include <ydb/core/protos/subdomains.pb.h> 
#include <ydb/core/base/blobstorage.h> 
#include <ydb/core/base/appdata.h> 

#include <library/cpp/actors/core/event_pb.h>

namespace NKikimr {
struct TEvTxProxy {
    enum EEv {
        EvProposeTransaction = EventSpaceBegin(TKikimrEvents::ES_TX_PROXY),
        EvAcquireReadStep,

        EvProposeTransactionStatus = EvProposeTransaction + 1 * 512,
        EvAcquireReadStepResult,

        EvEnd
    };

    static_assert(EvEnd < EventSpaceEnd(TKikimrEvents::ES_TX_PROXY), "expect EvEnd < EventSpaceEnd(TKikimrEvents::ES_TX_PROXY)");

    struct TEvProposeTransaction : public TEventPB<TEvProposeTransaction, NKikimrTx::TEvProposeTransaction, EvProposeTransaction> {
        TEvProposeTransaction() = default;

        TEvProposeTransaction(ui64 coordinator, ui64 txId, ui8 execLevel, ui64 minStep, ui64 maxStep);
    };

    struct TEvProposeTransactionStatus : public TEventPB<TEvProposeTransactionStatus, NKikimrTx::TEvProposeTransactionStatus, EvProposeTransactionStatus> {
        enum class EStatus {
            StatusUnknown,
            StatusDeclined,
            StatusOutdated,
            StatusAborted,
            StatusDeclinedNoSpace,
            StatusRestarting, // coordinator is restarting (tx dropped)

            StatusAccepted = 16, // accepted by coordinator
            StatusPlanned, // planned by coordinator
            StatusProcessed, // plan entry delivered to all tablets (for synth-exec-level)
            StatusConfirmed, // execution confirmed by moderator (for synth-exec-level)
        };

        TEvProposeTransactionStatus() = default;

        TEvProposeTransactionStatus(EStatus status, ui64 txid, ui64 stepId);

        EStatus GetStatus() const {
            Y_VERIFY_DEBUG(Record.HasStatus());
            return static_cast<EStatus>(Record.GetStatus());
        }
    };

    struct TEvAcquireReadStep
        : public TEventPB<TEvAcquireReadStep, NKikimrTx::TEvAcquireReadStep, EvAcquireReadStep>
    {
        TEvAcquireReadStep() = default;

        explicit TEvAcquireReadStep(ui64 coordinator) {
            Record.SetCoordinatorID(coordinator);
        }
    };

    struct TEvAcquireReadStepResult
        : public TEventPB<TEvAcquireReadStepResult, NKikimrTx::TEvAcquireReadStepResult, EvAcquireReadStepResult>
    {
        TEvAcquireReadStepResult() = default;

        TEvAcquireReadStepResult(ui64 coordinator, ui64 step) {
            Record.SetCoordinatorID(coordinator);
            Record.SetStep(step);
        }
    };
};

// basic

struct TExecLevelHierarchy {
    struct TEntry {
        ui32 ExecLevel;
        ui64 ReversedDomainMask;
    };

    TVector<TEntry> Entries;

    ui32 Select(ui64 mask) const {
        for (ui32 i = 0, e = Entries.size(); i != e; ++i) {
            const TEntry &x = Entries[i];

            if ((x.ReversedDomainMask & mask) == 0)
                return x.ExecLevel;
        }
        return 0;
    }
};

// test hierarchy
// one availability domain #0.
// one synthetic execution level (#0) with 2 controller shards (#0, #1).
// one domain execution level (#1) with 2 controller shards (#0, #1).
// one proxy #0.
// one mediator (0-#0)
// three dummy tx-tablets in domain (##0-2)
//      or 8 data shard in domain (##0-7)
// one scheme shard (#F0)

struct TTestTxConfig {
    static constexpr ui64 DomainUid = 0;
    static constexpr ui64 Coordinator = 0x0000000000800001;
    static constexpr ui64 Mediator = 0x0000000000810001;
    static constexpr ui64 TxAllocator = 0x0000000000820001;
    static constexpr ui64 Moderator = 0x0000000000830001;
    static constexpr ui64 TxTablet0 = 0x0000000000900000;
    static constexpr ui64 TxTablet1 = 0x0000000000900001;
    static constexpr ui64 TxTablet2 = 0x0000000000900002;
    static constexpr ui64 TxTablet3 = 0x0000000000900003;
    static constexpr ui64 TxTablet4 = 0x0000000000900004;
    static constexpr ui64 TxTablet5 = 0x0000000000900005;
    static constexpr ui64 TxTablet6 = 0x0000000000900006;
    static constexpr ui64 TxTablet7 = 0x0000000000900006;
    static constexpr ui64 FakeHiveTablets = 0x000000000090000a;
    static constexpr ui64 SchemeShard = 0x00000000008587a0;
    static constexpr ui64 Hive =  0x000000000000A001;
    static constexpr ui64 UseLessId = 0xFFFFFFFFFFFFFFF;
};

struct TEvSubDomain {
    enum EEv {
        EvConfigure = EventSpaceBegin(TKikimrEvents::ES_SUB_DOMAIN),
        EvConfigureStatus,

        EvEnd
    };

    static_assert(EvEnd < EventSpaceEnd(TKikimrEvents::ES_SUB_DOMAIN), "expect EvEnd < EventSpaceEnd(TKikimrEvents::ES_SUB_DOMAIN)");

    struct TEvConfigure : public TEventPB<TEvConfigure, NKikimrSubDomains::TProcessingParams, EvConfigure> {
        TEvConfigure() = default;

        TEvConfigure(const NKikimrSubDomains::TProcessingParams &processing);
        TEvConfigure(NKikimrSubDomains::TProcessingParams &&processing);
    };

    struct TEvConfigureStatus : public TEventPB<TEvConfigureStatus, NKikimrTx::TEvSubDomainConfigurationAck, EvConfigureStatus> {
        TEvConfigureStatus() = default;

        TEvConfigureStatus(NKikimrTx::TEvSubDomainConfigurationAck::EStatus status,
                           ui64 tabletId);
    };
};

TAutoPtr<TEvSubDomain::TEvConfigure> CreateDomainConfigurationFromStatic(const TAppData *appdata, ui64 tabletId);

}

template<>
inline void Out<NKikimr::TEvTxProxy::TEvProposeTransactionStatus::EStatus>(IOutputStream& o,
        NKikimr::TEvTxProxy::TEvProposeTransactionStatus::EStatus x) {
    o << (ui32)x;
}


