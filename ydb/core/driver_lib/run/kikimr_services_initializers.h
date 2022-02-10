#pragma once
#include "config.h"
#include "factories.h"
#include "service_initializer.h"

#include <library/cpp/actors/util/affinity.h>
#include <ydb/core/base/appdata.h>
#include <ydb/core/base/statestorage.h>
#include <ydb/core/tablet/tablet_setup.h>
#include <ydb/core/tablet/node_tablet_monitor.h>
#include <ydb/core/tablet_flat/shared_sausagecache.h>

#include <ydb/core/protos/config.pb.h>

#include <ydb/public/lib/base/msgbus.h>

#include <ydb/core/yq/libs/shared_resources/interface/shared_resources.h>

#include <library/cpp/actors/core/defs.h>
#include <library/cpp/actors/core/actorsystem.h>
#include <library/cpp/actors/core/log_settings.h>
#include <library/cpp/actors/core/scheduler_actor.h>
#include <library/cpp/actors/core/scheduler_basic.h>
#include <library/cpp/actors/interconnect/poller_tcp.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/vector.h>

namespace NKikimr {

namespace NKikimrServicesInitializers {

class IKikimrServicesInitializer : public IServiceInitializer {
protected:
    const NKikimrConfig::TAppConfig& Config;
    const ui32                       NodeId;
    const TKikimrScopeId             ScopeId;

public:
    IKikimrServicesInitializer(const TKikimrRunConfig& runConfig);
};

// base, nameservice, interconnect
class TBasicServicesInitializer : public IKikimrServicesInitializer {
    static IExecutorPool*
    CreateExecutorPool(const NKikimrConfig::TActorSystemConfig::TExecutor& poolConfig,
        const NKikimrConfig::TActorSystemConfig& systemConfig,
        ui32 poolId, ui32 maxActivityType);

    static ISchedulerThread* CreateScheduler(const NKikimrConfig::TActorSystemConfig::TScheduler &config);

public:
    TBasicServicesInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TImmediateControlBoardInitializer : public IKikimrServicesInitializer {
public:
    TImmediateControlBoardInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TBSNodeWardenInitializer : public IKikimrServicesInitializer {
public:
    TBSNodeWardenInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

// ss: proxies, replicas; board: replicas; scheme_board: replicas
class TStateStorageServiceInitializer : public IKikimrServicesInitializer {
public:
    TStateStorageServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TLocalServiceInitializer : public IKikimrServicesInitializer {
public:
    TLocalServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TSharedCacheInitializer : public IKikimrServicesInitializer {
public:
    TSharedCacheInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TBlobCacheInitializer : public IKikimrServicesInitializer {
public:
    TBlobCacheInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TLoggerInitializer : public IKikimrServicesInitializer {
    TIntrusivePtr<NActors::NLog::TSettings> LogSettings;
    std::shared_ptr<TLogBackend> LogBackend;
    TString PathToConfigCacheFile;

public:
    TLoggerInitializer(const TKikimrRunConfig& runConfig,
                       TIntrusivePtr<NActors::NLog::TSettings> logSettings,
                       std::shared_ptr<TLogBackend> logBackend);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TSchedulerActorInitializer : public IKikimrServicesInitializer {
public:
    TSchedulerActorInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TProfilerInitializer : public IKikimrServicesInitializer {
public:
    TProfilerInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TResourceBrokerInitializer : public IKikimrServicesInitializer {
public:
    TResourceBrokerInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTabletResolverInitializer : public IKikimrServicesInitializer {
public:
    TTabletResolverInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTabletPipePeNodeCachesInitializer : public IKikimrServicesInitializer {
public:
    TTabletPipePeNodeCachesInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTabletMonitoringProxyInitializer : public IKikimrServicesInitializer {
public:
    TTabletMonitoringProxyInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTabletCountersAggregatorInitializer : public IKikimrServicesInitializer {
public:
    TTabletCountersAggregatorInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TGRpcProxyStatusInitializer : public IKikimrServicesInitializer {
public:
    TGRpcProxyStatusInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TRestartsCountPublisher : public IKikimrServicesInitializer {
    static void PublishRestartsCount(const NMonitoring::TDynamicCounters::TCounterPtr& counter,
                                     const TString& restartsCountFile);

public:
    TRestartsCountPublisher(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TBootstrapperInitializer : public IKikimrServicesInitializer {
public:
    TBootstrapperInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

// alternative (RTMR-style) bootstrapper
class TTabletsInitializer : public IKikimrServicesInitializer {
    TIntrusivePtr<ITabletFactory> CustomTablets;

public:
    TTabletsInitializer(const TKikimrRunConfig& runConfig,
                        TIntrusivePtr<ITabletFactory> customTablets);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TMediatorTimeCastProxyInitializer : public IKikimrServicesInitializer {
public:
    TMediatorTimeCastProxyInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTxProxyInitializer : public IKikimrServicesInitializer {
    TVector<ui64> CollectAllAllocatorsFromAllDomains(const NKikimr::TAppData* appData);

public:
    TTxProxyInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TLongTxServiceInitializer : public IKikimrServicesInitializer {
public:
    TLongTxServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TSequenceProxyServiceInitializer : public IKikimrServicesInitializer {
public:
    TSequenceProxyServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TMiniKQLCompileServiceInitializer : public IKikimrServicesInitializer {
public:
    TMiniKQLCompileServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

// msgbus: proxy, trace
class TMessageBusServicesInitializer : public IKikimrServicesInitializer {
    NMsgBusProxy::IMessageBusServer& BusServer;

public:
    TMessageBusServicesInitializer(const TKikimrRunConfig& runConfig,
                                   NMsgBusProxy::IMessageBusServer& busServer);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

// ticket_parser and so on...
class TSecurityServicesInitializer : public IKikimrServicesInitializer {
public:
    std::shared_ptr<TModuleFactories> Factories;
    TSecurityServicesInitializer(const TKikimrRunConfig& runConfig, std::shared_ptr<TModuleFactories> factories);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

// grpc_proxy
class TGRpcServicesInitializer : public IKikimrServicesInitializer {
private:
    std::shared_ptr<TModuleFactories> Factories;

public:
    TGRpcServicesInitializer(const TKikimrRunConfig& runConfig, std::shared_ptr<TModuleFactories> factories);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

#ifdef ACTORSLIB_COLLECT_EXEC_STATS
// stats_collector, procstats_collector
class TStatsCollectorInitializer : public IKikimrServicesInitializer {
public:
    TStatsCollectorInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};
#endif

class TSelfPingInitializer : public IKikimrServicesInitializer {
public:
    TSelfPingInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TWhiteBoardServiceInitializer : public IKikimrServicesInitializer {
public:
    TWhiteBoardServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TNodeIdentifierInitializer : public IKikimrServicesInitializer {
public:
    TNodeIdentifierInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TTabletMonitorInitializer : public IKikimrServicesInitializer {
    TIntrusivePtr<NNodeTabletMonitor::ITabletStateClassifier> TabletStateClassifier;
    TIntrusivePtr<NNodeTabletMonitor::ITabletListRenderer> TabletListRenderer;

public:
    TTabletMonitorInitializer(const TKikimrRunConfig& runConfig,
                              const TIntrusivePtr<NNodeTabletMonitor::ITabletStateClassifier>& tabletStateClassifier,
                              const TIntrusivePtr<NNodeTabletMonitor::ITabletListRenderer>& tabletListRenderer);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TViewerInitializer : public IKikimrServicesInitializer {
    const TKikimrRunConfig& KikimrRunConfig;

public:
    TViewerInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TLoadInitializer : public IKikimrServicesInitializer {
public:
    TLoadInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TFailureInjectionInitializer : public IKikimrServicesInitializer {
public:
    TFailureInjectionInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup *setup, const NKikimr::TAppData *appData) override;
};

class TPersQueueL2CacheInitializer : public IKikimrServicesInitializer {
public:
    TPersQueueL2CacheInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TNetClassifierInitializer : public IKikimrServicesInitializer { 
public: 
    TNetClassifierInitializer(const TKikimrRunConfig& runConfig); 

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
}; 
 
class TPersQueueClusterTrackerInitializer : public IKikimrServicesInitializer { 
public: 
    TPersQueueClusterTrackerInitializer(const TKikimrRunConfig& runConfig); 

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
}; 
 
class TPersQueueLibSharedInstanceInitializer : public IKikimrServicesInitializer {
public:
    TPersQueueLibSharedInstanceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TMemProfMonitorInitializer : public IKikimrServicesInitializer {
public:
    TMemProfMonitorInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TMemoryTrackerInitializer : public IKikimrServicesInitializer {
public:
    TMemoryTrackerInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TQuoterServiceInitializer : public IKikimrServicesInitializer {
public:
    TQuoterServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TKqpServiceInitializer : public IKikimrServicesInitializer {
public:
    TKqpServiceInitializer(const TKikimrRunConfig& runConfig, std::shared_ptr<TModuleFactories> factories);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
private:
    std::shared_ptr<TModuleFactories> Factories;
};

class TMemoryLogInitializer : public IKikimrServicesInitializer {
    size_t LogBufferSize = 0;
    size_t LogGrainSize = 0;

public:
    TMemoryLogInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TCmsServiceInitializer : public IKikimrServicesInitializer {
public:
    TCmsServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TLeaseHolderInitializer : public IKikimrServicesInitializer {
public:
    TLeaseHolderInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TSqsServiceInitializer : public IKikimrServicesInitializer {
public:
    TSqsServiceInitializer(const TKikimrRunConfig& runConfig, const std::shared_ptr<TModuleFactories>& factories);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;

private:
    std::shared_ptr<TModuleFactories> Factories;
};

class TConfigsDispatcherInitializer : public IKikimrServicesInitializer {
public:
    TConfigsDispatcherInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TConfigsCacheInitializer : public IKikimrServicesInitializer {
private:
    TString PathToConfigCacheFile;
public:
    TConfigsCacheInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TTabletInfoInitializer : public IKikimrServicesInitializer {
public:
    TTabletInfoInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TConfigValidatorsInitializer : public IKikimrServicesInitializer {
public:
    TConfigValidatorsInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TSysViewServiceInitializer : public IKikimrServicesInitializer {
public:
    TSysViewServiceInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TMeteringWriterInitializer : public IKikimrServicesInitializer {
public:
    TMeteringWriterInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TSchemeBoardMonitoringInitializer : public IKikimrServicesInitializer {
public:
    TSchemeBoardMonitoringInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TYqlLogsInitializer : public IKikimrServicesInitializer {
public:
    TYqlLogsInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class THealthCheckInitializer : public IKikimrServicesInitializer {
public:
    THealthCheckInitializer(const TKikimrRunConfig& runConfig);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;
};

class TYandexQueryInitializer : public IKikimrServicesInitializer {
public:
    TYandexQueryInitializer(const TKikimrRunConfig& runConfig, std::shared_ptr<TModuleFactories> factories, NYq::IYqSharedResources::TPtr yqSharedResources);

    void InitializeServices(NActors::TActorSystemSetup* setup, const NKikimr::TAppData* appData) override;

    static void SetIcPort(ui32 icPort);
private:
    std::shared_ptr<TModuleFactories> Factories;
    NYq::IYqSharedResources::TPtr YqSharedResources;
    static ui32 IcPort;
};

} // namespace NKikimrServicesInitializers
} // namespace NKikimr
