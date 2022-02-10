#pragma once

#include "defs.h"

#include "config_index.h"
#include "configs_config.h"
#include "console.h"
#include "tx_processor.h"

#include <ydb/core/actorlib_impl/long_timer.h> 
#include <ydb/core/base/tablet_pipe.h> 
#include <ydb/core/tablet_flat/tablet_flat_executed.h> 

#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/interconnect/interconnect.h>

namespace NKikimr {
namespace NConsole {

using NTabletFlatExecutor::ITransaction;
using NTabletFlatExecutor::TTransactionBase;
using NTabletFlatExecutor::TTransactionContext;

class TConsole;

class TConfigsManager : public TActorBootstrapped<TConfigsManager> {
private:
    using TBase = TActorBootstrapped<TConfigsManager>;

public:
    struct TEvPrivate {
        enum EEv {
            EvStateLoaded = EventSpaceBegin(TKikimrEvents::ES_PRIVATE),
            EvCleanupSubscriptions,
            EvEnd
        };

        static_assert(EvEnd < EventSpaceEnd(TKikimrEvents::ES_PRIVATE), "expect EvEnd < EventSpaceEnd(TKikimrEvents::ES_PRIVATE)");

        struct TEvStateLoaded : public TEventLocal<TEvStateLoaded, EvStateLoaded> {};

        struct TEvCleanupSubscriptions : public TEventLocal<TEvCleanupSubscriptions, EvCleanupSubscriptions> {};
    };

public:
    void ClearState();
    void SetConfig(const NKikimrConsole::TConfigsConfig &config);
    // Check if specified config may be applied to configs manager.
    bool CheckConfig(const NKikimrConsole::TConfigsConfig &config,
                     Ydb::StatusIds::StatusCode &code,
                     TString &error);

    void ApplyPendingConfigModifications(const TActorContext &ctx,
                                         TAutoPtr<IEventHandle> ev = nullptr);
    void ApplyPendingSubscriptionModifications(const TActorContext &ctx,
                                               TAutoPtr<IEventHandle> ev = nullptr);

    bool MakeNewSubscriptionChecks(TSubscription::TPtr subscription,
                                   Ydb::StatusIds::StatusCode &code,
                                   TString &error);

    bool IsConfigItemScopeAllowed(TConfigItem::TPtr item);
    bool IsConfigItemScopeAllowed(TConfigItem::TPtr item,
                                  const TConfigsConfig &config);
    bool IsSupportedMergeStrategy(ui32 value) const;

    void DumpStateHTML(IOutputStream &os) const;

    // Database functions
    void DbApplyPendingConfigModifications(TTransactionContext &txc,
                                           const TActorContext &ctx) const;
    void DbApplyPendingSubscriptionModifications(TTransactionContext &txc,
                                                 const TActorContext &ctx) const;
    bool DbLoadState(TTransactionContext &txc,
                     const TActorContext &ctx);
    void DbRemoveItem(ui64 id,
                      TTransactionContext &txc,
                      const TActorContext &ctx) const;
    void DbRemoveSubscription(ui64 id,
                              TTransactionContext &txc,
                              const TActorContext &ctx) const;
    void DbUpdateItem(TConfigItem::TPtr item,
                      TTransactionContext &txc,
                      const TActorContext &ctx) const;
    void DbUpdateNextConfigItemId(TTransactionContext &txc,
                                  const TActorContext &ctx) const;
    void DbUpdateNextSubscriptionId(TTransactionContext &txc,
                                    const TActorContext &ctx) const;
    void DbUpdateSubscription(TSubscription::TPtr subscription,
                              TTransactionContext &txc,
                              const TActorContext &ctx) const;
    void DbUpdateSubscriptionLastProvidedConfig(ui64 id,
                                                const TConfigId &configId,
                                                TTransactionContext &txc,
                                                const TActorContext &ctx) const;

private:
    class TTxAddConfigSubscription;
    class TTxCleanupSubscriptions;
    class TTxConfigure;
    class TTxRemoveConfigSubscription;
    class TTxRemoveConfigSubscriptions;
    class TTxReplaceConfigSubscriptions;
    class TTxToggleConfigValidator;
    class TTxUpdateLastProvidedConfig;

    ITransaction *CreateTxAddConfigSubscription(TEvConsole::TEvAddConfigSubscriptionRequest::TPtr &ev);
    ITransaction *CreateTxCleanupSubscriptions(TEvInterconnect::TEvNodesInfo::TPtr &ev);
    ITransaction *CreateTxConfigure(TEvConsole::TEvConfigureRequest::TPtr &ev);
    ITransaction *CreateTxRemoveConfigSubscription(TEvConsole::TEvRemoveConfigSubscriptionRequest::TPtr &ev);
    ITransaction *CreateTxRemoveConfigSubscriptions(TEvConsole::TEvRemoveConfigSubscriptionsRequest::TPtr &ev);
    ITransaction *CreateTxReplaceConfigSubscriptions(TEvConsole::TEvReplaceConfigSubscriptionsRequest::TPtr &ev);
    ITransaction *CreateTxToggleConfigValidator(TEvConsole::TEvToggleConfigValidatorRequest::TPtr &ev);
    ITransaction *CreateTxUpdateLastProvidedConfig(TEvConsole::TEvConfigNotificationResponse::TPtr &ev);

    void Handle(TEvConsole::TEvAddConfigSubscriptionRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvConfigNotificationResponse::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvConfigureRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvListConfigValidatorsRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvRemoveConfigSubscriptionRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvRemoveConfigSubscriptionsRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvReplaceConfigSubscriptionsRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvConsole::TEvToggleConfigValidatorRequest::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvInterconnect::TEvNodesInfo::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvPrivate::TEvStateLoaded::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvPrivate::TEvCleanupSubscriptions::TPtr &ev, const TActorContext &ctx);

    void ForwardToConfigsProvider(TAutoPtr<IEventHandle> &ev, const TActorContext &ctx);

    void ScheduleSubscriptionsCleanup(const TActorContext &ctx);

    STFUNC(StateWork)
    {
        TRACE_EVENT(NKikimrServices::CMS_CONFIGS);
        switch (ev->GetTypeRewrite()) {
            HFuncTraced(TEvConsole::TEvAddConfigSubscriptionRequest, Handle);
            FFunc(TEvConsole::EvCheckConfigUpdatesRequest, ForwardToConfigsProvider);
            HFuncTraced(TEvConsole::TEvConfigNotificationResponse, Handle);
            HFuncTraced(TEvConsole::TEvConfigureRequest, Handle);
            FFunc(TEvConsole::EvGetConfigItemsRequest, ForwardToConfigsProvider);
            FFunc(TEvConsole::EvGetConfigSubscriptionRequest, ForwardToConfigsProvider);
            FFunc(TEvConsole::EvGetNodeConfigItemsRequest, ForwardToConfigsProvider);
            FFunc(TEvConsole::EvGetNodeConfigRequest, ForwardToConfigsProvider);
            FFunc(TEvConsole::EvListConfigSubscriptionsRequest, ForwardToConfigsProvider);
            HFuncTraced(TEvConsole::TEvListConfigValidatorsRequest, Handle);
            HFuncTraced(TEvConsole::TEvRemoveConfigSubscriptionRequest, Handle);
            HFuncTraced(TEvConsole::TEvRemoveConfigSubscriptionsRequest, Handle);
            HFuncTraced(TEvConsole::TEvReplaceConfigSubscriptionsRequest, Handle);
            HFuncTraced(TEvConsole::TEvToggleConfigValidatorRequest, Handle);
            HFuncTraced(TEvInterconnect::TEvNodesInfo, Handle);
            HFuncTraced(TEvPrivate::TEvCleanupSubscriptions, Handle);
            HFuncTraced(TEvPrivate::TEvStateLoaded, Handle);
            FFunc(TEvConsole::EvConfigSubscriptionRequest, ForwardToConfigsProvider);
            FFunc(TEvConsole::EvConfigSubscriptionCanceled, ForwardToConfigsProvider);

        default:
            Y_FAIL("TConfigsManager::StateWork unexpected event type: %" PRIx32 " event: %s",
                   ev->GetTypeRewrite(), ev->HasEvent() ? ev->GetBase()->ToString().data() : "serialized?");
        }
    }

public:
    TConfigsManager(TConsole &self)
        : Self(self)
    {
    }

    ~TConfigsManager()
    {
        ClearState();
    }

    static constexpr NKikimrServices::TActivity::EType ActorActivityType()
    {
        return NKikimrServices::TActivity::CMS_CONFIGS_MANAGER;
    }

    void Bootstrap(const TActorContext &ctx);
    void Detach();

private:
    TConsole &Self;
    TConfigsConfig Config;
    // All config items by id.
    TConfigIndex ConfigIndex;
    ui64 NextConfigItemId;
    TConfigModifications PendingConfigModifications;
    // Validators state.
    THashSet<TString> DisabledValidators;
    // Subscriptions.
    TSubscriptionIndex SubscriptionIndex;
    THashMap<TSubscriberId, THashSet<ui64>> SubscriptionsBySubscriber;
    ui64 NextSubscriptionId;
    TSubscriptionModifications PendingSubscriptionModifications;
    TSchedulerCookieHolder SubscriptionsCleanupTimerCookieHolder;

    TActorId ConfigsProvider;
    TTxProcessor::TPtr TxProcessor;
};

} // namespace NConsole
} // namespace NKikimr
