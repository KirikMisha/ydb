#include "test_client.h"

#include <ydb/core/testlib/basics/runtime.h> 
#include <ydb/core/base/appdata.h> 
#include <ydb/core/base/hive.h> 
#include <ydb/public/lib/base/msgbus.h> 
#include <ydb/core/grpc_services/grpc_request_proxy.h> 
#include <ydb/services/auth/grpc_service.h>
#include <ydb/services/yq/grpc_service.h> 
#include <ydb/services/yq/private_grpc.h> 
#include <ydb/services/cms/grpc_service.h> 
#include <ydb/services/datastreams/grpc_service.h> 
#include <ydb/services/kesus/grpc_service.h> 
#include <ydb/core/grpc_services/grpc_mon.h> 
#include <ydb/services/ydb/ydb_clickhouse_internal.h> 
#include <ydb/services/ydb/ydb_dummy.h> 
#include <ydb/services/ydb/ydb_experimental.h> 
#include <ydb/services/ydb/ydb_export.h> 
#include <ydb/services/ydb/ydb_import.h> 
#include <ydb/services/ydb/ydb_operation.h> 
#include <ydb/services/ydb/ydb_s3_internal.h> 
#include <ydb/services/ydb/ydb_scheme.h> 
#include <ydb/services/ydb/ydb_scripting.h> 
#include <ydb/services/ydb/ydb_table.h> 
#include <ydb/services/ydb/ydb_long_tx.h> 
#include <ydb/services/ydb/ydb_logstore.h> 
#include <ydb/services/discovery/grpc_service.h> 
#include <ydb/services/rate_limiter/grpc_service.h> 
#include <ydb/services/persqueue_cluster_discovery/grpc_service.h> 
#include <ydb/services/persqueue_v1/persqueue.h> 
#include <ydb/services/persqueue_v1/grpc_pq_write.h> 
#include <ydb/services/yq/grpc_service.h> 
#include <ydb/core/yq/libs/control_plane_proxy/control_plane_proxy.h> 
#include <ydb/core/yq/libs/control_plane_storage/control_plane_storage.h> 
#include <ydb/core/client/metadata/types_metadata.h> 
#include <ydb/core/client/metadata/functions_metadata.h> 
#include <ydb/core/client/minikql_compile/mkql_compile_service.h> 
#include <ydb/core/cms/console/configs_dispatcher.h> 
#include <ydb/core/cms/console/console.h> 
#include <ydb/core/cms/console/immediate_controls_configurator.h> 
#include <ydb/core/formats/clickhouse_block.h> 
#include <ydb/core/security/ticket_parser.h> 
#include <ydb/core/base/user_registry.h> 
#include <ydb/core/health_check/health_check.h> 
#include <ydb/core/kqp/kqp.h> 
#include <ydb/core/kqp/rm/kqp_rm.h> 
#include <ydb/core/metering/metering.h> 
#include <ydb/core/protos/services.pb.h> 
#include <ydb/core/tablet_flat/tablet_flat_executed.h> 
#include <ydb/core/tx/columnshard/columnshard.h> 
#include <ydb/core/tx/coordinator/coordinator.h> 
#include <ydb/core/tx/datashard/datashard.h> 
#include <ydb/core/tx/long_tx_service/public/events.h> 
#include <ydb/core/tx/long_tx_service/long_tx_service.h> 
#include <ydb/core/tx/mediator/mediator.h> 
#include <ydb/core/tx/replication/controller/controller.h>
#include <ydb/core/tx/schemeshard/schemeshard.h> 
#include <ydb/core/tx/sequenceproxy/sequenceproxy.h> 
#include <ydb/core/tx/sequenceshard/sequenceshard.h> 
#include <ydb/core/tx/tx_allocator/txallocator.h> 
#include <ydb/core/tx/tx_proxy/proxy.h> 
#include <ydb/core/tx/time_cast/time_cast.h> 
#include <ydb/core/mind/address_classification/net_classifier.h> 
#include <ydb/core/mind/bscontroller/bsc.h> 
#include <ydb/core/mind/hive/hive.h> 
#include <ydb/core/mind/labels_maintainer.h> 
#include <ydb/core/mind/tenant_pool.h> 
#include <ydb/core/mind/tenant_slot_broker.h> 
#include <ydb/core/mind/tenant_node_enumeration.h> 
#include <ydb/core/kesus/tablet/events.h> 
#include <ydb/core/sys_view/service/sysview_service.h> 
#include <ydb/library/yql/minikql/mkql_function_registry.h>
#include <ydb/library/yql/minikql/invoke_builtins/mkql_builtins.h>
#include <ydb/library/yql/public/issue/yql_issue_message.h>
#include <ydb/core/engine/mkql_engine_flat.h> 

#include <library/cpp/testing/unittest/registar.h>
#include <ydb/core/kesus/proxy/proxy.h> 
#include <ydb/core/kesus/tablet/tablet.h> 
#include <ydb/core/sys_view/processor/processor.h> 
#include <ydb/core/keyvalue/keyvalue.h> 
#include <ydb/core/persqueue/pq.h> 
#include <ydb/core/persqueue/cluster_tracker.h> 
#include <ydb/core/yq/libs/audit/mock/yq_mock_audit_service.h> 
#include <ydb/library/security/ydb_credentials_provider_factory.h> 
#include <ydb/core/yq/libs/init/init.h> 
#include <ydb/core/yq/libs/mock/yql_mock.h> 
#include <ydb/library/folder_service/mock/mock_folder_service.h> 

#include <ydb/core/client/server/msgbus_server_tracer.h> 

#include <library/cpp/actors/interconnect/interconnect.h>

#include <library/cpp/grpc/server/actors/logger.h>

#include <util/system/sanitizers.h>
#include <util/system/valgrind.h>
#include <util/system/env.h>

namespace NKikimr {

namespace Tests {



    TServerSettings& TServerSettings::SetDomainName(const TString& value) {
        StoragePoolTypes.erase("test");
        DomainName = value;
        AddStoragePool("test", "/" + DomainName + ":test");
        return *this;
    }

    const char* ServerRedirectEnvVar = "KIKIMR_SERVER";
    const char* DomainRedirectEnvVar = "KIKIMR_TEST_DOMAIN";
    const TDuration TIMEOUT = NSan::PlainOrUnderSanitizer(
        NValgrind::PlainOrUnderValgrind(TDuration::Seconds(3), TDuration::Seconds(60)),
        TDuration::Seconds(15)
    );
    const ui64 ConnectTimeoutMilliSeconds = NSan::PlainOrUnderSanitizer(
        NValgrind::PlainOrUnderValgrind(TDuration::Seconds(60), TDuration::Seconds(120)),
        TDuration::Seconds(120)
    ).MilliSeconds();

    NMiniKQL::IFunctionRegistry* DefaultFrFactory(const NScheme::TTypeRegistry& typeRegistry) {
        Y_UNUSED(typeRegistry);
        // register test UDFs
        auto freg = NKikimr::NMiniKQL::CreateFunctionRegistry(NKikimr::NMiniKQL::CreateBuiltinRegistry())->Clone();
        NKikimr::NMiniKQL::FillStaticModules(*freg);
        return freg.Release();
    }

    ui64 ChangeDomain(ui64 tabletId, ui32 domainUid) {
        return MakeTabletID(StateStorageGroupFromTabletID(tabletId), domainUid, UniqPartFromTabletID(tabletId));
    }

    ui64 ChangeStateStorage(ui64 tabletId, ui32 ssUid) {
        return MakeTabletID(ssUid, HiveUidFromTabletID(tabletId), UniqPartFromTabletID(tabletId));
    }

    TServer::TServer(TServerSettings::TConstPtr settings, bool init)
        : Settings(settings)
        , UseStoragePools(!Settings->StoragePoolTypes.empty())
    {
        if (Settings->SupportsRedirect && IsServerRedirected())
            return;

        if (init)
            Initialize();
    }

    TServer::TServer(const TServerSettings &settings, bool init)
        : TServer(new TServerSettings(settings), init)
    {
    }

    void TServer::Initialize() {
        if (Settings->SupportsRedirect && IsServerRedirected())
            return;

        TAppPrepare app; /* will cook TAppData */
        app.SetNetDataSourceUrl(Settings->NetClassifierConfig.GetUpdaterConfig().GetNetDataSourceUrl());
        app.SetEnableKqpSpilling(Settings->EnableKqpSpilling);
        app.SetKeepSnapshotTimeout(Settings->KeepSnapshotTimeout);
        app.SetChangesQueueItemsLimit(Settings->ChangesQueueItemsLimit);
        app.SetChangesQueueBytesLimit(Settings->ChangesQueueBytesLimit);
        app.CompactionConfig = Settings->CompactionConfig;
        app.FeatureFlags = Settings->FeatureFlags;

        Runtime = MakeHolder<TTestBasicRuntime>(StaticNodes() + DynamicNodes(), Settings->UseRealThreads);

        if (!Settings->UseRealThreads)
            Runtime->SetRegistrationObserverFunc([](TTestActorRuntimeBase& runtime, const TActorId&, const TActorId& actorId) {
                    runtime.EnableScheduleForActor(actorId);
                });

        for (auto& it: Settings->NodeKeys)     {
            ui32 nodeId = it.first;
            const TString& keyValue = it.second;

            TString baseDir = Runtime->GetTempDir();
            TString keyfile = TStringBuilder() << baseDir << "/key-" << nodeId << ".txt";
            {
                TFileOutput file(keyfile);
                file << keyValue;
            }
            app.SetKeyForNode(keyfile, nodeId);
        }

        SetupLogging();

        SetupMessageBus(Settings->Port, Settings->TracePath);
        SetupDomains(app);

        app.AddHive(Settings->Domain, ChangeStateStorage(Hive, Settings->Domain));
        app.SetFnRegistry(Settings->FrFactory);
        app.SetFormatsFactory(Settings->Formats);

        if (Settings->Formats) {
            NKikHouse::RegisterFormat(*Settings->Formats);
        }

        NKikimr::SetupChannelProfiles(app, Settings->Domain);

        Runtime->SetupMonitoring();
        Runtime->SetLogBackend(Settings->LogBackend);

        SetupTabletServices(*Runtime, &app, (StaticNodes() + DynamicNodes()) == 1 && Settings->EnableMockOnSingleNode, Settings->CustomDiskParams);

        CreateBootstrapTablets();
        SetupStorage();

        for (ui32 nodeIdx = 0; nodeIdx < StaticNodes() + DynamicNodes(); ++nodeIdx) {
            SetupDomainLocalService(nodeIdx);
            Runtime->GetAppData(nodeIdx).AuthConfig.MergeFrom(Settings->AuthConfig);
            Runtime->GetAppData(nodeIdx).PQConfig.MergeFrom(Settings->PQConfig);
            Runtime->GetAppData(nodeIdx).PQClusterDiscoveryConfig.MergeFrom(Settings->PQClusterDiscoveryConfig);
            Runtime->GetAppData(nodeIdx).NetClassifierConfig.MergeFrom(Settings->NetClassifierConfig);
            Runtime->GetAppData(nodeIdx).StreamingConfig.MergeFrom(Settings->AppConfig.GetGRpcConfig().GetStreamingConfig());
            Runtime->GetAppData(nodeIdx).EnforceUserTokenRequirement = Settings->AppConfig.GetDomainsConfig().GetSecurityConfig().GetEnforceUserTokenRequirement();
            SetupConfigurators(nodeIdx);
            SetupProxies(nodeIdx);
        }
    }

    void TServer::SetupMessageBus(ui16 port, const TString &tracePath) {
        if (port) {
            Bus = NBus::CreateMessageQueue(NBus::TBusQueueConfig());
            if (tracePath) {
                BusServer.Reset(NMsgBusProxy::CreateMsgBusTracingServer(
                    Bus.Get(),
                    BusServerSessionConfig,
                    tracePath,
                    Settings->PersQueueGetReadSessionsInfoWorkerFactory,
                    port
                ));
            } else {
                BusServer.Reset(NMsgBusProxy::CreateMsgBusServer(
                    Bus.Get(),
                    BusServerSessionConfig,
                    Settings->PersQueueGetReadSessionsInfoWorkerFactory,
                    port
                ));
            }
        }
    }

    void TServer::EnableGRpc(const NGrpc::TServerOptions& options) {
        GRpcServer.reset(new NGrpc::TGRpcServer(options));
        auto grpcService = new NGRpcProxy::TGRpcService();

        auto system(Runtime->GetAnyNodeActorSystem());
        auto grpcRequestProxy = NGRpcService::CreateGRpcRequestProxy(Settings->AppConfig);
        auto grpcRequestProxyId = system->Register(grpcRequestProxy, TMailboxType::ReadAsFilled);
        system->RegisterLocalService(NGRpcService::CreateGRpcRequestProxyId(), grpcRequestProxyId);
        auto grpcMon = system->Register(NGRpcService::CreateGrpcMonService(), TMailboxType::ReadAsFilled);
        system->RegisterLocalService(NGRpcService::GrpcMonServiceId(), grpcMon);

        GRpcServerRootCounters = MakeIntrusive<NMonitoring::TDynamicCounters>();
        auto& counters = GRpcServerRootCounters;

        auto& appData = Runtime->GetAppData();

        // Setup discovery for typically used services on the node
        {
            TIntrusivePtr<NGRpcService::TGrpcEndpointDescription> desc = new NGRpcService::TGrpcEndpointDescription();
            desc->Address = options.Host;
            desc->Port = options.Port;
            desc->Ssl = !options.SslData.Empty();

            TVector<TString> rootDomains;
            for (auto &domain : appData.DomainsInfo->Domains) {
                rootDomains.emplace_back("/" + domain.second->Name);
            }
            desc->ServedDatabases.insert(desc->ServedDatabases.end(), rootDomains.begin(), rootDomains.end());

            TVector<TString> grpcServices = {"yql", "clickhouse_internal", "datastreams", "table_service", "scripting", "experimental", "discovery", "pqcd", "pq", "pqv1" };
            desc->ServedServices.insert(desc->ServedServices.end(), grpcServices.begin(), grpcServices.end());

            system->Register(NGRpcService::CreateGrpcEndpointPublishActor(desc.Get()), TMailboxType::ReadAsFilled, appData.UserPoolId);
        }

        auto future = grpcService->Prepare(
            system,
            NMsgBusProxy::CreatePersQueueMetaCacheV2Id(),
            NMsgBusProxy::CreateMsgBusProxyId(),
            counters
        );
        auto startCb = [grpcService] (NThreading::TFuture<void> result) {
            if (result.HasException()) {
                try {
                    result.GetValue();
                } catch (const std::exception& ex) {
                    Y_FAIL("Unable to prepare GRpc service: %s", ex.what());
                }
            } else {
                grpcService->Start();
            }
        };

        future.Subscribe(startCb);

        GRpcServer->AddService(grpcService);
        GRpcServer->AddService(new NGRpcService::TGRpcYdbExportService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbImportService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbSchemeService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbTableService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbScriptingService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcOperationService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::V1::TGRpcPersQueueService(system, counters, NMsgBusProxy::CreatePersQueueMetaCacheV2Id(), grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcPQClusterDiscoveryService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NKesus::TKesusGRpcService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcCmsService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcDiscoveryService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbExperimentalService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbClickhouseInternalService(system, counters, appData.InFlightLimiterRegistry, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbS3InternalService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NQuoter::TRateLimiterGRpcService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcYdbLongTxService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcDataStreamsService(system, counters, grpcRequestProxyId));
        if (Settings->EnableYq) {
            GRpcServer->AddService(new NGRpcService::TGRpcYandexQueryService(system, counters, grpcRequestProxyId));
            GRpcServer->AddService(new NGRpcService::TGRpcYqPrivateTaskService(system, counters, grpcRequestProxyId));
        }
        if (const auto& factory = Settings->GrpcServiceFactory) {
            // All services enabled by default for ut
            static const std::unordered_set<TString> dummy;
            for (const auto& service : factory->Create(dummy, dummy, system, counters, grpcRequestProxyId)) {
                GRpcServer->AddService(service);
            }
        }
        GRpcServer->AddService(new NGRpcService::TGRpcYdbLogStoreService(system, counters, grpcRequestProxyId));
        GRpcServer->AddService(new NGRpcService::TGRpcAuthService(system, counters, grpcRequestProxyId));
        GRpcServer->Start();
    }

    void TServer::EnableGRpc(ui16 port) {
        EnableGRpc(NGrpc::TServerOptions()
            .SetHost("localhost")
            .SetPort(port)
            .SetLogger(NGrpc::CreateActorSystemLogger(*Runtime->GetAnyNodeActorSystem(), NKikimrServices::GRPC_SERVER))
        );
    }

    void TServer::SetupDomains(TAppPrepare &app) {
        const ui32 domainId = Settings->Domain;
        ui64 planResolution = Settings->DomainPlanResolution;
        if (!planResolution) {
            planResolution = Settings->UseRealThreads ? 7 : 500;
        }
        auto domain = TDomainsInfo::TDomain::ConstructDomainWithExplicitTabletIds(Settings->DomainName, domainId, ChangeStateStorage(SchemeRoot, domainId),
                                                                                  domainId, domainId, TVector<ui32>{domainId},
                                                                                  domainId, TVector<ui32>{domainId},
                                                                                  planResolution,
                                                                                  TVector<ui64>{TDomainsInfo::MakeTxCoordinatorIDFixed(domainId, 1)},
                                                                                  TVector<ui64>{TDomainsInfo::MakeTxMediatorIDFixed(domainId, 1)},
                                                                                  TVector<ui64>{TDomainsInfo::MakeTxAllocatorIDFixed(domainId, 1)},
                                                                                  Settings->StoragePoolTypes);
        app.AddDomain(domain.Release());
    }

    void TServer::CreateBootstrapTablets() {
        const ui32 domainId = Settings->Domain;
        Y_VERIFY(TDomainsInfo::MakeTxAllocatorIDFixed(domainId, 1) == ChangeStateStorage(TxAllocator, domainId));
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(TxAllocator, domainId), TTabletTypes::TX_ALLOCATOR), &CreateTxAllocator);
        Y_VERIFY(TDomainsInfo::MakeTxCoordinatorIDFixed(domainId, 1) == ChangeStateStorage(Coordinator, domainId));
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(Coordinator, domainId), TTabletTypes::FLAT_TX_COORDINATOR), &CreateFlatTxCoordinator);
        Y_VERIFY(TDomainsInfo::MakeTxMediatorIDFixed(domainId, 1) == ChangeStateStorage(Mediator, domainId));
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(Mediator, domainId), TTabletTypes::TX_MEDIATOR), &CreateTxMediator);
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(SchemeRoot, domainId), TTabletTypes::FLAT_SCHEMESHARD), &CreateFlatTxSchemeShard);
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(Hive, domainId), TTabletTypes::FLAT_HIVE), &CreateDefaultHive);
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(MakeBSControllerID(domainId), TTabletTypes::FLAT_BS_CONTROLLER), &CreateFlatBsController);
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(MakeTenantSlotBrokerID(domainId), TTabletTypes::TENANT_SLOT_BROKER), &NTenantSlotBroker::CreateTenantSlotBroker);
        if (Settings->EnableConsole)
            CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(MakeConsoleID(domainId), TTabletTypes::CONSOLE), &NConsole::CreateConsole);
    }

    void TServer::SetupStorage() {
        TActorId sender = Runtime->AllocateEdgeActor();

        NTabletPipe::TClientConfig pipeConfig;
        pipeConfig.RetryPolicy = NTabletPipe::TClientRetryPolicy::WithRetries();

        //get NodesInfo, nodes hostname and port are interested
        Runtime->Send(new IEventHandle(GetNameserviceActorId(), sender, new TEvInterconnect::TEvListNodes));
        TAutoPtr<IEventHandle> handleNodesInfo;
        auto nodesInfo = Runtime->GrabEdgeEventRethrow<TEvInterconnect::TEvNodesInfo>(handleNodesInfo);

        auto bsConfigureRequest = MakeHolder<TEvBlobStorage::TEvControllerConfigRequest>();

        NKikimrBlobStorage::TDefineBox boxConfig;
        boxConfig.SetBoxId(Settings->BOX_ID);

        ui32 nodeId = Runtime->GetNodeId(0);
        Y_VERIFY(nodesInfo->Nodes[0].NodeId == nodeId);
        auto& nodeInfo = nodesInfo->Nodes[0];

        NKikimrBlobStorage::TDefineHostConfig hostConfig;
        hostConfig.SetHostConfigId(nodeId);
        TString path = TStringBuilder() << Runtime->GetTempDir() << "pdisk_1.dat";
        hostConfig.AddDrive()->SetPath(path);
        Cerr << "test_client.cpp: SetPath # " << path << Endl;
        bsConfigureRequest->Record.MutableRequest()->AddCommand()->MutableDefineHostConfig()->CopyFrom(hostConfig);

        auto& host = *boxConfig.AddHost();
        host.MutableKey()->SetFqdn(nodeInfo.Host);
        host.MutableKey()->SetIcPort(nodeInfo.Port);
        host.SetHostConfigId(hostConfig.GetHostConfigId());
        bsConfigureRequest->Record.MutableRequest()->AddCommand()->MutableDefineBox()->CopyFrom(boxConfig);

        for (const auto& [poolKind, storagePool] : Settings->StoragePoolTypes) {
            if (storagePool.GetNumGroups() > 0) {
                bsConfigureRequest->Record.MutableRequest()->AddCommand()->MutableDefineStoragePool()->CopyFrom(storagePool);
            }
        }

        Runtime->SendToPipe(MakeBSControllerID(Settings->Domain), sender, bsConfigureRequest.Release(), 0, pipeConfig);

        TAutoPtr<IEventHandle> handleConfigureResponse;
        auto configureResponse = Runtime->GrabEdgeEventRethrow<TEvBlobStorage::TEvControllerConfigResponse>(handleConfigureResponse);
        if (!configureResponse->Record.GetResponse().GetSuccess()) {
            Cerr << "\n\n configResponse is #" << configureResponse->Record.DebugString() << "\n\n";
        }
        UNIT_ASSERT(configureResponse->Record.GetResponse().GetSuccess());
    }

    void TServer::SetupDefaultProfiles() {
        NKikimr::Tests::TClient client(*Settings);
        TAutoPtr<NMsgBusProxy::TBusConsoleRequest> request(new NMsgBusProxy::TBusConsoleRequest());
        auto &item = *request->Record.MutableConfigureRequest()->AddActions()->MutableAddConfigItem()->MutableConfigItem();
        item.SetKind((ui32)NKikimrConsole::TConfigItem::TableProfilesConfigItem);
        auto &profiles = *item.MutableConfig()->MutableTableProfilesConfig();
        {
            // Storage policy:
            auto& policy = *profiles.AddStoragePolicies();
            policy.SetName("default");
            auto& family = *policy.AddColumnFamilies();
            family.SetId(0);
            family.MutableStorageConfig()->MutableSysLog()->SetPreferredPoolKind("test");
            family.MutableStorageConfig()->MutableLog()->SetPreferredPoolKind("test");
            family.MutableStorageConfig()->MutableData()->SetPreferredPoolKind("test");
        }
        {
            // Compaction policy:
            NLocalDb::TCompactionPolicyPtr defaultPolicy = NLocalDb::CreateDefaultUserTablePolicy();
            NKikimrSchemeOp::TCompactionPolicy defaultflatSchemePolicy;
            defaultPolicy->Serialize(defaultflatSchemePolicy);
            auto &defaultCompactionPolicy = *profiles.AddCompactionPolicies();
            defaultCompactionPolicy.SetName("default");
            defaultCompactionPolicy.MutableCompactionPolicy()->CopyFrom(defaultflatSchemePolicy);

            NLocalDb::TCompactionPolicy policy1;
            policy1.Generations.push_back({ 0, 8, 8, 128 * 1024 * 1024, NLocalDb::LegacyQueueIdToTaskName(1), true });
            NKikimrSchemeOp::TCompactionPolicy flatSchemePolicy1;
            policy1.Serialize(flatSchemePolicy1);
            auto &compactionPolicy1 = *profiles.AddCompactionPolicies();
            compactionPolicy1.SetName("compaction1");
            compactionPolicy1.MutableCompactionPolicy()->CopyFrom(flatSchemePolicy1);

            NLocalDb::TCompactionPolicy policy2;
            policy2.Generations.push_back({ 0, 8, 8, 128 * 1024 * 1024, NLocalDb::LegacyQueueIdToTaskName(1), true });
            policy2.Generations.push_back({ 40 * 1024 * 1024, 5, 16, 512 * 1024 * 1024, NLocalDb::LegacyQueueIdToTaskName(2), false });
            NKikimrSchemeOp::TCompactionPolicy flatSchemePolicy2;
            policy2.Serialize(flatSchemePolicy2);
            auto &compactionPolicy2 = *profiles.AddCompactionPolicies();
            compactionPolicy2.SetName("compaction2");
            compactionPolicy2.MutableCompactionPolicy()->CopyFrom(flatSchemePolicy2);
        }
        {
            auto& profile = *profiles.AddTableProfiles();
            profile.SetName("default");
            profile.SetStoragePolicy("default");
        }
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = client.SyncCall(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        auto resp = dynamic_cast<NMsgBusProxy::TBusConsoleResponse*>(reply.Get())->Record;
        UNIT_ASSERT_VALUES_EQUAL(resp.GetStatus().GetCode(), Ydb::StatusIds::SUCCESS);
    }

    void TServer::SetupDomainLocalService(ui32 nodeIdx) {
        SetupLocalService(nodeIdx, Settings->DomainName);
    }

    void TServer::SetupDynamicLocalService(ui32 nodeIdx, const TString &tenantName) {
        Y_VERIFY(nodeIdx >= StaticNodes());
        SetupLocalService(nodeIdx, tenantName);
    }

    void TServer::DestroyDynamicLocalService(ui32 nodeIdx) {
        Y_VERIFY(nodeIdx >= StaticNodes());
        TActorId local = MakeLocalID(Runtime->GetNodeId(nodeIdx)); // MakeTenantPoolRootID?
        Runtime->Send(new IEventHandle(local, TActorId(), new TEvents::TEvPoisonPill()));
    }

    void TServer::SetupLocalConfig(TLocalConfig &localConfig, const NKikimr::TAppData &appData) {
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.Dummy] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateFlatDummyTablet, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.DataShard] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateDataShard, TMailboxType::Revolving, appData.UserPoolId, 
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.KeyValue] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateKeyValueFlat, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.ColumnShard] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateColumnShard, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.PersQueue] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreatePersQueue, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.PersQueueReadBalancer] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreatePersQueueReadBalancer, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.Coordinator] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateFlatTxCoordinator, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.Mediator] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateTxMediator, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.Kesus] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &NKesus::CreateKesusTablet, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.SchemeShard] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateFlatTxSchemeShard, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.Hive] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &CreateDefaultHive, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.SysViewProcessor] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &NSysView::CreateSysViewProcessorForTests, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.SequenceShard] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &NSequenceShard::CreateSequenceShard, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
        localConfig.TabletClassInfo[appData.DefaultTabletTypes.ReplicationController] =
            TLocalConfig::TTabletClassInfo(new TTabletSetupInfo(
                &NReplication::CreateController, TMailboxType::Revolving, appData.UserPoolId,
                TMailboxType::Revolving, appData.SystemPoolId));
    }

    void TServer::SetupLocalService(ui32 nodeIdx, const TString &domainName) {
        TLocalConfig::TPtr localConfig = new TLocalConfig();
        auto &appData = Runtime->GetAppData(nodeIdx);
        SetupLocalConfig(*localConfig, appData);

        TTenantPoolConfig::TPtr tenantPoolConfig = new TTenantPoolConfig(localConfig);
        tenantPoolConfig->AddStaticSlot(domainName);

        auto poolId = Runtime->Register(CreateTenantPool(tenantPoolConfig), nodeIdx, appData.SystemPoolId,
                                        TMailboxType::Revolving, 0);
        Runtime->RegisterService(MakeTenantPoolRootID(), poolId, nodeIdx);
        if (Settings->EnableConfigsDispatcher) {
            auto *dispatcher = NConsole::CreateConfigsDispatcher(Settings->AppConfig);
            auto aid = Runtime->Register(dispatcher, nodeIdx, appData.SystemPoolId, TMailboxType::Revolving, 0);
            Runtime->RegisterService(NConsole::MakeConfigsDispatcherID(Runtime->GetNodeId(nodeIdx)), aid);
        }
        Runtime->Register(CreateLabelsMaintainer({}), nodeIdx, appData.SystemPoolId, TMailboxType::Revolving, 0);

        auto sysViewService = NSysView::CreateSysViewServiceForTests();
        TActorId sysViewServiceId = Runtime->Register(sysViewService.Release(), nodeIdx);
        Runtime->RegisterService(NSysView::MakeSysViewServiceID(Runtime->GetNodeId(nodeIdx)), sysViewServiceId, nodeIdx);

        auto tenantPublisher = CreateTenantNodeEnumerationPublisher();
        Runtime->Register(tenantPublisher, nodeIdx);
    }

    void TServer::SetupConfigurators(ui32 nodeIdx) {
        auto &appData = Runtime->GetAppData(nodeIdx);
        Runtime->Register(NConsole::CreateImmediateControlsConfigurator(appData.Icb, Settings->Controls),
                          nodeIdx, appData.SystemPoolId, TMailboxType::Revolving, 0);
    }

    void TServer::SetupProxies(ui32 nodeIdx) {
        Runtime->SetTxAllocatorTabletIds({ChangeStateStorage(TxAllocator, Settings->Domain)});
        {
            IActor* ticketParser = Settings->CreateTicketParser(Settings->AuthConfig);
            TActorId ticketParserId = Runtime->Register(ticketParser, nodeIdx);
            Runtime->RegisterService(MakeTicketParserID(), ticketParserId, nodeIdx);
        }

        {
            IActor* healthCheck = NHealthCheck::CreateHealthCheckService();
            TActorId healthCheckId = Runtime->Register(healthCheck, nodeIdx);
            Runtime->RegisterService(NHealthCheck::MakeHealthCheckID(), healthCheckId, nodeIdx);
        }

        {
            IActor* kqpRmService = NKqp::CreateKqpResourceManagerActor(Settings->AppConfig.GetTableServiceConfig().GetResourceManager(), nullptr);
            TActorId kqpRmServiceId = Runtime->Register(kqpRmService, nodeIdx);
            Runtime->RegisterService(NKqp::MakeKqpRmServiceID(Runtime->GetNodeId(nodeIdx)), kqpRmServiceId, nodeIdx);
        }

        {
            IActor* kqpProxyService = NKqp::CreateKqpProxyService(Settings->AppConfig.GetLogConfig(),
                                                                  Settings->AppConfig.GetTableServiceConfig(),
                                                                  TVector<NKikimrKqp::TKqpSetting>(Settings->KqpSettings),
                                                                  nullptr); 
            TActorId kqpProxyServiceId = Runtime->Register(kqpProxyService, nodeIdx);
            Runtime->RegisterService(NKqp::MakeKqpProxyID(Runtime->GetNodeId(nodeIdx)), kqpProxyServiceId, nodeIdx);
        }

        {
            IActor* txProxy = CreateTxProxy(Runtime->GetTxAllocatorTabletIds());
            TActorId txProxyId = Runtime->Register(txProxy, nodeIdx);
            Runtime->RegisterService(MakeTxProxyID(), txProxyId, nodeIdx);
        }

        {
            IActor* compileService = CreateMiniKQLCompileService(100000);
            TActorId compileServiceId = Runtime->Register(compileService, nodeIdx, Runtime->GetAppData(nodeIdx).SystemPoolId, TMailboxType::Revolving, 0);
            Runtime->RegisterService(MakeMiniKQLCompileServiceID(), compileServiceId, nodeIdx);
        }

        {
            IActor* longTxService = NLongTxService::CreateLongTxService();
            TActorId longTxServiceId = Runtime->Register(longTxService, nodeIdx);
            Runtime->RegisterService(NLongTxService::MakeLongTxServiceID(Runtime->GetNodeId(nodeIdx)), longTxServiceId, nodeIdx);
        }

        {
            IActor* sequenceProxy = NSequenceProxy::CreateSequenceProxy();
            TActorId sequenceProxyId = Runtime->Register(sequenceProxy, nodeIdx);
            Runtime->RegisterService(NSequenceProxy::MakeSequenceProxyServiceID(), sequenceProxyId, nodeIdx);
        }

        if (BusServer && nodeIdx == 0) { // MsgBus and GRPC are run now only on first node
            {
                IActor* proxy = BusServer->CreateProxy();
                TActorId proxyId = Runtime->Register(proxy, nodeIdx, Runtime->GetAppData(nodeIdx).SystemPoolId, TMailboxType::Revolving, 0);
                Runtime->RegisterService(NMsgBusProxy::CreateMsgBusProxyId(), proxyId, nodeIdx);
            }

            {
                IActor* traceService = BusServer->CreateMessageBusTraceService();
                if (traceService) {
                    TActorId traceServiceId = Runtime->Register(traceService, nodeIdx, Runtime->GetAppData(nodeIdx).IOPoolId, TMailboxType::Simple, 0);
                    Runtime->RegisterService(NMessageBusTracer::MakeMessageBusTraceServiceID(), traceServiceId, nodeIdx);
                }
            }
        }

        {
            auto driverConfig = NYdb::TDriverConfig().SetEndpoint(TStringBuilder() << "localhost:" << Settings->GrpcPort);
            if (!Driver) {
                Driver.Reset(new NYdb::TDriver(driverConfig));
            }
            Runtime->GetAppData(nodeIdx).YdbDriver = Driver.Get();
        }

        {
            if (Settings->PQConfig.GetEnabled() == true) {
                IActor *pqMetaCache = NMsgBusProxy::NPqMetaCacheV2::CreatePQMetaCache(
                        new NMonitoring::TDynamicCounters(), TDuration::Seconds(1)
                );

                TActorId pqMetaCacheId = Runtime->Register(pqMetaCache, nodeIdx);
                Runtime->RegisterService(NMsgBusProxy::CreatePersQueueMetaCacheV2Id(), pqMetaCacheId, nodeIdx);
            }
        }

        {
            if (Settings->EnableMetering) {
                THolder<TFileLogBackend> fileBackend;
                try {
                    fileBackend = MakeHolder<TFileLogBackend>(Settings->MeteringFilePath);
                    auto meteringActor = NMetering::CreateMeteringWriter(std::move(fileBackend));
                    TActorId meteringId = Runtime->Register(meteringActor.Release(), nodeIdx);
                    Runtime->RegisterService(NMetering::MakeMeteringServiceID(), meteringId, nodeIdx);

                } catch (const TFileError &ex) {
                    Cerr << "TMeteringWriterInitializer: failed to open file '" << Settings->MeteringFilePath << "': "
                         << ex.what() << Endl;
                }
            }
        }

        {
            IActor* kesusService = NKesus::CreateKesusProxyService();
            TActorId kesusServiceId = Runtime->Register(kesusService, nodeIdx);
            Runtime->RegisterService(NKesus::MakeKesusProxyServiceId(), kesusServiceId, nodeIdx);
        }

        {
            IActor* pqClusterTracker = NPQ::NClusterTracker::CreateClusterTracker();
            TActorId pqClusterTrackerId = Runtime->Register(pqClusterTracker, nodeIdx);
            Runtime->RegisterService(NPQ::NClusterTracker::MakeClusterTrackerID(), pqClusterTrackerId, nodeIdx);
        }

        {
            IActor* netClassifier = NNetClassifier::CreateNetClassifier();
            TActorId netClassifierId = Runtime->Register(netClassifier, nodeIdx);
            Runtime->RegisterService(NNetClassifier::MakeNetClassifierID(), netClassifierId, nodeIdx);
        }

        if (Settings->EnableYq) {
            NYq::NConfig::TConfig protoConfig;
            protoConfig.SetEnabled(true);

            protoConfig.MutableCommon()->SetIdsPrefix("id");

            TString endpoint = TStringBuilder() << "localhost:" << Settings->GrpcPort;
            TString prefix = "Root/yq";
            auto port = Runtime->GetPortManager().GetPort();
            TString ydbMvpEndpoint = TStringBuilder()
                << "http://localhost:"
                << port
                << "/yql-mock/abc";

            {
                auto& controlPlaneProxyConfig = *protoConfig.MutableControlPlaneProxy();
                controlPlaneProxyConfig.SetEnabled(true);
            }

            {
                auto& testConnectionConfig = *protoConfig.MutableTestConnection();
                testConnectionConfig.SetEnabled(true);
            }

            {
                auto& controlPlaneStorageConfig = *protoConfig.MutableControlPlaneStorage();
                controlPlaneStorageConfig.SetEnabled(true);
                controlPlaneStorageConfig.SetUseInMemory(Settings->AppConfig.GetYandexQueryConfig().GetControlPlaneStorage().GetUseInMemory());
                auto& storage = *controlPlaneStorageConfig.MutableStorage();
                storage.SetEndpoint(endpoint);
                storage.SetTablePrefix(prefix);

                controlPlaneStorageConfig.AddAvailableBinding("DATA_STREAMS");
                controlPlaneStorageConfig.AddAvailableBinding("OBJECT_STORAGE");

                controlPlaneStorageConfig.AddAvailableConnection("YDB_DATABASE");
                controlPlaneStorageConfig.AddAvailableConnection("CLICKHOUSE_CLUSTER");
                controlPlaneStorageConfig.AddAvailableConnection("DATA_STREAMS");
                controlPlaneStorageConfig.AddAvailableConnection("OBJECT_STORAGE");
                controlPlaneStorageConfig.AddAvailableConnection("MONITORING");
            }

            {
                auto& commonConfig = *protoConfig.MutableCommon();
                commonConfig.SetYdbMvpCloudEndpoint(ydbMvpEndpoint);
                commonConfig.SetIdsPrefix("ut");
            }

            {
                auto& privateApiConfig = *protoConfig.MutablePrivateApi();
                privateApiConfig.SetEnabled(true);
                privateApiConfig.SetTaskServiceEndpoint(endpoint);
                privateApiConfig.SetTaskServiceDatabase("Root");
            }

            {
                auto& tokenAccessorConfig = *protoConfig.MutableTokenAccessor();
                tokenAccessorConfig.SetEnabled(true);
            }

            {
                auto& dbPoolConfig = *protoConfig.MutableDbPool();
                dbPoolConfig.SetEnabled(true);
                auto& storage = *dbPoolConfig.MutableStorage();
                storage.SetEndpoint(endpoint);
                storage.SetTablePrefix(prefix);
            }

            {
                auto& resourceManagerConfig = *protoConfig.MutableResourceManager();
                resourceManagerConfig.SetEnabled(true);
            }

            {
                auto& privateProxyConfig = *protoConfig.MutablePrivateProxy();
                privateProxyConfig.SetEnabled(true);
            }

            {
                auto& nodesManagerConfig = *protoConfig.MutableNodesManager();
                nodesManagerConfig.SetEnabled(true);
            }

            {
                auto& pendingFetcherConfig = *protoConfig.MutablePendingFetcher();
                pendingFetcherConfig.SetEnabled(true);
            }

            auto& appData = Runtime->GetAppData();

            auto actorRegistrator = [&](NActors::TActorId serviceActorId, NActors::IActor* actor) {
                auto actorId = Runtime->Register(actor, nodeIdx);
                Runtime->RegisterService(serviceActorId, actorId, nodeIdx);
            };

            const auto ydbCredFactory = NKikimr::CreateYdbCredentialsProviderFactory;
            auto counters = MakeIntrusive<NMonitoring::TDynamicCounters>();
            auto yqSharedResources = NYq::CreateYqSharedResources(protoConfig, ydbCredFactory, counters);
            NYq::Init(
                protoConfig,
                Runtime->GetNodeId(nodeIdx),
                actorRegistrator,
                &appData,
                "TestTenant",
                nullptr, // MakeIntrusive<NPq::NConfigurationManager::TConnections>(),
                yqSharedResources,
                NKikimr::NFolderService::CreateMockFolderServiceActor,
                NYq::CreateMockYqAuditServiceActor,
                ydbCredFactory,
                /*IcPort = */0
                );
            NYq::InitTest(Runtime.Get(), port, Settings->GrpcPort, yqSharedResources);
        }
    }

    void TServer::SetupLogging() {
        Runtime->SetLogPriority(NKikimrServices::FLAT_TX_SCHEMESHARD, NLog::PRI_WARN);
        //Runtime->SetLogPriority(NKikimrServices::SCHEMESHARD_DESCRIBE, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::HIVE, NActors::NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::LOCAL, NActors::NLog::PRI_DEBUG);

        Runtime->SetLogPriority(NKikimrServices::BS_CONTROLLER, NLog::PRI_WARN);
        Runtime->SetLogPriority(NKikimrServices::MSGBUS_REQUEST, NLog::PRI_WARN);

        //Runtime->SetLogPriority(NKikimrServices::TX_COORDINATOR, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::TX_MEDIATOR, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::TX_PROXY, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::TX_PROXY_SCHEME_CACHE, NLog::PRI_DEBUG);

        //Runtime->SetLogPriority(NKikimrServices::MINIKQL_ENGINE, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::KQP_PROXY, NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::KQP_WORKER, NLog::PRI_DEBUG);

        //Runtime->SetLogPriority(NKikimrServices::TX_PROXY, NActors::NLog::PRI_TRACE);
        //Runtime->SetLogPriority(NKikimrServices::TX_PROXY_SCHEME_CACHE, NActors::NLog::PRI_TRACE);
        //Runtime->SetLogPriority(NKikimrServices::SCHEME_BOARD_REPLICA, NActors::NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::SCHEME_BOARD_POPULATOR, NActors::NLog::PRI_DEBUG);
        //Runtime->SetLogPriority(NKikimrServices::SCHEME_BOARD_SUBSCRIBER, NActors::NLog::PRI_TRACE);
        //Runtime->SetLogPriority(NKikimrServices::YQL_PROXY, NActors::NLog::PRI_DEBUG);

        if (Settings->LoggerInitializer) {
            Settings->LoggerInitializer(*Runtime);
        }
    }

    void TServer::StartDummyTablets() {
        if (!Runtime)
            ythrow TWithBackTrace<yexception>() << "Server is redirected";

        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(DummyTablet1, Settings->Domain), TTabletTypes::TX_DUMMY), &CreateFlatDummyTablet);
        CreateTestBootstrapper(*Runtime, CreateTestTabletInfo(ChangeStateStorage(DummyTablet2, Settings->Domain), TTabletTypes::TX_DUMMY), &CreateFlatDummyTablet);
    }

    TTestActorRuntime* TServer::GetRuntime() const {
        return Runtime.Get();
    }

    const TServerSettings &TServer::GetSettings() const {
        return *Settings;
    }

    const NScheme::TTypeRegistry* TServer::GetTypeRegistry() {
        return Runtime->GetAppData().TypeRegistry;
    }

    const NMiniKQL::IFunctionRegistry* TServer::GetFunctionRegistry() {
        return Runtime->GetAppData().FunctionRegistry;
    }

    TServer::~TServer() {
        if (Runtime->GetAppData().Mon) {
            Runtime->GetAppData().Mon->Stop();
        }

        if (GRpcServer) {
            GRpcServer->Stop();
        }

        if (Runtime) {
            Runtime.Destroy();
        }

        if (Bus) {
            Bus->Stop();
            Bus.Drop();
        }
    }


    TClient::TClient(const TServerSettings& settings)
        : Domain(settings.Domain)
        , DomainName(settings.DomainName)
        , SupportsRedirect(settings.SupportsRedirect)
        , StoragePoolTypes(settings.StoragePoolTypes)
        , FunctionRegistry(NKikimr::NMiniKQL::CreateFunctionRegistry(NKikimr::NMiniKQL::CreateBuiltinRegistry()))
        , LoadedFunctionRegistry(NKikimr::NMiniKQL::CreateFunctionRegistry(NKikimr::NMiniKQL::CreateBuiltinRegistry()))
    {
        TServerSetup serverSetup;
        if (SupportsRedirect && Tests::IsServerRedirected()) {
            serverSetup = GetServerSetup();
        } else {
            serverSetup = TServerSetup("localhost", settings.Port);
        }

        ClientConfig.Ip = serverSetup.IpAddress;
        ClientConfig.Port = serverSetup.Port;
        ClientConfig.BusSessionConfig.TotalTimeout = Max<int>() / 2;
        ClientConfig.BusSessionConfig.ConnectTimeout = ConnectTimeoutMilliSeconds;
        ClientConfig.BusSessionConfig.NumRetries = 10;
        Client.reset(new NMsgBusProxy::TMsgBusClient(ClientConfig)); 
        Client->Init();
    }

    const NMsgBusProxy::TMsgBusClientConfig& TClient::GetClientConfig() const {
        return ClientConfig;
    }

    TClient::~TClient() {
        Client->Shutdown();
    }


    std::shared_ptr<NMsgBusProxy::TMsgBusClient> TClient::GetClient() const { 
        return Client;
    }

    const NScheme::TTypeRegistry& TClient::GetTypeRegistry() const {
        return TypeRegistry;
    }

    bool TClient::LoadTypes() {
        TAutoPtr<NMsgBusProxy::TBusTypesRequest> request(new NMsgBusProxy::TBusTypesRequest());
        if (TypesEtag.Defined()) {
            request->Record.SetETag(*TypesEtag.Get());
        }

        TAutoPtr<NBus::TBusMessage> reply;
        UNIT_ASSERT_VALUES_EQUAL(SyncCall(request, reply), NBus::MESSAGE_OK);
        const NKikimrClient::TTypeMetadataResponse &response = static_cast<NMsgBusProxy::TBusTypesResponse*>(reply.Get())->Record;
        UNIT_ASSERT_VALUES_EQUAL((ui32)NMsgBusProxy::MSTATUS_OK, response.GetStatus());
        if (!response.HasETag()) {
            UNIT_ASSERT(TypesEtag.Defined());
            return false;
        }

        UNIT_ASSERT(response.HasTypeMetadata() && response.HasFunctionMetadata());
        DeserializeMetadata(response.GetTypeMetadata(), &LoadedTypeMetadataRegistry);
        DeserializeMetadata(response.GetFunctionMetadata(), *LoadedFunctionRegistry->GetBuiltins());
        TypesEtag = response.GetETag();
        return true;
    }

    const NScheme::TTypeMetadataRegistry& TClient::GetTypeMetadataRegistry() const {
        if (TypesEtag.Defined())
            return LoadedTypeMetadataRegistry;

        return TypeRegistry.GetTypeMetadataRegistry();
    }

    const NMiniKQL::IFunctionRegistry& TClient::GetFunctionRegistry() const {
        if (TypesEtag.Defined())
            return *LoadedFunctionRegistry;

        return *FunctionRegistry;
    }

    ui64 TClient::GetPatchedSchemeRoot(ui64 schemeRoot, ui32 domain, bool supportsRedirect) {
        if (!supportsRedirect || !IsServerRedirected())
            return ChangeStateStorage(schemeRoot, domain);

        TString domainRedirect = GetEnv(DomainRedirectEnvVar);
        if (!domainRedirect)
            ythrow TWithBackTrace<yexception>() << "Please set domain redirect, format: KIKIMR_TEST_DOMAIN=domain/RootShardTabletId";

        TStringBuf domainUidStr;
        TStringBuf tabletIdStr;
        TStringBuf(domainRedirect).Split('/', domainUidStr, tabletIdStr);
        const ui32 domainUid = FromString<ui32>(domainUidStr);
        if (domainUid != domain) {
            ythrow TWithBackTrace<yexception>() << "Mismatch domain redirect, expected domain: " << domain
                << ", redirected domain: " << domainUid;
        }

        return FromString<ui64>(tabletIdStr);
    }

    void TClient::WaitRootIsUp(const TString& root) {
        while (true) {
            TAutoPtr<NMsgBusProxy::TBusResponse> resp = Ls(root);
            UNIT_ASSERT(resp);

            if (resp->Record.GetStatus() == NMsgBusProxy::MSTATUS_OK && resp->Record.GetSchemeStatus() == NKikimrScheme::StatusSuccess) {
                break;
            }
        }
    }

    TAutoPtr<NBus::TBusMessage> TClient::InitRootSchemeWithReply(const TString& root) {
        WaitRootIsUp(root);

        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request = new NMsgBusProxy::TBusSchemeOperation();
        auto* transaction = request->Record.MutableTransaction()->MutableModifyScheme();
        transaction->SetWorkingDir("/");
        transaction->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterSubDomain);
        auto op = transaction->MutableSubDomain();
        op->SetName(root);

        for (const auto& [kind, pool] : StoragePoolTypes) {
            auto* p = op->AddStoragePools();
            p->SetKind(kind);
            p->SetName(pool.GetName());
        }

        TAutoPtr<NBus::TBusMessage> reply;
        SendAndWaitCompletion(request, reply);

#ifndef NDEBUG
        Cout << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        return reply;
    }

    void TClient::InitRootScheme() {
        InitRootScheme(DomainName);
    }

    void TClient::InitRootScheme(const TString& root) {
        TAutoPtr<NBus::TBusMessage> reply = InitRootSchemeWithReply(root);
        auto resp = dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Get());
        UNIT_ASSERT(resp);
        UNIT_ASSERT_VALUES_EQUAL(resp->Record.GetStatus(), NMsgBusProxy::MSTATUS_OK);
    }


    void TClient::ExecuteTraceCommand(NKikimrClient::TMessageBusTraceRequest::ECommand command, const TString &path) {
        TAutoPtr<NMsgBusProxy::TBusMessageBusTraceRequest> request(new NMsgBusProxy::TBusMessageBusTraceRequest());
        request->Record.SetCommand(command);
        if (path)
            request->Record.SetPath(path);
        TAutoPtr<NBus::TBusMessage> reply;
        UNIT_ASSERT_VALUES_EQUAL(SyncCall(request, reply), NBus::MESSAGE_OK);
    }

    TString TClient::StartTrace(const TString &path) {
        TAutoPtr<NMsgBusProxy::TBusMessageBusTraceRequest> request(new NMsgBusProxy::TBusMessageBusTraceRequest());
        request->Record.SetCommand(NKikimrClient::TMessageBusTraceRequest::START);
        if (path)
            request->Record.SetPath(path);
        TAutoPtr<NBus::TBusMessage> reply;
        UNIT_ASSERT_VALUES_EQUAL(SyncCall(request, reply), NBus::MESSAGE_OK);
        if (reply.Get()->GetHeader()->Type == NMsgBusProxy::MTYPE_CLIENT_MESSAGE_BUS_TRACE_STATUS) {
            const NKikimrClient::TMessageBusTraceStatus &response = static_cast<NMsgBusProxy::TBusMessageBusTraceStatus *>(reply.Get())->Record;
            return response.GetPath();
        } else {
            ythrow yexception() << "MessageBus trace not enabled on the server (see mbus/--trace-path option)";
        }
    }

    void TClient::StopTrace() {
        TAutoPtr<NMsgBusProxy::TBusMessageBusTraceRequest> request(new NMsgBusProxy::TBusMessageBusTraceRequest());
        request->Record.SetCommand(NKikimrClient::TMessageBusTraceRequest::STOP);
        TAutoPtr<NBus::TBusMessage> reply;
        UNIT_ASSERT_VALUES_EQUAL(SyncCall(request, reply), NBus::MESSAGE_OK);
    }

    NBus::EMessageStatus TClient::WaitCompletion(ui64 txId, ui64 schemeshard, ui64 pathId,
                                        TAutoPtr<NBus::TBusMessage>& reply,
                                        TDuration timeout)
    {
        auto deadline = TInstant::Now() + timeout;

        NBus::EMessageStatus status;
        const NKikimrClient::TResponse* response = nullptr;
        do {
            TAutoPtr<NMsgBusProxy::TBusSchemeOperationStatus> msg = new NMsgBusProxy::TBusSchemeOperationStatus();
            msg->Record.MutableFlatTxId()->SetTxId(txId);
            msg->Record.MutableFlatTxId()->SetSchemeShardTabletId(schemeshard);
            msg->Record.MutableFlatTxId()->SetPathId(pathId);
            msg->Record.MutablePollOptions()->SetTimeout(timeout.MilliSeconds());
#ifndef NDEBUG
            Cerr << "waiting..." << Endl;
#endif
            status = SyncCall(msg, reply);
            if (status != NBus::MESSAGE_OK) {
                const char *description = NBus::MessageStatusDescription(status);
                Cerr << description << Endl;
                return status;
            }
            if (reply->GetHeader()->Type != NMsgBusProxy::MTYPE_CLIENT_RESPONSE) {
                break;
            }
            response = &static_cast<NMsgBusProxy::TBusResponse*>(reply.Get())->Record;
        } while (response->GetStatus() == NMsgBusProxy::MSTATUS_INPROGRESS && deadline >= TInstant::Now());

        return status;
    }

    NBus::EMessageStatus TClient::SendAndWaitCompletion(TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request,
                                                        TAutoPtr<NBus::TBusMessage>& reply,
                                                        TDuration timeout) {
        NBus::EMessageStatus status = SendWhenReady(request, reply, timeout.MilliSeconds());

        if (status != NBus::MESSAGE_OK) {
            return status;
        }

        const NMsgBusProxy::TBusResponse* flatResponse = dynamic_cast<const NMsgBusProxy::TBusResponse*>(reply.Get());
        if (!flatResponse)
            return NBus::MESSAGE_UNKNOWN;

        const NKikimrClient::TResponse* response = &flatResponse->Record;

        if (response->HasErrorReason()) {
            Cerr << "reason: " << response->GetErrorReason() << Endl;
        }

        if (response->GetStatus() != NMsgBusProxy::MSTATUS_INPROGRESS) {
            return status;
        }

        NKikimrClient::TFlatTxId txId = response->GetFlatTxId();
        return WaitCompletion(txId.GetTxId(), txId.GetSchemeShardTabletId(), txId.GetPathId(), reply, timeout);
    }

    NMsgBusProxy::EResponseStatus TClient::MkDir(const TString& parent, const TString& name, const TApplyIf& applyIf) {
        NMsgBusProxy::TBusSchemeOperation* request(new NMsgBusProxy::TBusSchemeOperation());
        auto* mkDirTx = request->Record.MutableTransaction()->MutableModifyScheme();
        mkDirTx->SetWorkingDir(parent);
        mkDirTx->SetOperationType(NKikimrSchemeOp::ESchemeOpMkDir);
        mkDirTx->MutableMkDir()->SetName(name);
        SetApplyIf(*mkDirTx, applyIf);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendAndWaitCompletion(request, reply);
#ifndef NDEBUG
        Cout << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::RmDir(const TString& parent, const TString& name, const TApplyIf& applyIf) {
        NMsgBusProxy::TBusSchemeOperation* request(new NMsgBusProxy::TBusSchemeOperation());
        auto* mkDirTx = request->Record.MutableTransaction()->MutableModifyScheme();
        mkDirTx->SetWorkingDir(parent);
        mkDirTx->SetOperationType(NKikimrSchemeOp::ESchemeOpRmDir);
        mkDirTx->MutableDrop()->SetName(name);
        SetApplyIf(*mkDirTx, applyIf);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendAndWaitCompletion(request, reply);
#ifndef NDEBUG
        Cout << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateSubdomain(const TString &parent, const TString &description) {
        NKikimrSubDomains::TSubDomainSettings subdomain;
        UNIT_ASSERT(::google::protobuf::TextFormat::ParseFromString(description, &subdomain));
        return CreateSubdomain(parent, subdomain);
    }

    NMsgBusProxy::EResponseStatus TClient::CreateSubdomain(const TString &parent, const NKikimrSubDomains::TSubDomainSettings &subdomain) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateSubDomain);
        op->SetWorkingDir(parent);
        op->MutableSubDomain()->CopyFrom(subdomain);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateExtSubdomain(const TString &parent, const TString &description) {
        NKikimrSubDomains::TSubDomainSettings subdomain;
        UNIT_ASSERT(::google::protobuf::TextFormat::ParseFromString(description, &subdomain));
        return CreateExtSubdomain(parent, subdomain);
    }

    NMsgBusProxy::EResponseStatus TClient::CreateExtSubdomain(const TString &parent, const NKikimrSubDomains::TSubDomainSettings &subdomain) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateExtSubDomain);
        op->SetWorkingDir(parent);
        op->MutableSubDomain()->CopyFrom(subdomain);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::AlterUserAttributes(const TString &parent, const TString &name, const TVector<std::pair<TString, TString>>& addAttrs, const TVector<TString>& dropAttrs, const TApplyIf& applyIf) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetWorkingDir(parent);
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterUserAttributes);
        auto userAttributes = op->MutableAlterUserAttributes();
        userAttributes->SetPathName(name);
        for (const auto& item: addAttrs) {
            auto attr = userAttributes->AddUserAttributes();
            attr->SetKey(item.first);
            attr->SetValue(item.second);
        }
        for (const auto& item: dropAttrs) {
            auto attr = userAttributes->AddUserAttributes();
            attr->SetKey(item);
        }
        SetApplyIf(*op, applyIf);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::AlterSubdomain(const TString &parent, const TString &description, TDuration timeout) {
        NKikimrSubDomains::TSubDomainSettings subdomain;
        UNIT_ASSERT(::google::protobuf::TextFormat::ParseFromString(description, &subdomain));
        return AlterSubdomain(parent, subdomain, timeout);
    }

    NMsgBusProxy::EResponseStatus TClient::AlterSubdomain(const TString &parent, const NKikimrSubDomains::TSubDomainSettings &subdomain, TDuration timeout) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterSubDomain);
        op->SetWorkingDir(parent);
        op->MutableSubDomain()->CopyFrom(subdomain);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::AlterExtSubdomain(const TString &parent, const NKikimrSubDomains::TSubDomainSettings &subdomain, TDuration timeout) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterExtSubDomain);
        op->SetWorkingDir(parent);
        op->MutableSubDomain()->CopyFrom(subdomain);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::DeleteSubdomain(const TString &parent, const TString &name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpDropSubDomain);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::ForceDeleteSubdomain(const TString &parent, const TString &name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpForceDropSubDomain);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::ForceDeleteUnsafe(const TString &parent, const TString &name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpForceDropUnsafe);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateUser(const TString& parent, const TString& user, const TString& password) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto* op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterLogin);
        op->SetWorkingDir(parent);

        auto* createUser = op->MutableAlterLogin()->MutableCreateUser();
        createUser->SetUser(user);
        createUser->SetPassword(password);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateTable(const TString& parent, const NKikimrSchemeOp::TTableDescription &table, TDuration timeout) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateTable);
        op->SetWorkingDir(parent);
        op->MutableCreateTable()->CopyFrom(table);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateTableWithUniformShardedIndex(const TString& parent,
        const NKikimrSchemeOp::TTableDescription &table, const TString& indexName, const TVector<TString> indexColumns, TDuration timeout)
    {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateIndexedTable);
        op->SetWorkingDir(parent);

        NKikimrSchemeOp::TTableDescription* tableDesc = op->MutableCreateIndexedTable()->MutableTableDescription();
        tableDesc->CopyFrom(table);

        {
            auto indexDesc = op->MutableCreateIndexedTable()->MutableIndexDescription()->Add();
            indexDesc->SetName(indexName);
            for (const auto& c : indexColumns) {
                indexDesc->AddKeyColumnNames(c);
            }

            indexDesc->SetType(NKikimrSchemeOp::EIndexType::EIndexTypeGlobal);
            indexDesc->MutableIndexImplTableDescription()->SetUniformPartitionsCount(16);
        }

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::SplitTable(const TString& table, ui64 datashardId, ui64 border, TDuration timeout) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpSplitMergeTablePartitions);
        auto split = op->MutableSplitMergeTablePartitions();
        split->SetTablePath(table);
        split->AddSourceTabletId(datashardId);
        split->AddSplitBoundary()->MutableKeyPrefix()->AddTuple()->MutableOptional()->SetUint64(border);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CopyTable(const TString &parent, const TString &name, const TString &src) {
        NKikimrSchemeOp::TTableDescription table;
        table.SetName(name);
        table.SetCopyFromTable(src);
        return CreateTable(parent, table, TDuration::Seconds(5000));
    }

    NMsgBusProxy::EResponseStatus TClient::ConsistentCopyTables(TVector<std::pair<TString, TString>> desc, TDuration timeout) {
        NKikimrSchemeOp::TConsistentTableCopyingConfig coping;
        for (auto& task: desc) {
            auto* item = coping.AddCopyTableDescriptions();
            item->SetSrcPath(std::move(task.first));
            item->SetDstPath(std::move(task.second));
        }

        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateConsistentCopyTables);
        *op->MutableCreateConsistentCopyTables() = std::move(coping);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply, timeout);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateTable(const TString& parent, const TString& scheme, TDuration timeout) {
        NKikimrSchemeOp::TTableDescription table;
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(scheme, &table);
        UNIT_ASSERT(parseOk);
        return CreateTable(parent, table, timeout);
    }

    NMsgBusProxy::EResponseStatus TClient::CreateKesus(const TString& parent, const TString& name) {
        auto* request = new NMsgBusProxy::TBusSchemeOperation();
        auto* tx = request->Record.MutableTransaction()->MutableModifyScheme();
        tx->SetOperationType(NKikimrSchemeOp::ESchemeOpCreateKesus);
        tx->SetWorkingDir(parent);
        tx->MutableKesus()->SetName(name);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendAndWaitCompletion(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::DeleteKesus(const TString& parent, const TString& name) {
        auto* request = new NMsgBusProxy::TBusSchemeOperation();
        auto* tx = request->Record.MutableTransaction()->MutableModifyScheme();
        tx->SetOperationType(NKikimrSchemeOp::ESchemeOpDropKesus);
        tx->SetWorkingDir(parent);
        tx->MutableDrop()->SetName(name);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendAndWaitCompletion(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateOlapStore(const TString& parent, const TString& scheme) {
        NKikimrSchemeOp::TColumnStoreDescription store;
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(scheme, &store);
        UNIT_ASSERT(parseOk);
        return CreateOlapStore(parent, store);
    }

    NMsgBusProxy::EResponseStatus TClient::CreateOlapStore(const TString& parent,
                                                           const NKikimrSchemeOp::TColumnStoreDescription& store) {
        auto request = std::make_unique<NMsgBusProxy::TBusSchemeOperation>();
        auto* op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateColumnStore);
        op->SetWorkingDir(parent);
        op->MutableCreateColumnStore()->CopyFrom(store);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse& response = dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateOlapTable(const TString& parent, const TString& scheme) {
        NKikimrSchemeOp::TColumnTableDescription table;
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(scheme, &table);
        UNIT_ASSERT(parseOk);
        return CreateOlapTable(parent, table);
    }

    NMsgBusProxy::EResponseStatus TClient::CreateOlapTable(const TString& parent,
                                                           const NKikimrSchemeOp::TColumnTableDescription& table) {
        auto request = std::make_unique<NMsgBusProxy::TBusSchemeOperation>();
        auto* op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpCreateColumnTable);
        op->SetWorkingDir(parent);
        op->MutableCreateColumnTable()->CopyFrom(table);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse& response = dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::CreateSolomon(const TString& parent, const TString& name, ui32 parts, ui32 channelProfile) {
        auto* request = new NMsgBusProxy::TBusSchemeOperation();
        auto* tx = request->Record.MutableTransaction()->MutableModifyScheme();
        tx->SetOperationType(NKikimrSchemeOp::ESchemeOpCreateSolomonVolume);
        tx->SetWorkingDir(parent);
        tx->MutableCreateSolomonVolume()->SetName(name);
        tx->MutableCreateSolomonVolume()->SetPartitionCount(parts);
        tx->MutableCreateSolomonVolume()->SetChannelProfileId(channelProfile);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendAndWaitCompletion(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::AlterTable(const TString& parent, const NKikimrSchemeOp::TTableDescription& alter, const TString& userToken) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpAlterTable);
        op->SetWorkingDir(parent);
        op->MutableAlterTable()->CopyFrom(alter);
        TAutoPtr<NBus::TBusMessage> reply;
        if (userToken) {
            request->Record.SetSecurityToken(userToken);
        }
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        return dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Release());
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::AlterTable(const TString& parent, const TString& alter, const TString& userToken) {
        NKikimrSchemeOp::TTableDescription table;
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(alter, &table);
        UNIT_ASSERT(parseOk);
        return AlterTable(parent, table, userToken);
    }

    NMsgBusProxy::EResponseStatus TClient::AlterTable(const TString& parent, const NKikimrSchemeOp::TTableDescription& alter) {
        TAutoPtr<NMsgBusProxy::TBusResponse> reply = AlterTable(parent, alter, TString());
        const NKikimrClient::TResponse &response = reply->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::AlterTable(const TString& parent, const TString& alter) {
        NKikimrSchemeOp::TTableDescription table;
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(alter, &table);
        UNIT_ASSERT(parseOk);
        return AlterTable(parent, table);
    }

    NMsgBusProxy::EResponseStatus TClient::StoreTableBackup(const TString& parent, const NKikimrSchemeOp::TBackupTask& task) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpBackup);
        op->SetWorkingDir(parent);
        op->MutableBackup()->CopyFrom(task);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::DeleteTable(const TString& parent, const TString& name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpDropTable);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    NMsgBusProxy::EResponseStatus TClient::DeleteTopic(const TString& parent, const TString& name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpDropPersQueueGroup);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::TryDropPersQueueGroup(const TString& parent, const TString& name) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto * op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpDropPersQueueGroup);
        op->SetWorkingDir(parent);
        op->MutableDrop()->SetName(name);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SyncCall(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        NMsgBusProxy::TBusResponse* res = dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Release());
        UNIT_ASSERT(res);
        return res;
    }

    NMsgBusProxy::EResponseStatus TClient::WaitCreateTx(TTestActorRuntime* runtime, const TString& path, TDuration timeout) {
        TAutoPtr<NSchemeShard::TEvSchemeShard::TEvDescribeScheme> request(new NSchemeShard::TEvSchemeShard::TEvDescribeScheme());
        request->Record.SetPath(path);
        const ui64 schemeRoot = GetPatchedSchemeRoot(SchemeRoot, Domain, SupportsRedirect);
        TActorId sender = runtime->AllocateEdgeActor(0);
        ForwardToTablet(*runtime, schemeRoot, sender, request.Release(), 0);

        TAutoPtr<IEventHandle> handle;
        runtime->GrabEdgeEvent<NSchemeShard::TEvSchemeShard::TEvDescribeSchemeResult>(handle);
        auto& record = handle->Get<NSchemeShard::TEvSchemeShard::TEvDescribeSchemeResult>()->GetRecord();
        //Cerr << record.DebugString() << Endl;

        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        auto& descr = record.GetPathDescription().GetSelf();
        TAutoPtr<NBus::TBusMessage> reply;
        auto msgStatus = WaitCompletion(descr.GetCreateTxId(), descr.GetSchemeshardId(), descr.GetPathId(), reply, timeout);
#ifndef NDEBUG
        Cout << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        return (NMsgBusProxy::EResponseStatus)response.GetStatus();
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::LsImpl(const TString& path) {
        TAutoPtr<NMsgBusProxy::TBusSchemeDescribe> request(new NMsgBusProxy::TBusSchemeDescribe());
        request->Record.SetPath(path);
        request->Record.MutableOptions()->SetShowPrivateTable(true);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendWhenReady(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
#ifndef NDEBUG
        Cerr << "TClient::Ls: " << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        return dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Release());
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::Ls(const TString& path) {
        return LsImpl(path).Release();
    }

    TClient::TPathVersion TClient::ExtractPathVersion(const TAutoPtr<NMsgBusProxy::TBusResponse>& describe) {
        UNIT_ASSERT(describe.Get());
        auto& record = describe->Record;
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NMsgBusProxy::MSTATUS_OK);
        UNIT_ASSERT_VALUES_EQUAL(record.GetSchemeStatus(), NKikimrScheme::StatusSuccess);

        UNIT_ASSERT(record.HasPathDescription());
        auto& descr = record.GetPathDescription();

        UNIT_ASSERT(descr.HasSelf());
        auto& self = descr.GetSelf();

        return TPathVersion{self.GetSchemeshardId(), self.GetPathId(), self.GetPathVersion()};
    }

    TVector<ui64> TClient::ExtractTableShards(const TAutoPtr<NMsgBusProxy::TBusResponse>& describe) {
        UNIT_ASSERT(describe.Get());
        NKikimrClient::TResponse& record = describe->Record;
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NMsgBusProxy::MSTATUS_OK);
        UNIT_ASSERT_VALUES_EQUAL(record.GetSchemeStatus(), NKikimrScheme::StatusSuccess);

        UNIT_ASSERT(record.HasPathDescription());
        auto& descr = record.GetPathDescription();
        UNIT_ASSERT(descr.TablePartitionsSize() > 0);
        auto& parts = descr.GetTablePartitions();

        TVector<ui64> shards;
        for (const auto& part : parts) {
            shards.emplace_back(part.GetDatashardId());
        }
        return shards;
    }

    void TClient::RefreshPathCache(TTestActorRuntime* runtime, const TString& path, ui32 nodeIdx) {
        TActorId sender = runtime->AllocateEdgeActor(nodeIdx);
        auto request = MakeHolder<NSchemeCache::TSchemeCacheNavigate>();
        auto& entry = request->ResultSet.emplace_back();
        entry.Path = SplitPath(path);
        entry.Operation = NSchemeCache::TSchemeCacheNavigate::OpPath;
        runtime->Send(
            new IEventHandle(
                MakeSchemeCacheID(),
                sender,
                new TEvTxProxySchemeCache::TEvNavigateKeySet(request.Release())),
            nodeIdx);
        auto ev = runtime->GrabEdgeEvent<TEvTxProxySchemeCache::TEvNavigateKeySetResult>(sender);
        Y_VERIFY(ev);
    }

    void TClient::ModifyOwner(const TString& parent, const TString& name, const TString& owner) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpModifyACL);
        op->SetWorkingDir(parent);
        op->MutableModifyACL()->SetName(name);
        op->MutableModifyACL()->SetNewOwner(owner);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
    }

    void TClient::ModifyACL(const TString& parent, const TString& name, const TString& acl) {
        TAutoPtr<NMsgBusProxy::TBusSchemeOperation> request(new NMsgBusProxy::TBusSchemeOperation());
        auto *op = request->Record.MutableTransaction()->MutableModifyScheme();
        op->SetOperationType(NKikimrSchemeOp::EOperationType::ESchemeOpModifyACL);
        op->SetWorkingDir(parent);
        op->MutableModifyACL()->SetName(name);
        op->MutableModifyACL()->SetDiffACL(acl);
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SendAndWaitCompletion(request.Release(), reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
    }

    TAutoPtr<NMsgBusProxy::TBusResponse> TClient::HiveCreateTablet(ui32 domainUid, ui64 owner, ui64 owner_index, TTabletTypes::EType tablet_type,
                                                               const TVector<ui32>& allowed_node_ids,
                                                               const TVector<TSubDomainKey>& allowed_domains,
                                                               const TChannelsBindings& bindings)
    {
        TAutoPtr<NMsgBusProxy::TBusHiveCreateTablet> request(new NMsgBusProxy::TBusHiveCreateTablet());
        NKikimrClient::THiveCreateTablet& record = request->Record;
        record.SetDomainUid(domainUid);
        auto *cmdCreate = record.AddCmdCreateTablet();
        cmdCreate->SetOwnerId(owner);
        cmdCreate->SetOwnerIdx(owner_index);
        cmdCreate->SetTabletType(tablet_type);
        for (ui32 node_id: allowed_node_ids) {
            cmdCreate->AddAllowedNodeIDs(node_id);
        }
        for (auto& domain_id: allowed_domains) {
            *cmdCreate->AddAllowedDomains() = domain_id;
        }
        if (!bindings.empty()) {
            for (auto& binding: bindings) {
                *cmdCreate->AddBindedChannels() = binding;
            }
        } else {
            UNIT_ASSERT(!StoragePoolTypes.empty());
            TString storagePool = StoragePoolTypes.begin()->second.GetName();
            cmdCreate->AddBindedChannels()->SetStoragePoolName(storagePool); // 0
            cmdCreate->AddBindedChannels()->SetStoragePoolName(storagePool); // 1
            cmdCreate->AddBindedChannels()->SetStoragePoolName(storagePool); // 2
        }
        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus status = SyncCall(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);
        NMsgBusProxy::TBusResponse* res = dynamic_cast<NMsgBusProxy::TBusResponse*>(reply.Release());
        UNIT_ASSERT(res);
        return res;
    }

    TString TClient::CreateStoragePool(const TString& poolKind, const TString& partOfName, ui32 groups) {
        Y_VERIFY(StoragePoolTypes.contains(poolKind));
        const TString poolName = Sprintf("name_%s_kind_%s", partOfName.c_str(), poolKind.c_str());
        const ui64 poolId = THash<TString>()(poolName);

        NKikimrBlobStorage::TDefineStoragePool storagePool = StoragePoolTypes.at(poolKind);
        Y_VERIFY(storagePool.GetKind() == poolKind);
        storagePool.SetStoragePoolId(poolId);
        storagePool.SetName(poolName);
        storagePool.SetNumGroups(groups);

        TAutoPtr<NMsgBusProxy::TBusBlobStorageConfigRequest> request(new NMsgBusProxy::TBusBlobStorageConfigRequest());
        request->Record.MutableRequest()->AddCommand()->MutableDefineStoragePool()->CopyFrom(storagePool);
        request->Record.SetDomain(Domain);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendWhenReady(request, reply);

        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        Y_VERIFY(response.HasBlobStorageConfigResponse() && response.GetBlobStorageConfigResponse().GetSuccess());
        UNIT_ASSERT((NMsgBusProxy::EResponseStatus)response.GetStatus());

        return poolName;
    }

    NKikimrBlobStorage::TDefineStoragePool TClient::DescribeStoragePool(const TString& name) {
        TAutoPtr<NMsgBusProxy::TBusBlobStorageConfigRequest> readRequest(new NMsgBusProxy::TBusBlobStorageConfigRequest());
        readRequest->Record.SetDomain(Domain);
        auto readParam = readRequest->Record.MutableRequest()->AddCommand()->MutableReadStoragePool();
        readParam->SetBoxId(TServerSettings::BOX_ID);
        readParam->AddName(name);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SendWhenReady(readRequest, reply);

#ifndef NDEBUG
        Cerr << PrintResult<NMsgBusProxy::TBusResponse>(reply.Get()) << Endl;
#endif
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        UNIT_ASSERT(response.HasBlobStorageConfigResponse() && response.GetBlobStorageConfigResponse().GetSuccess());
        UNIT_ASSERT((NMsgBusProxy::EResponseStatus)response.GetStatus());
        UNIT_ASSERT(response.GetBlobStorageConfigResponse().StatusSize() > 0);
        auto status = response.GetBlobStorageConfigResponse().GetStatus(0);
        UNIT_ASSERT(status.StoragePoolSize() > 0);

        auto storagePool = status.GetStoragePool(0);
        UNIT_ASSERT(name == storagePool.GetName());
        UNIT_ASSERT(TServerSettings::BOX_ID == storagePool.GetBoxId());

        return storagePool;
    }

    void TClient::RemoveStoragePool(const TString& name) {
        auto storagePool = DescribeStoragePool(name);

        TAutoPtr<NMsgBusProxy::TBusBlobStorageConfigRequest> deleteRequest(new NMsgBusProxy::TBusBlobStorageConfigRequest());
        deleteRequest->Record.SetDomain(Domain);
        auto deleteParam = deleteRequest->Record.MutableRequest()->AddCommand()->MutableDeleteStoragePool();
        deleteParam->SetBoxId(TServerSettings::BOX_ID);
        deleteParam->SetStoragePoolId(storagePool.GetStoragePoolId());
        deleteParam->SetItemConfigGeneration(storagePool.GetItemConfigGeneration());

        TAutoPtr<NBus::TBusMessage> replyDelete;
        NBus::EMessageStatus msgStatus = SendWhenReady(deleteRequest, replyDelete);

#ifndef NDEBUG
        Cout << PrintResult<NMsgBusProxy::TBusResponse>(replyDelete.Get()) << Endl;
#endif
        UNIT_ASSERT_VALUES_EQUAL(msgStatus, NBus::MESSAGE_OK);
        const NKikimrClient::TResponse &responseDelete = dynamic_cast<NMsgBusProxy::TBusResponse *>(replyDelete.Get())->Record;
        UNIT_ASSERT(responseDelete.HasBlobStorageConfigResponse() && responseDelete.GetBlobStorageConfigResponse().GetSuccess());
        UNIT_ASSERT((NMsgBusProxy::EResponseStatus)responseDelete.GetStatus());
    }

    bool TClient::LocalQuery(const ui64 tabletId, const TString &pgmText, NKikimrMiniKQL::TResult& result) {
        TAutoPtr<NMsgBusProxy::TBusTabletLocalMKQL> request = new NMsgBusProxy::TBusTabletLocalMKQL();
        request->Record.SetTabletID(ChangeStateStorage(tabletId, Domain));
        request->Record.SetWithRetry(true);
        auto *mkql = request->Record.MutableProgram();
        mkql->MutableProgram()->SetText(pgmText);

        TAutoPtr<NBus::TBusMessage> reply;
        auto status = SyncCall(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);

        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        UNIT_ASSERT_VALUES_EQUAL(response.GetStatus(), NMsgBusProxy::MSTATUS_OK);

        if (response.HasExecutionEngineEvaluatedResponse())
            result.CopyFrom(response.GetExecutionEngineEvaluatedResponse());

        return response.GetExecutionEngineResponseStatus() == ui32(NMiniKQL::IEngineFlat::EStatus::Complete);
    }

    bool TClient::LocalSchemeTx(const ui64 tabletId, const NTabletFlatScheme::TSchemeChanges& changes, bool dryRun,
                                NTabletFlatScheme::TSchemeChanges& scheme, TString& err) {
        TAutoPtr<NMsgBusProxy::TBusTabletLocalSchemeTx> request = new NMsgBusProxy::TBusTabletLocalSchemeTx();
        request->Record.SetTabletID(ChangeStateStorage(tabletId, Domain));
        request->Record.SetDryRun(dryRun);
        auto *schemeChanges = request->Record.MutableSchemeChanges();
        schemeChanges->CopyFrom(changes);

        TAutoPtr<NBus::TBusMessage> reply;
        auto status = SyncCall(request, reply);
        UNIT_ASSERT_EQUAL(status, NBus::MESSAGE_OK);

        const NKikimrClient::TResponse &response = dynamic_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        UNIT_ASSERT_EQUAL(response.GetStatus(), NMsgBusProxy::MSTATUS_OK);

        err = response.GetErrorReason();
        scheme.CopyFrom(response.GetLocalDbScheme());

        return err.empty();
    }

    bool TClient::LocalSchemeTx(const ui64 tabletId, const TString &schemeChangesStr, bool dryRun,
                                NTabletFlatScheme::TSchemeChanges& scheme, TString& err) {
        NTabletFlatScheme::TSchemeChanges schemeChanges;
        ::google::protobuf::TextFormat::ParseFromString(schemeChangesStr, &schemeChanges);
        return LocalSchemeTx(tabletId, schemeChanges, dryRun, scheme, err);
    }

    bool TClient::Compile(const TString &mkql, TString &compiled) {
        TAutoPtr<NMsgBusProxy::TBusRequest> request = new NMsgBusProxy::TBusRequest();
        auto* mkqlTx = request->Record.MutableTransaction()->MutableMiniKQLTransaction();
        mkqlTx->MutableProgram()->SetText(mkql);
        mkqlTx->SetFlatMKQL(true);
        mkqlTx->SetMode(NKikimrTxUserProxy::TMiniKQLTransaction::COMPILE);

        TAutoPtr<NBus::TBusMessage> reply;
        NBus::EMessageStatus msgStatus = SyncCall(request, reply);
        UNIT_ASSERT_EQUAL(msgStatus, NBus::MESSAGE_OK);

        const NKikimrClient::TResponse &response = static_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record;
        if (!response.HasMiniKQLCompileResults())
            return false;

        const auto &compileRes = response.GetMiniKQLCompileResults();
        if (compileRes.ProgramCompileErrorsSize()) {
            NYql::TIssues issues;
            NYql::IssuesFromMessage(compileRes.GetProgramCompileErrors(), issues);
            TStringStream err;
            issues.PrintTo(err);
            Cerr << "error: " << err.Str() << Endl;

            return false;
        }

        compiled = compileRes.GetCompiledProgram();
        return true;
    }

    ui32 TClient::FlatQueryRaw(const TString &query, TFlatQueryOptions& opts, NKikimrClient::TResponse& response, int retryCnt) {
        while (retryCnt--) {
            TAutoPtr<NMsgBusProxy::TBusRequest> request = new NMsgBusProxy::TBusRequest();
            {
                auto* mkqlTx = request->Record.MutableTransaction()->MutableMiniKQLTransaction();
                if (opts.IsQueryCompiled)
                    mkqlTx->MutableProgram()->SetBin(query);
                else
                    mkqlTx->MutableProgram()->SetText(query);

                if (opts.Params)
                    mkqlTx->MutableParams()->SetText(opts.Params);
                mkqlTx->SetFlatMKQL(true);
                if (opts.CollectStats)
                    mkqlTx->SetCollectStats(true);
            }

            TAutoPtr<NBus::TBusMessage> reply;
            NBus::EMessageStatus msgStatus = SyncCall(request, reply);
            UNIT_ASSERT_EQUAL(msgStatus, NBus::MESSAGE_OK);

            NMsgBusProxy::TBusResponse * ret = static_cast<NMsgBusProxy::TBusResponse *>(reply.Get());
            ui32 responseStatus = ret->Record.GetStatus();
            if (responseStatus == NMsgBusProxy::MSTATUS_NOTREADY ||
                responseStatus == NMsgBusProxy::MSTATUS_TIMEOUT ||
                responseStatus == NMsgBusProxy::MSTATUS_INPROGRESS)
                continue;

            response.Swap(&static_cast<NMsgBusProxy::TBusResponse *>(reply.Get())->Record);
            break;
        }

        UNIT_ASSERT(retryCnt > 0);
        return response.GetStatus();
    }

    bool TClient::FlatQuery(const TString &query, TFlatQueryOptions& opts, NKikimrMiniKQL::TResult &result, const NKikimrClient::TResponse& expectedResponse) {
        NKikimrClient::TResponse response;
        FlatQueryRaw(query, opts, response);

        if (!response.GetDataShardErrors().empty()) {
            Cerr << "DataShardErrors:" << Endl << response.GetDataShardErrors() << Endl;
        }
        if (!response.GetMiniKQLErrors().empty()) {
            Cerr << "MiniKQLErrors:" << Endl << response.GetMiniKQLErrors() << Endl;
        }
        if (response.HasProxyErrorCode()) {
            if (response.GetProxyErrorCode() != TEvTxUserProxy::TResultStatus::ExecComplete)
                Cerr << "proxy error code: " << static_cast<TEvTxUserProxy::TResultStatus::EStatus>(response.GetProxyErrorCode()) << Endl;
            if (expectedResponse.HasProxyErrorCode()) {
                UNIT_ASSERT_VALUES_EQUAL(response.GetProxyErrorCode(), expectedResponse.GetProxyErrorCode());
            }
        }
        if (response.HasProxyErrors()) {
            Cerr << "proxy errors: " << response.GetProxyErrors() << Endl;
        }
        if (response.UnresolvedKeysSize() > 0) {
            for (size_t i = 0, end = response.UnresolvedKeysSize(); i < end; ++i) {
                Cerr << response.GetUnresolvedKeys(i) << Endl;
            }
        }
        if (response.HasMiniKQLCompileResults()) {
            const auto &compileRes = response.GetMiniKQLCompileResults();
            if (compileRes.ProgramCompileErrorsSize()) {
                NYql::TIssues issues;
                NYql::IssuesFromMessage(compileRes.GetProgramCompileErrors(), issues);
                TStringStream err;
                issues.PrintTo(err);
                Cerr << "error: " << err.Str() << Endl;
            }
            if (compileRes.ParamsCompileErrorsSize()) {
                NYql::TIssues issues;
                NYql::IssuesFromMessage(compileRes.GetParamsCompileErrors(), issues);
                TStringStream err;
                issues.PrintTo(err);
                Cerr << "error: " << err.Str() << Endl;
            }
        }
        if (response.HasHadFollowerReads() && response.GetHadFollowerReads()) {
            Cerr << "had follower reads" << Endl;
        }

        if (expectedResponse.HasStatus()) {
            UNIT_ASSERT_VALUES_EQUAL(response.GetStatus(), expectedResponse.GetStatus());
        }
        if (expectedResponse.GetStatus() != NMsgBusProxy::MSTATUS_OK)
            return false;

        UNIT_ASSERT(response.HasTxId());
        UNIT_ASSERT(response.GetExecutionEngineResponseStatus() == ui32(NMiniKQL::IEngineFlat::EStatus::Complete)
            || response.GetExecutionEngineResponseStatus() == ui32(NMiniKQL::IEngineFlat::EStatus::Aborted));

        if (response.HasExecutionEngineEvaluatedResponse()) {
            result.Swap(response.MutableExecutionEngineEvaluatedResponse());
        }

        return response.GetExecutionEngineResponseStatus() == ui32(NMiniKQL::IEngineFlat::EStatus::Complete);
    }

    bool TClient::FlatQuery(const TString &query, TFlatQueryOptions& opts, NKikimrMiniKQL::TResult &result, ui32 expectedStatus) {
        NKikimrClient::TResponse expectedResponse;
        expectedResponse.SetStatus(expectedStatus);
        return FlatQuery(query, opts, result, expectedResponse);
    }

    bool TClient::FlatQueryParams(const TString& mkql, const TString& params, bool queryCompiled, NKikimrMiniKQL::TResult &result) {
        TFlatQueryOptions opts;
        opts.Params = params;
        opts.IsQueryCompiled = queryCompiled;
        return FlatQuery(mkql, opts, result);
    }

    bool TClient::FlatQuery(const TString& mkql, NKikimrMiniKQL::TResult& result) {
        TFlatQueryOptions opts;
        return FlatQuery(mkql, opts, result);
    }

    TString TClient::SendTabletMonQuery(TTestActorRuntime* runtime, ui64 tabletId, TString query) {
        TActorId sender = runtime->AllocateEdgeActor(0);
        ForwardToTablet(*runtime, tabletId, sender, new NActors::NMon::TEvRemoteHttpInfo(query), 0);
        TAutoPtr<IEventHandle> handle;
        // Timeout for DEBUG purposes only
        runtime->GrabEdgeEvent<NMon::TEvRemoteJsonInfoRes>(handle);
        TString res = handle->Get<NMon::TEvRemoteJsonInfoRes>()->Json;
#ifndef NDEBUG
        Cerr << res << Endl;
#endif
        return res;
    }

    TString TClient::MarkNodeInHive(TTestActorRuntime* runtime, ui32 nodeIdx, bool up) {
        ui32 nodeId = runtime->GetNodeId(nodeIdx);
        ui64 hive = ChangeStateStorage(Tests::Hive, Domain);
        TInstant deadline = TInstant::Now() + TIMEOUT;
        while (TInstant::Now() <= deadline) {
            TString res = SendTabletMonQuery(runtime, hive, TString("/app?page=SetDown&node=") + ToString(nodeId) + "&down=" + (up ? "0" : "1"));
            if (!res.empty() && !res.Contains("Error"))
                return res;

        }
        UNIT_ASSERT_C(false, "Failed to mark node in hive");
        return TString();
    }

    TString TClient::KickNodeInHive(TTestActorRuntime* runtime, ui32 nodeIdx) {
        ui32 nodeId = runtime->GetNodeId(nodeIdx);
        ui64 hive = ChangeStateStorage(Tests::Hive, Domain);
        return SendTabletMonQuery(runtime, hive, TString("/app?page=KickNode&node=") + ToString(nodeId));
    }

    bool TClient::WaitForTabletAlive(TTestActorRuntime* runtime, ui64 tabletId, bool leader, TDuration timeout) {
        TActorId edge = runtime->AllocateEdgeActor();
        NTabletPipe::TClientConfig clientConfig;
        clientConfig.AllowFollower = !leader;
        clientConfig.ForceFollower = !leader;
        clientConfig.RetryPolicy = NTabletPipe::TClientRetryPolicy::WithRetries();
        TActorId pipeClient = runtime->Register(NTabletPipe::CreateClient(edge, tabletId, clientConfig));
        TAutoPtr<IEventHandle> handle;
        const TInstant deadline = TInstant::Now() + timeout;
        bool res = false;

        try {
            while (TInstant::Now() <= deadline) {
                TEvTabletPipe::TEvClientConnected* ev = runtime->GrabEdgeEvent<TEvTabletPipe::TEvClientConnected>(handle, deadline - TInstant::Now());
                if (!ev) {
                    break;
                }
                if (ev->ClientId == pipeClient && ev->TabletId == tabletId) {
                    res = (ev->Status == NKikimrProto::OK);
                    break;
                }
            }
        } catch (TEmptyEventQueueException &) {}

        runtime->Send(new IEventHandle(pipeClient, TActorId(), new TEvents::TEvPoisonPill()));
        return res;
    }

    bool TClient::WaitForTabletDown(TTestActorRuntime* runtime, ui64 tabletId, bool leader, TDuration timeout) {
        TActorId edge = runtime->AllocateEdgeActor();
        NTabletPipe::TClientConfig clientConfig;
        clientConfig.AllowFollower = !leader;
        clientConfig.ForceFollower = !leader;
        clientConfig.RetryPolicy = {
            .RetryLimitCount = 5,
            .MinRetryTime = TDuration::MilliSeconds(500),
            .MaxRetryTime = TDuration::Seconds(1),
            .BackoffMultiplier = 2,
        };
        TActorId pipeClient = runtime->Register(NTabletPipe::CreateClient(edge, tabletId, clientConfig));
        TInstant deadline = TInstant::Now() + timeout;

        bool res = false;

        try {
            while (TInstant::Now() <= deadline) {
                TAutoPtr<IEventHandle> handle;
                auto result = runtime->GrabEdgeEvents<TEvTabletPipe::TEvClientConnected, TEvTabletPipe::TEvClientDestroyed>(handle, deadline - TInstant::Now());
                if (handle && handle->Recipient == edge && handle->Sender == pipeClient) {
                    if (std::get<TEvTabletPipe::TEvClientDestroyed*>(result) != nullptr)
                    {
                        TEvTabletPipe::TEvClientDestroyed* event = std::get<TEvTabletPipe::TEvClientDestroyed*>(result);
                        if (event->TabletId == tabletId) {
                            res = true;
                            break;
                        }
                    }
                    if (std::get<TEvTabletPipe::TEvClientConnected*>(result) != nullptr)
                    {
                        TEvTabletPipe::TEvClientConnected* event = std::get<TEvTabletPipe::TEvClientConnected*>(result);
                        if (event->TabletId == tabletId && event->Status != NKikimrProto::OK) {
                            res = true;
                            break;
                        }
                    }
                }
            }
        } catch (TEmptyEventQueueException &) {}

        runtime->Send(new IEventHandle(pipeClient, edge, new TEvents::TEvPoisonPill()));
        return res;
    }

    void TClient::GetTabletInfoFromHive(TTestActorRuntime* runtime, ui64 tabletId, bool returnFollowers, NKikimrHive::TEvResponseHiveInfo& res) {
        TAutoPtr<TEvHive::TEvRequestHiveInfo> ev(new TEvHive::TEvRequestHiveInfo);
        ev->Record.SetTabletID(tabletId);
        ev->Record.SetReturnFollowers(returnFollowers);

        ui64 hive = ChangeStateStorage(Tests::Hive, Domain);
        TActorId edge = runtime->AllocateEdgeActor();
        runtime->SendToPipe(hive, edge, ev.Release());
        TAutoPtr<IEventHandle> handle;
        TEvHive::TEvResponseHiveInfo* response = runtime->GrabEdgeEventRethrow<TEvHive::TEvResponseHiveInfo>(handle);
        res.Swap(&response->Record);
    }

    ui32 TClient::GetLeaderNode(TTestActorRuntime* runtime, ui64 tabletId) {
        NKikimrHive::TEvResponseHiveInfo res;
        GetTabletInfoFromHive(runtime, tabletId, false, res);
        // Cerr << res << Endl;

        for (const NKikimrHive::TTabletInfo& tablet : res.GetTablets()) {
            if (tablet.GetTabletID() == tabletId && tablet.GetNodeID() != 0) {
                return NodeIdToIndex(runtime, tablet.GetNodeID());
            }
        }

        return Max<ui32>();
    }

    bool TClient::TabletExistsInHive(TTestActorRuntime* runtime, ui64 tabletId, bool evenInDeleting) {
        NKikimrHive::TEvResponseHiveInfo res;
        GetTabletInfoFromHive(runtime, tabletId, false, res);
        // Cerr << res << Endl;

        for (const NKikimrHive::TTabletInfo& tablet : res.GetTablets()) {
            if (tablet.GetTabletID() == tabletId) {
                return evenInDeleting || tablet.GetState() != (ui32)NHive::ETabletState::Deleting;
            }
        }

        return false;
    }

    void TClient::GetTabletStorageInfoFromHive(TTestActorRuntime* runtime, ui64 tabletId, NKikimrHive::TEvGetTabletStorageInfoResult& res) {
        TAutoPtr<TEvHive::TEvGetTabletStorageInfo> ev(new TEvHive::TEvGetTabletStorageInfo(tabletId));

        ui64 hive = ChangeStateStorage(Tests::Hive, Domain);
        TActorId edge = runtime->AllocateEdgeActor();
        runtime->SendToPipe(hive, edge, ev.Release());
        TAutoPtr<IEventHandle> handle;
        TEvHive::TEvGetTabletStorageInfoResult* response = runtime->GrabEdgeEventRethrow<TEvHive::TEvGetTabletStorageInfoResult>(handle);

        res.Swap(&response->Record);
#ifndef NDEBUG
        Cerr << response->Record.DebugString() << "\n";
#endif

        if (res.GetStatus() == NKikimrProto::OK) {
            auto& info = res.GetInfo();
            Y_VERIFY(res.GetTabletID() == info.GetTabletID());
            Y_VERIFY(info.ChannelsSize() > 0);

            auto& channel = info.GetChannels(0);
            Y_VERIFY(channel.GetChannel() == 0);
            Y_VERIFY(channel.HistorySize() > 0);
        }
    }

    TVector<ui32> TClient::GetFollowerNodes(TTestActorRuntime* runtime, ui64 tabletId) {
        NKikimrHive::TEvResponseHiveInfo res;
        GetTabletInfoFromHive(runtime, tabletId, true, res);
        // Cerr << res << Endl;

        TVector<ui32> followerNodes;
        for (const NKikimrHive::TTabletInfo& tablet : res.GetTablets()) {
            if (tablet.GetTabletID() == tabletId && tablet.HasFollowerID()) {
                followerNodes.push_back(NodeIdToIndex(runtime, tablet.GetNodeID()));
            }
        }

        return followerNodes;
    }

    void TClient::S3Listing(const TString& table, const TString& prefixColumnsPb,
                            const TString& pathPrefix, const TString& pathDelimiter,
                            const TString& startAfterSuffixColumnsPb,
                            const TVector<TString>& columnsToReturn, ui32 maxKeys,
                            ui32 timeoutMillisec,
                            NKikimrClient::TS3ListingResponse& res) {
        TAutoPtr<NMsgBusProxy::TBusS3ListingRequest> request = new NMsgBusProxy::TBusS3ListingRequest();
        request->Record.SetTableName(table);
        bool parseOk = ::google::protobuf::TextFormat::ParseFromString(prefixColumnsPb, request->Record.MutableKeyPrefix());
        UNIT_ASSERT_C(parseOk, "Failed to parse prefix columns: " + prefixColumnsPb);
        request->Record.SetPathColumnPrefix(pathPrefix);
        request->Record.SetPathColumnDelimiter(pathDelimiter);
        if (!startAfterSuffixColumnsPb.empty()) {
            parseOk = ::google::protobuf::TextFormat::ParseFromString(startAfterSuffixColumnsPb, request->Record.MutableStartAfterKeySuffix());
            UNIT_ASSERT_C(parseOk, "Failed to parse suffix columns: " + startAfterSuffixColumnsPb);
        }
        request->Record.SetMaxKeys(maxKeys);
        request->Record.SetTimeout(timeoutMillisec);
        for (const TString& c : columnsToReturn) {
            request->Record.AddColumnsToReturn(c);
        }

        TAutoPtr<NBus::TBusMessage> reply;
        auto status = SyncCall(request, reply);
        UNIT_ASSERT_VALUES_EQUAL(status, NBus::MESSAGE_OK);

        NKikimrClient::TS3ListingResponse& response = dynamic_cast<NMsgBusProxy::TBusS3ListingResponse*>(reply.Get())->Record;

        res.Swap(&response);
    }

    ui64 TClient::GetKesusTabletId(const TString& kesusPath) {
        auto describeResult = Ls(kesusPath);
        UNIT_ASSERT_C(describeResult->Record.GetPathDescription().HasKesus(), describeResult->Record);
        return describeResult->Record.GetPathDescription().GetKesus().GetKesusTabletId();
    }

    Ydb::StatusIds::StatusCode TClient::AddQuoterResource(TTestActorRuntime* runtime, const TString& kesusPath, const TString& resourcePath, const TMaybe<double> maxUnitsPerSecond) {
        NKikimrKesus::THierarchicalDRRResourceConfig cfg;
        if (maxUnitsPerSecond) {
            cfg.SetMaxUnitsPerSecond(*maxUnitsPerSecond);
        }
        return AddQuoterResource(runtime, kesusPath, resourcePath, cfg);
    }

    Ydb::StatusIds::StatusCode TClient::AddQuoterResource(TTestActorRuntime* runtime, const TString& kesusPath, const TString& resourcePath, const NKikimrKesus::THierarchicalDRRResourceConfig& props) {
        THolder<NKesus::TEvKesus::TEvAddQuoterResource> request = MakeHolder<NKesus::TEvKesus::TEvAddQuoterResource>();
        request->Record.MutableResource()->SetResourcePath(resourcePath);
        *request->Record.MutableResource()->MutableHierarhicalDRRResourceConfig() = props;

        TActorId sender = runtime->AllocateEdgeActor(0);
        ForwardToTablet(*runtime, GetKesusTabletId(kesusPath), sender, request.Release(), 0);

        TAutoPtr<IEventHandle> handle;
        runtime->GrabEdgeEvent<NKesus::TEvKesus::TEvAddQuoterResourceResult>(handle);
        auto& record = handle->Get<NKesus::TEvKesus::TEvAddQuoterResourceResult>()->Record;
        return record.GetError().GetStatus();
    }

    THolder<NKesus::TEvKesus::TEvGetConfigResult> TClient::GetKesusConfig(TTestActorRuntime* runtime, const TString& kesusPath) {
        THolder<NKesus::TEvKesus::TEvGetConfig> request = MakeHolder<NKesus::TEvKesus::TEvGetConfig>();

        TActorId sender = runtime->AllocateEdgeActor(0);
        ForwardToTablet(*runtime, GetKesusTabletId(kesusPath), sender, request.Release(), 0);

        TAutoPtr<IEventHandle> handle;
        runtime->GrabEdgeEvent<NKesus::TEvKesus::TEvGetConfigResult>(handle);
        return handle->Release<NKesus::TEvKesus::TEvGetConfigResult>();
    }

    bool IsServerRedirected() {
        return !!GetEnv(ServerRedirectEnvVar);
    }

    TServerSetup GetServerSetup() {
        if (!IsServerRedirected()) {
            return TServerSetup("localhost", 0);
        }

        TStringBuf str(GetEnv(ServerRedirectEnvVar));
        TStringBuf address;
        TStringBuf port;
        str.Split('/', address, port);
        ui64 portValue = 0;
        if (address.empty() || !TryFromString(port, portValue))
            ythrow TWithBackTrace<yexception>() << "Incorrect server redirect, expected 'IpAddress/Port'";

        return TServerSetup(TString(address), portValue);
    }

    TTenants::TTenants(TServer::TPtr server)
        : Server(server)
    {
        ui32 dynamicNodeStartsAt = Server->StaticNodes();
        ui32 dynamicNodeEndsAt = dynamicNodeStartsAt + Server->DynamicNodes();
        for (ui32 nodeIdx = dynamicNodeStartsAt; nodeIdx < dynamicNodeEndsAt; ++nodeIdx) {
            VacantNodes.push_back(nodeIdx);
        }
        MakeHeap(VacantNodes.begin(), VacantNodes.end());
    }

    TTenants::~TTenants() {
        Stop();
    }

    void TTenants::Run(const TString &name, ui32 nodes) {
        Y_VERIFY(!Tenants.contains(name));
        Y_VERIFY(Availabe() >= nodes);

        Tenants[name] = {};
        RunNodes(name, nodes);
    }

    void TTenants::Stop(const TString &name) {
        Y_VERIFY(Tenants.contains(name));

        Free(name, Size(name));
        Tenants.erase(name);
    }

    void TTenants::Stop() {
        for (auto &it: Tenants) {
            const TString &name = it.first;
            Free(name, Size(name));
        }
        Tenants.clear();
    }

    void TTenants::Add(const TString &name, ui32 nodes) {
        Y_VERIFY(Tenants.contains(name));
        Y_VERIFY(Availabe() >= nodes);

        return RunNodes(name, nodes);
    }

    void TTenants::Free(const TString &name, ui32 nodes) {
        Y_VERIFY(Tenants.contains(name));
        Y_VERIFY(Size(name) >= nodes);

        return StopNodes(name, nodes);
    }

    void TTenants::FreeNode(const TString &name, ui32 nodeIdx) {
        Y_VERIFY(Tenants.contains(name));
        Y_VERIFY(Size(name) >= 1);

        return StopPaticularNode(name, nodeIdx);
    }

    bool TTenants::IsStaticNode(ui32 nodeIdx) const {
        return nodeIdx < Server->StaticNodes();
    }

    bool TTenants::IsActive(const TString &name, ui32 nodeIdx) const {
        const TVector<ui32>& nodes = List(name);
#ifndef NDEBUG
        Cerr << "IsActive: " << name << " -- " << nodeIdx << Endl;
        for (auto& x: nodes) {
            Cerr << " -- " << x;
        }
        Cerr << Endl;
#endif
        return std::find(nodes.begin(), nodes.end(), nodeIdx) != nodes.end();
    }

    const TVector<ui32> &TTenants::List(const TString &name) const {
        Y_VERIFY(Tenants.contains(name));

        return Tenants.at(name);
    }

    ui32 TTenants::Size(const TString &name) const {
        if (!Tenants.contains(name))
            return 0;
        return List(name).size();
    }

    ui32 TTenants::Size() const {
        return Capacity() - Availabe();
    }

    ui32 TTenants::Availabe() const {
        return VacantNodes.size();
    }

    ui32 TTenants::Capacity() const {
        return Server->DynamicNodes();
    }

    TVector<ui32> &TTenants::Nodes(const TString &name) {
        return Tenants[name];
    }

    void TTenants::StopNode(const TString, ui32 nodeIdx) {
        Server->DestroyDynamicLocalService(nodeIdx);
    }

    void TTenants::RunNode(const TString &name, ui32 nodeIdx) {
        Server->SetupDynamicLocalService(nodeIdx, name);
    }

    void TTenants::StopPaticularNode(const TString &name, ui32 nodeIdx) {
        TVector<ui32>& nodes = Nodes(name);

        auto subj = std::find(nodes.begin(), nodes.end(), nodeIdx);
        Y_VERIFY(subj != nodes.end());

        StopNode(name, nodeIdx);

        std::swap(*subj, nodes.back());
        nodes.pop_back();
        FreeNodeIdx(nodeIdx);
    }

    void TTenants::StopNodes(const TString &name, ui32 count) {
        TVector<ui32>& nodes = Nodes(name);

        for (ui32 num = 0; num < count && nodes; ++num) {
            ui32 nodeIdx = nodes.back();
            StopNode(name, nodeIdx);
            nodes.pop_back();
            FreeNodeIdx(nodeIdx);
        }
    }

    void TTenants::RunNodes(const TString &name, ui32 count) {
        TVector<ui32>& nodes = Nodes(name);

        for (ui32 num = 0; num < count; ++num) {
            ui32 nodeIdx = AllocNodeIdx();
            RunNode(name, nodeIdx);
            nodes.push_back(nodeIdx);
        }
    }

    ui32 TTenants::AllocNodeIdx() {
        Y_VERIFY(VacantNodes);
        PopHeap(VacantNodes.begin(), VacantNodes.end());
        ui32 node = VacantNodes.back();
        VacantNodes.pop_back();
        return node;
    }

    void TTenants::FreeNodeIdx(ui32 nodeIdx) {
        VacantNodes.push_back(nodeIdx);
        PushHeap(VacantNodes.begin(), VacantNodes.end());
    }

    TServerSettings& TServerSettings::AddStoragePool(const TString& poolKind, const TString& poolName, ui32 numGroups, ui32 encryptionMode) {
        NKikimrBlobStorage::TDefineStoragePool& hddPool = StoragePoolTypes[poolKind];
        hddPool.SetBoxId(BOX_ID);
        hddPool.SetStoragePoolId(POOL_ID++);
        hddPool.SetErasureSpecies("none");
        hddPool.SetVDiskKind("Default");
        hddPool.AddPDiskFilter()->AddProperty()->SetType(NKikimrBlobStorage::ROT);
        hddPool.SetKind(poolKind);
        if (poolName) {
            hddPool.SetName(poolName);
        } else {
            hddPool.SetName(poolKind);
        }
        if (encryptionMode) {
            hddPool.SetEncryptionMode(encryptionMode);
        }
        hddPool.SetNumGroups(numGroups);
        return *this;
    }

    TServerSettings& TServerSettings::AddStoragePoolType(const TString& poolKind, ui32 encryptionMode) {
        NKikimrBlobStorage::TDefineStoragePool hddPool;
        hddPool.SetBoxId(BOX_ID);
        hddPool.SetStoragePoolId(POOL_ID++);
        hddPool.SetErasureSpecies("none");
        hddPool.SetVDiskKind("Default");
        hddPool.AddPDiskFilter()->AddProperty()->SetType(NKikimrBlobStorage::ROT);
        hddPool.SetKind(poolKind);
        if (encryptionMode) {
            hddPool.SetEncryptionMode(encryptionMode);
        }
        StoragePoolTypes[poolKind] = hddPool;
        return *this;
    }


}
}
