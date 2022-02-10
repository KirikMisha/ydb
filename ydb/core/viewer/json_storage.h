#pragma once
#include <unordered_map>
#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/interconnect.h>
#include <library/cpp/actors/core/mon.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>
#include <ydb/core/viewer/json/json.h>
#include <ydb/core/protos/node_whiteboard.pb.h>
#include <ydb/core/viewer/protos/viewer.pb.h>
#include "viewer.h"
#include "json_pipe_req.h"
#include "json_vdiskinfo.h"
#include "json_pdiskinfo.h"

namespace NKikimr {
namespace NViewer {

using namespace NActors;
using namespace NNodeWhiteboard;

using ::google::protobuf::FieldDescriptor;

class TJsonStorage : public TViewerPipeClient<TJsonStorage> {
    using TThis = TJsonStorage;
    using TBase = TViewerPipeClient<TJsonStorage>;
    using TNodeId = ui32;
    IViewer* Viewer;
    TActorId Initiator;
    NMon::TEvHttpInfo::TPtr Event;
    THolder<TEvInterconnect::TEvNodesInfo> NodesInfo;
    TMap<ui32, THolder<TEvWhiteboard::TEvVDiskStateResponse>> VDiskInfo;
    TMap<ui32, THolder<TEvWhiteboard::TEvPDiskStateResponse>> PDiskInfo;
    TMap<ui32, THolder<TEvWhiteboard::TEvBSGroupStateResponse>> BSGroupInfo;
    THashMap<TString, THolder<NSchemeShard::TEvSchemeShard::TEvDescribeSchemeResult>> DescribeResult;
    THashMap<TTabletId, THolder<TEvHive::TEvResponseHiveStorageStats>> HiveStorageStats;
    THolder<TEvBlobStorage::TEvControllerConfigResponse> BaseConfig;

    struct TStoragePoolInfo {
        TString Kind;
        TSet<TString> Groups;
        NKikimrViewer::EFlag Overall = NKikimrViewer::EFlag::Grey;
    };

    THashMap<TString, TStoragePoolInfo> StoragePoolInfo;
    TJsonSettings JsonSettings;
    ui32 Timeout = 0;
    TString FilterTenant;
    THashSet<TString> FilterStoragePools;
    TVector<TString> FilterGroupIds;
    TVector<ui32> FilterNodeIds;
    std::unordered_set<TNodeId> NodeIds;
    bool NeedGroups = true;
    bool NeedDisks = true;

    enum class EWith {
        Everything,
        MissingDisks,
        SpaceProblems,
    };

    EWith With = EWith::Everything;

public:
    static constexpr NKikimrServices::TActivity::EType ActorActivityType() {
        return NKikimrServices::TActivity::VIEWER_HANDLER;
    }

    TJsonStorage(IViewer* viewer, NMon::TEvHttpInfo::TPtr& ev)
        : Viewer(viewer)
        , Initiator(ev->Sender)
        , Event(std::move(ev))
    {
        const auto& params(Event->Get()->Request.GetParams());
        JsonSettings.EnumAsNumbers = !FromStringWithDefault<bool>(params.Get("enums"), true);
        JsonSettings.UI64AsString = !FromStringWithDefault<bool>(params.Get("ui64"), false);
        InitConfig(params);
        Timeout = FromStringWithDefault<ui32>(params.Get("timeout"), 10000);
        FilterTenant = params.Get("tenant");
        TString filterStoragePool = params.Get("pool");
        if (!filterStoragePool.empty()) {
            FilterStoragePools.emplace(filterStoragePool);
        }
        SplitIds(params.Get("node_id"), ',', FilterNodeIds);
        SplitIds(params.Get("group_id"), ',', FilterGroupIds);
        Sort(FilterGroupIds);
        NeedGroups = FromStringWithDefault<bool>(params.Get("need_groups"), true);
        NeedDisks = FromStringWithDefault<bool>(params.Get("need_disks"), NeedGroups);
        NeedGroups = Max(NeedGroups, NeedDisks);
        if (params.Get("with") == "missing") {
            With = EWith::MissingDisks;
        } if (params.Get("with") == "space") {
            With = EWith::SpaceProblems;
        }
    }

    void Bootstrap() {
        TIntrusivePtr<TDomainsInfo> domains = AppData()->DomainsInfo;

        if (FilterTenant.empty()) {
            RequestConsoleListTenants();
        } else {
            RequestSchemeCacheNavigate(FilterTenant);
        }
        std::replace(FilterNodeIds.begin(), FilterNodeIds.end(), (ui32)0, TlsActivationContext->ExecutorThread.ActorSystem->NodeId);
        if (FilterNodeIds.empty()) {
            SendRequest(GetNameserviceActorId(), new TEvInterconnect::TEvListNodes());
        } else {
            for (ui32 nodeId : FilterNodeIds) {
                SendNodeRequests(nodeId);
            }
            if (Requests == 0) {
                ReplyAndPassAway();
                return;
            }
        }

        TIntrusivePtr<TDomainsInfo::TDomain> domain = domains->Domains.begin()->second;
        ui64 hiveId = domains->GetHive(domain->DefaultHiveUid);
        if (hiveId != 0) {
            RequestHiveStorageStats(hiveId);
        }

        RequestBSControllerConfig();

        TBase::Become(&TThis::StateWork, TDuration::MilliSeconds(Timeout), new TEvents::TEvWakeup());
    }

    void PassAway() override {
        for (const TNodeId nodeId : NodeIds) {
            Send(TActivationContext::InterconnectProxy(nodeId), new TEvents::TEvUnsubscribe());
        }
        TBase::PassAway();
    }

    void SendNodeRequests(ui32 nodeId) {
        if (NodeIds.insert(nodeId).second) {
            TActorId whiteboardServiceId = MakeNodeWhiteboardServiceId(nodeId);
            SendRequest(whiteboardServiceId, new TEvWhiteboard::TEvVDiskStateRequest(), IEventHandle::FlagTrackDelivery | IEventHandle::FlagSubscribeOnSession, nodeId);
            SendRequest(whiteboardServiceId, new TEvWhiteboard::TEvPDiskStateRequest(), IEventHandle::FlagTrackDelivery | IEventHandle::FlagSubscribeOnSession, nodeId);
            SendRequest(whiteboardServiceId, new TEvWhiteboard::TEvBSGroupStateRequest(), IEventHandle::FlagTrackDelivery | IEventHandle::FlagSubscribeOnSession, nodeId);
        }
    }

    void Handle(TEvBlobStorage::TEvControllerSelectGroupsResult::TPtr& ev) {
        for (const auto& matchingGroups : ev->Get()->Record.GetMatchingGroups()) {
            for (const auto& group : matchingGroups.GetGroups()) {
                TString storagePoolName = group.GetStoragePoolName();
                StoragePoolInfo[storagePoolName].Groups.emplace(ToString(group.GetGroupID()));
            }
        }
        RequestDone();
    }

    void Handle(TEvBlobStorage::TEvControllerConfigResponse::TPtr& ev) {
        const NKikimrBlobStorage::TEvControllerConfigResponse& pbRecord(ev->Get()->Record);

        if (pbRecord.HasResponse() && pbRecord.GetResponse().StatusSize() > 0) {
            const NKikimrBlobStorage::TConfigResponse::TStatus& pbStatus(pbRecord.GetResponse().GetStatus(0));
            if (pbStatus.HasBaseConfig()) {
                BaseConfig = ev->Release();
            }
        }
        RequestDone();
    }

    void Handle(NConsole::TEvConsole::TEvListTenantsResponse::TPtr& ev) {
        Ydb::Cms::ListDatabasesResult listTenantsResult;
        ev->Get()->Record.GetResponse().operation().result().UnpackTo(&listTenantsResult);
        for (const TString& path : listTenantsResult.paths()) {
            RequestSchemeCacheNavigate(path);
        }
        RequestDone();
    }

    void Handle(TEvInterconnect::TEvNodesInfo::TPtr& ev) {
        ui32 maxAllowedNodeId = std::numeric_limits<ui32>::max();
        TIntrusivePtr<TDynamicNameserviceConfig> dynamicNameserviceConfig = AppData()->DynamicNameserviceConfig;
        if (dynamicNameserviceConfig) {
            maxAllowedNodeId = dynamicNameserviceConfig->MaxStaticNodeId;
        }
        NodesInfo = ev->Release();
        for (const auto& ni : NodesInfo->Nodes) {
            if (ni.NodeId <= maxAllowedNodeId) {
                SendNodeRequests(ni.NodeId);
            }
        }
        RequestDone();
    }

    void Handle(TEvTxProxySchemeCache::TEvNavigateKeySetResult::TPtr& ev) {
        if (ev->Get()->Request->ResultSet.size() == 1 && ev->Get()->Request->ResultSet.begin()->Status == NSchemeCache::TSchemeCacheNavigate::EStatus::Ok) {
            TString path = CanonizePath(ev->Get()->Request->ResultSet.begin()->Path);
            TIntrusiveConstPtr<TSchemeCacheNavigate::TDomainDescription> domainDescription = ev->Get()->Request->ResultSet.begin()->DomainDescription;
            TIntrusiveConstPtr<NSchemeCache::TDomainInfo> domainInfo = ev->Get()->Request->ResultSet.begin()->DomainInfo;

            if (domainInfo != nullptr && domainDescription != nullptr) {
                TTabletId hiveId = domainInfo->Params.GetHive();
                if (hiveId != 0) {
                    RequestHiveStorageStats(hiveId);
                }

                for (const auto& storagePool : domainDescription->Description.GetStoragePools()) {
                    TString storagePoolName = storagePool.GetName();
                    if (!FilterTenant.empty()) {
                        FilterStoragePools.emplace(storagePoolName);
                    }
                    StoragePoolInfo[storagePoolName].Kind = storagePool.GetKind();
                    THolder<TEvBlobStorage::TEvControllerSelectGroups> request = MakeHolder<TEvBlobStorage::TEvControllerSelectGroups>();
                    request->Record.SetReturnAllMatchingGroups(true);
                    request->Record.AddGroupParameters()->MutableStoragePoolSpecifier()->SetName(storagePoolName);
                    RequestBSControllerSelectGroups(std::move(request));
                }
            }
        }
        RequestDone();
    }

    void Handle(TEvHive::TEvResponseHiveStorageStats::TPtr& ev) {
        HiveStorageStats[ev->Cookie] = ev->Release();
        RequestDone();
    }

    void Undelivered(TEvents::TEvUndelivered::TPtr& ev) {
        ui32 nodeId = ev.Get()->Cookie;
        switch (ev->Get()->SourceType) {
        case TEvWhiteboard::EvVDiskStateRequest:
            if (VDiskInfo.emplace(nodeId, nullptr).second) {
                RequestDone();
            }
            break;
        case TEvWhiteboard::EvPDiskStateRequest:
            if (PDiskInfo.emplace(nodeId, nullptr).second) {
                RequestDone();
            }
            break;
        case TEvWhiteboard::EvBSGroupStateRequest:
            if (BSGroupInfo.emplace(nodeId, nullptr).second) {
                RequestDone();
            }
            break;
        }
    }

    void Disconnected(TEvInterconnect::TEvNodeDisconnected::TPtr& ev) {
        ui32 nodeId = ev->Get()->NodeId;
        if (VDiskInfo.emplace(nodeId, nullptr).second) {
            RequestDone();
        }
        if (PDiskInfo.emplace(nodeId, nullptr).second) {
            RequestDone();
        }
        if (BSGroupInfo.emplace(nodeId, nullptr).second) {
            RequestDone();
        }
    }

    void Handle(TEvWhiteboard::TEvVDiskStateResponse::TPtr& ev) {
        ui64 nodeId = ev.Get()->Cookie;
        VDiskInfo[nodeId] = ev->Release();
        RequestDone();
    }

    void Handle(TEvWhiteboard::TEvPDiskStateResponse::TPtr& ev) {
        ui64 nodeId = ev.Get()->Cookie;
        PDiskInfo[nodeId] = ev->Release();
        RequestDone();
    }

    void Handle(TEvWhiteboard::TEvBSGroupStateResponse::TPtr& ev) {
        for (const auto& info : ev->Get()->Record.GetBSGroupStateInfo()) {
            TString storagePoolName = info.GetStoragePoolName();
            if (storagePoolName.empty()) {
                continue;
            }
            StoragePoolInfo[storagePoolName].Groups.emplace(ToString(info.GetGroupID()));
        }
        ui64 nodeId = ev.Get()->Cookie;
        BSGroupInfo[nodeId] = ev->Release();
        RequestDone();
    }

    STATEFN(StateWork) {
        switch (ev->GetTypeRewrite()) {
            hFunc(TEvInterconnect::TEvNodesInfo, Handle);
            hFunc(TEvWhiteboard::TEvVDiskStateResponse, Handle);
            hFunc(TEvWhiteboard::TEvPDiskStateResponse, Handle);
            hFunc(TEvWhiteboard::TEvBSGroupStateResponse, Handle);
            hFunc(NConsole::TEvConsole::TEvListTenantsResponse, Handle);
            hFunc(TEvTxProxySchemeCache::TEvNavigateKeySetResult, Handle);
            hFunc(TEvBlobStorage::TEvControllerSelectGroupsResult, Handle);
            hFunc(TEvBlobStorage::TEvControllerConfigResponse, Handle);
            hFunc(TEvHive::TEvResponseHiveStorageStats, Handle);
            hFunc(TEvents::TEvUndelivered, Undelivered);
            hFunc(TEvInterconnect::TEvNodeDisconnected, Disconnected);
            hFunc(TEvTabletPipe::TEvClientConnected, TBase::Handle);
            cFunc(TEvents::TSystem::Wakeup, HandleTimeout);
        }
    }

    NKikimrViewer::TStorageInfo StorageInfo;
    THolder<TEvWhiteboard::TEvBSGroupStateResponse> MergedBSGroupInfo;
    THolder<TEvWhiteboard::TEvVDiskStateResponse> MergedVDiskInfo;
    THolder<TEvWhiteboard::TEvPDiskStateResponse> MergedPDiskInfo;
    TMap<TString, const NKikimrWhiteboard::TBSGroupStateInfo&> BSGroupIndex;
    TMap<TString, NKikimrHive::THiveStorageGroupStats> BSGroupHiveIndex;
    TMap<NKikimrBlobStorage::TVDiskID, const NKikimrWhiteboard::TVDiskStateInfo&> VDisksIndex;
    TMap<std::pair<ui32, ui32>, const NKikimrWhiteboard::TPDiskStateInfo&> PDisksIndex;
    TMap<TString, TString> BSGroupOverall;
    THashSet<TString> BSGroupWithMissingDisks;
    THashSet<TString> BSGroupWithSpaceProblems;
    TMap<NKikimrBlobStorage::TVDiskID, TString> VDisksOverall;
    TMap<std::pair<ui32, ui32>, TString> PDisksOverall;

    TList<NKikimrWhiteboard::TPDiskStateInfo> PDisksAppended;
    TList<NKikimrWhiteboard::TVDiskStateInfo> VDisksAppended;

    void RemapGroup(IOutputStream& json,
                    const ::google::protobuf::Message& protoFrom,
                    const TJsonSettings& jsonSettings) {
        const auto& info = static_cast<const NKikimrViewer::TStorageGroupInfo&>(protoFrom);
        TString groupId = info.GetGroupId();
        auto ib = BSGroupIndex.find(groupId);
        if (ib != BSGroupIndex.end()) {
            TProtoToJson::ProtoToJsonInline(json, ib->second, jsonSettings);
            if (auto ih = BSGroupHiveIndex.find(groupId); ih != BSGroupHiveIndex.end()) {
                json << ',';
                TProtoToJson::ProtoToJsonInline(json, ih->second, jsonSettings);
            }
            if (auto io = BSGroupOverall.find(groupId); io != BSGroupOverall.end()) {
                json << ",\"Overall\":\"" << io->second << "\"";
            }
        }
    }

    void RemapVDisks(IOutputStream& json,
                     const ::google::protobuf::Message& protoFrom,
                     const TJsonSettings& jsonSettings) {
        NKikimrWhiteboard::EFlag diskSpace = NKikimrWhiteboard::Grey;
        json << "\"VDisks\":[";
        const auto& info = static_cast<const NKikimrWhiteboard::TBSGroupStateInfo&>(protoFrom);
        const auto& vDiskIds = info.GetVDiskIds();
        for (auto iv = vDiskIds.begin(); iv != vDiskIds.end(); ++iv) {
            if (iv != vDiskIds.begin()) {
                json << ',';
            }
            const NKikimrBlobStorage::TVDiskID& vDiskId = *iv;
            auto ie = VDisksIndex.find(vDiskId);
            if (ie != VDisksIndex.end()) {
                json << '{';
                TProtoToJson::ProtoToJsonInline(json, ie->second, jsonSettings);
                if (auto io = VDisksOverall.find(vDiskId); io != VDisksOverall.end()) {
                    json << ",\"Overall\":\"" << io->second << "\"";
                }
                json << '}';
                diskSpace = std::max(diskSpace, ie->second.GetDiskSpace());
            } else {
                json << "{\"VDiskId\":";
                TProtoToJson::ProtoToJson(json, vDiskId, jsonSettings);
                json << "}";
            }
        }
        json << ']';
        if (diskSpace != NKikimrWhiteboard::Grey) {
            json << ",\"DiskSpace\":\"";
            json << NKikimrWhiteboard::EFlag_Name(diskSpace);
            json << "\"";
        }
    }

    void RemapPDisk(IOutputStream& json,
                    const ::google::protobuf::Message& protoFrom,
                    const TJsonSettings& jsonSettings) {
        json << "\"PDisk\":";
        const auto& info = static_cast<const NKikimrWhiteboard::TVDiskStateInfo&>(protoFrom);
        ui32 nodeId = info.GetNodeId();
        ui32 pDiskId = info.GetPDiskId();
        auto ie = PDisksIndex.find(std::make_pair(nodeId, pDiskId));
        if (ie != PDisksIndex.end()) {
            TProtoToJson::ProtoToJson(json, ie->second, jsonSettings);
            if (auto io = PDisksOverall.find(std::make_pair(nodeId, pDiskId)); io != PDisksOverall.end()) {
                json << ",\"Overall\":\"" << io->second << "\"";
            }
        } else {
            json << "{\"PDiskId\":" << pDiskId << ",\"NodeId\":" << nodeId << "}";
        }
    }

    void ReplyAndPassAway() {
        TStringStream json;
        MergedBSGroupInfo = MergeWhiteboardResponses(BSGroupInfo, TWhiteboardInfo<TEvWhiteboard::TEvBSGroupStateResponse>::GetDefaultMergeField());
        MergedVDiskInfo = MergeWhiteboardResponses(VDiskInfo, TWhiteboardInfo<TEvWhiteboard::TEvVDiskStateResponse>::GetDefaultMergeField());
        MergedPDiskInfo = MergeWhiteboardResponses(PDiskInfo, TWhiteboardInfo<TEvWhiteboard::TEvPDiskStateResponse>::GetDefaultMergeField());
        for (auto& element : *TWhiteboardInfo<TEvWhiteboard::TEvPDiskStateResponse>::GetElementsField(MergedPDiskInfo.Get())) {
            element.SetStateFlag(GetWhiteboardFlag(GetPDiskStateFlag(element)));
            auto overall = NKikimrViewer::EFlag_Name(GetPDiskOverallFlag(element));
            auto key = TWhiteboardInfo<TEvWhiteboard::TEvPDiskStateResponse>::GetElementKey(element);
            element.ClearOverall();
            PDisksOverall.emplace(key, overall);
            PDisksIndex.emplace(key, element);
        }
        for (auto& element : *TWhiteboardInfo<TEvWhiteboard::TEvVDiskStateResponse>::GetElementsField(MergedVDiskInfo.Get())) {
            auto overall = NKikimrViewer::EFlag_Name(GetVDiskOverallFlag(element));
            auto key = TWhiteboardInfo<TEvWhiteboard::TEvVDiskStateResponse>::GetElementKey(element);
            element.ClearOverall();
            element.ClearStoragePoolName();
            VDisksOverall.emplace(key, overall);
            VDisksIndex.emplace(key, element);
        }
        for (auto& element : *TWhiteboardInfo<TEvWhiteboard::TEvBSGroupStateResponse>::GetElementsField(MergedBSGroupInfo.Get())) {
            auto state = GetBSGroupOverallState(element, VDisksIndex, PDisksIndex);
            auto key = ToString(TWhiteboardInfo<TEvWhiteboard::TEvBSGroupStateResponse>::GetElementKey(element));
            if (state.MissingDisks > 0) {
                BSGroupWithMissingDisks.insert(key);
            }
            if (state.SpaceProblems > 0) {
                BSGroupWithSpaceProblems.insert(key);
            }
            auto& sp = StoragePoolInfo[element.GetStoragePoolName()];
            sp.Overall = Max(sp.Overall, state.Overall);
            element.ClearOverall();
            element.ClearNodeId();
            element.ClearStoragePoolName();
            BSGroupOverall.emplace(key, NKikimrViewer::EFlag_Name(state.Overall));
            BSGroupIndex.emplace(key, element);
        }

        if (BaseConfig) {
            const NKikimrBlobStorage::TEvControllerConfigResponse& pbRecord(BaseConfig->Record);
            const NKikimrBlobStorage::TConfigResponse::TStatus& pbStatus(pbRecord.GetResponse().GetStatus(0));
            const NKikimrBlobStorage::TBaseConfig& pbConfig(pbStatus.GetBaseConfig());
            for (const NKikimrBlobStorage::TBaseConfig::TPDisk& pDisk : pbConfig.GetPDisk()) {
                std::pair<ui32, ui32> pDiskKey(pDisk.GetNodeId(), pDisk.GetPDiskId());
                auto itPDisk = PDisksIndex.find(pDiskKey);
                if (itPDisk == PDisksIndex.end()) {
                    PDisksAppended.emplace_back();
                    NKikimrWhiteboard::TPDiskStateInfo& pbPDisk = PDisksAppended.back();
                    itPDisk = PDisksIndex.emplace(pDiskKey, pbPDisk).first;
                    pbPDisk.SetNodeId(pDisk.GetNodeId());
                    pbPDisk.SetPDiskId(pDisk.GetPDiskId());
                    pbPDisk.SetPath(pDisk.GetPath());
                    pbPDisk.SetGuid(pDisk.GetGuid());
                    pbPDisk.SetCategory(static_cast<ui64>(pDisk.GetType()));
                    pbPDisk.SetTotalSize(pDisk.GetPDiskMetrics().GetTotalSize());
                    pbPDisk.SetAvailableSize(pDisk.GetPDiskMetrics().GetAvailableSize());
                }
            }
            for (const NKikimrBlobStorage::TBaseConfig::TVSlot& vDisk : pbConfig.GetVSlot()) {
                NKikimrBlobStorage::TVDiskID vDiskKey;
                vDiskKey.SetGroupID(vDisk.GetGroupId());
                vDiskKey.SetGroupGeneration(vDisk.GetGroupGeneration());
                vDiskKey.SetRing(vDisk.GetFailRealmIdx());
                vDiskKey.SetDomain(vDisk.GetFailDomainIdx());
                vDiskKey.SetVDisk(vDisk.GetVDiskIdx());

                auto itVDisk = VDisksIndex.find(vDiskKey);
                if (itVDisk == VDisksIndex.end()) {
                    VDisksAppended.emplace_back();
                    NKikimrWhiteboard::TVDiskStateInfo& pbVDisk = VDisksAppended.back();
                    itVDisk = VDisksIndex.emplace(vDiskKey, pbVDisk).first;
                    pbVDisk.MutableVDiskId()->CopyFrom(vDiskKey);
                    pbVDisk.SetNodeId(vDisk.GetVSlotId().GetNodeId());
                    pbVDisk.SetPDiskId(vDisk.GetVSlotId().GetPDiskId());
                    pbVDisk.SetAllocatedSize(vDisk.GetVDiskMetrics().GetAllocatedSize());
                }
            }
        }

        for (const auto& [hiveId, hiveStats] : HiveStorageStats) {
            for (auto& pbPool : *hiveStats->Record.MutablePools()) {
                for (auto& pbGroup : *pbPool.MutableGroups()) {
                    TString groupId = ToString(pbGroup.GetGroupID());
                    NKikimrHive::THiveStorageGroupStats& stats = BSGroupHiveIndex[groupId];
                    stats.SetAcquiredUnits(stats.GetAcquiredUnits() + pbGroup.GetAcquiredUnits());
                    stats.SetAcquiredIOPS(stats.GetAcquiredIOPS() + pbGroup.GetAcquiredIOPS());
                    stats.SetAcquiredThroughput(stats.GetAcquiredThroughput() + pbGroup.GetAcquiredThroughput());
                    stats.SetAcquiredSize(stats.GetAcquiredSize() + pbGroup.GetAcquiredSize());
                    stats.SetMaximumIOPS(stats.GetMaximumIOPS() + pbGroup.GetMaximumIOPS());
                    stats.SetMaximumThroughput(stats.GetMaximumThroughput() + pbGroup.GetMaximumThroughput());
                    stats.SetMaximumSize(stats.GetMaximumSize() + pbGroup.GetMaximumSize());
                }
            }
        }
        for (const auto& [poolName, poolInfo] : StoragePoolInfo) {
            if (!FilterStoragePools.empty() && FilterStoragePools.count(poolName) == 0) {
                continue;
            }
            NKikimrViewer::TStoragePoolInfo* pool = StorageInfo.AddStoragePools();
            for (TString groupId : poolInfo.Groups) {
                if (!FilterGroupIds.empty() && !BinarySearch(FilterGroupIds.begin(), FilterGroupIds.end(), groupId)) {
                    continue;
                }
                switch (With) {
                    case EWith::MissingDisks:
                        if (BSGroupWithMissingDisks.count(groupId) == 0) {
                            continue;
                        }
                        break;
                    case EWith::SpaceProblems:
                        if (BSGroupWithSpaceProblems.count(groupId) == 0) {
                            continue;
                        }
                        break;
                    case EWith::Everything:
                        break;
                }
                pool->AddGroups()->SetGroupId(groupId);
                auto itHiveGroup = BSGroupHiveIndex.find(groupId);
                if (itHiveGroup != BSGroupHiveIndex.end()) {
                    pool->SetAcquiredUnits(pool->GetAcquiredUnits() + itHiveGroup->second.GetAcquiredUnits());
                    pool->SetAcquiredIOPS(pool->GetAcquiredIOPS() + itHiveGroup->second.GetAcquiredIOPS());
                    pool->SetAcquiredThroughput(pool->GetAcquiredThroughput() + itHiveGroup->second.GetAcquiredThroughput());
                    pool->SetAcquiredSize(pool->GetAcquiredSize() + itHiveGroup->second.GetAcquiredSize());
                    pool->SetMaximumIOPS(pool->GetMaximumIOPS() + itHiveGroup->second.GetMaximumIOPS());
                    pool->SetMaximumThroughput(pool->GetMaximumThroughput() + itHiveGroup->second.GetMaximumThroughput());
                    pool->SetMaximumSize(pool->GetMaximumSize() + itHiveGroup->second.GetMaximumSize());
                }
            }
            if (pool->GroupsSize() == 0) {
                StorageInfo.MutableStoragePools()->RemoveLast();
                continue;
            }
            if (!poolName.empty()) {
                pool->SetName(poolName);
            }
            if (!poolInfo.Kind.empty()) {
                pool->SetKind(poolInfo.Kind);
            }
            pool->SetOverall(poolInfo.Overall);
        }

        const FieldDescriptor* field;
        if (NeedGroups) {
            field = NKikimrViewer::TStorageGroupInfo::descriptor()->FindFieldByName("GroupId");
            JsonSettings.FieldRemapper[field] = [this](
                    IOutputStream& json,
                    const ::google::protobuf::Message& protoFrom,
                    const TJsonSettings& jsonSettings) -> void {
                RemapGroup(json, protoFrom, jsonSettings);
            };
        }
        if (NeedDisks) {
            field = NKikimrWhiteboard::TBSGroupStateInfo::descriptor()->FindFieldByName("VDiskIds");
            JsonSettings.FieldRemapper[field] = [this](
                    IOutputStream& json,
                    const ::google::protobuf::Message& protoFrom,
                    const TJsonSettings& jsonSettings) -> void {
                RemapVDisks(json, protoFrom, jsonSettings);
            };
            field = NKikimrWhiteboard::TVDiskStateInfo::descriptor()->FindFieldByName("PDiskId");
            JsonSettings.FieldRemapper[field] = [this](
                    IOutputStream& json,
                    const ::google::protobuf::Message& protoFrom,
                    const TJsonSettings& jsonSettings) -> void {
                RemapPDisk(json, protoFrom, jsonSettings);
            };
        }
        TProtoToJson::ProtoToJson(json, StorageInfo, JsonSettings);
        Send(Initiator, new NMon::TEvHttpInfoRes(Viewer->GetHTTPOKJSON() + json.Str(), 0, NMon::IEvHttpInfoRes::EContentType::Custom));
        PassAway();
    }

    void HandleTimeout() {
        ReplyAndPassAway();
    }
};

template <>
struct TJsonRequestSchema<TJsonStorage> {
    static TString GetSchema() {
        TStringStream stream;
        TProtoToJson::ProtoToJsonSchema<NKikimrViewer::TStorageInfo>(stream);
        return stream.Str();
    }
};

template <>
struct TJsonRequestParameters<TJsonStorage> {
    static TString GetParameters() {
        return R"___([{"name":"enums","in":"query","description":"convert enums to strings","required":false,"type":"boolean"},
                      {"name":"ui64","in":"query","description":"return ui64 as number","required":false,"type":"boolean"},
                      {"name":"tenant","in":"query","description":"tenant name","required":false,"type":"string"},
                      {"name":"pool","in":"query","description":"storage pool name","required":false,"type":"string"},
                      {"name":"group_id","in":"query","description":"group id","required":false,"type":"integer"},
                      {"name":"need_groups","in":"query","description":"return groups information","required":false,"type":"boolean","default":true},
                      {"name":"need_disks","in":"query","description":"return disks information","required":false,"type":"boolean","default":true},
                      {"name":"with","in":"query","description":"filter groups by missing or space","required":false,"type":"string"},
                      {"name":"timeout","in":"query","description":"timeout in ms","required":false,"type":"integer"}])___";
    }
};

template <>
struct TJsonRequestSummary<TJsonStorage> {
    static TString GetSummary() {
        return "\"Storage information\"";
    }
};

template <>
struct TJsonRequestDescription<TJsonStorage> {
    static TString GetDescription() {
        return "\"Returns information about storage\"";
    }
};

}
}
