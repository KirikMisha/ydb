#include "ls_checks.h"

#include <ydb/core/engine/mkql_proto.h> 
#include <ydb/core/scheme/scheme_tablecell.h> 
#include <ydb/core/scheme/scheme_tabledefs.h> 
#include <ydb/public/lib/scheme_types/scheme_type_id.h> 

#include <library/cpp/testing/unittest/registar.h>

namespace NSchemeShardUT_Private {
namespace NLs {

using namespace NKikimr;

void NotInSubdomain(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    UNIT_ASSERT(record.HasPathDescription());
    NKikimrSchemeOp::TPathDescription descr = record.GetPathDescription();
    UNIT_ASSERT(descr.HasSelf());
    UNIT_ASSERT(descr.HasDomainDescription());
    UNIT_ASSERT(descr.GetDomainDescription().HasDomainKey());
    UNIT_ASSERT_VALUES_EQUAL(descr.GetDomainDescription().GetDomainKey().GetPathId(), 1);
}

void InSubdomain(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    PathExist(record);

    UNIT_ASSERT(record.HasPathDescription());
    const auto& descr = record.GetPathDescription();
    UNIT_ASSERT(descr.HasSelf());
    UNIT_ASSERT(descr.HasDomainDescription());
    UNIT_ASSERT(descr.GetDomainDescription().GetProcessingParams().CoordinatorsSize() > 0);
    UNIT_ASSERT(descr.GetDomainDescription().GetProcessingParams().MediatorsSize() > 0);
    UNIT_ASSERT(descr.GetDomainDescription().GetProcessingParams().GetPlanResolution() > 0);
    UNIT_ASSERT(descr.GetDomainDescription().GetProcessingParams().GetTimeCastBucketsPerMediator() > 0);
    UNIT_ASSERT(descr.GetDomainDescription().HasDomainKey());
    UNIT_ASSERT(descr.GetDomainDescription().GetDomainKey().HasPathId());
    UNIT_ASSERT_VALUES_UNEQUAL(descr.GetDomainDescription().GetDomainKey().GetPathId(), 1);
    UNIT_ASSERT(descr.GetDomainDescription().GetDomainKey().HasSchemeShard());

    if (descr.GetSelf().GetPathType() == NKikimrSchemeOp::EPathTypeSubDomain) {
        UNIT_ASSERT_VALUES_EQUAL(descr.GetSelf().GetPathId(), descr.GetDomainDescription().GetDomainKey().GetPathId());
    }
}

TCheckFunc IsSubDomain(const TString& name) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& selfPath = pathDescr.GetSelf();
        UNIT_ASSERT_VALUES_EQUAL(selfPath.GetName(), name);
        UNIT_ASSERT_VALUES_EQUAL(selfPath.GetPathType(), NKikimrSchemeOp::EPathTypeSubDomain);
    };
}

TCheckFunc IsExternalSubDomain(const TString& name) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& selfPath = pathDescr.GetSelf();
        UNIT_ASSERT_VALUES_EQUAL(selfPath.GetName(), name);
        UNIT_ASSERT_VALUES_EQUAL(selfPath.GetPathType(), NKikimrSchemeOp::EPathTypeExtSubDomain);
    };
}

bool IsGoodDomainStatus(NKikimrScheme::EStatus status) {
    switch (status) {
    case NKikimrScheme::StatusSuccess:
    case NKikimrScheme::StatusRedirectDomain:
        return true;
    default:
        return false;
    }
}

TCheckFunc ExtractTenantSchemeshard(ui64* tenantSchemeShardId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        if (record.GetStatus() == NKikimrScheme::StatusSuccess) {
            const auto& pathDescr = record.GetPathDescription();
            const auto& selfPath = pathDescr.GetSelf();
            UNIT_ASSERT_VALUES_EQUAL(selfPath.GetPathType(), NKikimrSchemeOp::EPathTypeExtSubDomain);
            UNIT_ASSERT(pathDescr.HasDomainDescription());
            const auto& domainDesc = pathDescr.GetDomainDescription();
            UNIT_ASSERT(domainDesc.HasProcessingParams());
            const auto& procParams = domainDesc.GetProcessingParams();
            *tenantSchemeShardId = procParams.GetSchemeShard();
        } else if (record.GetStatus() == NKikimrScheme::StatusRedirectDomain) {
            const auto& pathDescr = record.GetPathDescription();
            UNIT_ASSERT(pathDescr.HasDomainDescription());
            const auto& domainDesc = pathDescr.GetDomainDescription();
            UNIT_ASSERT(domainDesc.HasProcessingParams());
            const auto& procParams = domainDesc.GetProcessingParams();
            *tenantSchemeShardId = procParams.GetSchemeShard();
        }
    };
}

TCheckFunc ExtractTenantSysViewProcessor(ui64* tenantSVPId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        UNIT_ASSERT(pathDescr.HasDomainDescription());
        const auto& domainDesc = pathDescr.GetDomainDescription();
        UNIT_ASSERT(domainDesc.HasProcessingParams());
        const auto& procParams = domainDesc.GetProcessingParams();
        *tenantSVPId = procParams.GetSysViewProcessor();
    };
}

void InExternalSubdomain(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    PathRedirected(record);

    UNIT_ASSERT(record.HasLastExistedPrefixDescription());
    UNIT_ASSERT(record.HasPathDescription());
    const auto& descr = record.GetPathDescription();
    UNIT_ASSERT(descr.HasDomainDescription());
    const auto& domain = descr.GetDomainDescription();
    UNIT_ASSERT(domain.HasDomainKey());
    UNIT_ASSERT(domain.GetDomainKey().HasPathId());
    UNIT_ASSERT_VALUES_UNEQUAL(domain.GetDomainKey().GetPathId(), 1);
    UNIT_ASSERT(domain.GetDomainKey().HasSchemeShard());

    const auto& extDomainPath = record.GetLastExistedPrefixDescription();
    UNIT_ASSERT_VALUES_EQUAL(extDomainPath.GetSelf().GetPathType(), NKikimrSchemeOp::EPathTypeExtSubDomain);
}

TCheckFunc SubDomainVersion(ui64 descrVersion) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& processingParams = pathDescr.GetDomainDescription().GetProcessingParams();
        UNIT_ASSERT_EQUAL_C(processingParams.GetVersion(), descrVersion,
                            "subdomain version mistmach"
                                << ", path id " << pathDescr.GetSelf().GetPathId()
                                << ", domain id " << pathDescr.GetDomainDescription().GetDomainKey().GetPathId()
                                << ", has version " << processingParams.GetVersion()
                                << ", but expected " << descrVersion);
    };
}

TCheckFunc DomainKey(ui64 pathId, ui64 schemeshardId) {
    return DomainKey(TPathId(schemeshardId, pathId));
}

TCheckFunc DomainKey(TPathId pathId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domainKey = pathDescr.GetDomainDescription().GetDomainKey();

        UNIT_ASSERT_VALUES_EQUAL(domainKey.GetPathId(), pathId.LocalPathId);
        UNIT_ASSERT_VALUES_EQUAL(domainKey.GetSchemeShard(), pathId.OwnerId);
    };
}

TCheckFunc StoragePoolsEqual(TSet<TString> poolNames) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();

        TSet<TString> presentPools;
        for (auto& stPool: pathDescr.GetDomainDescription().GetStoragePools()) {
            presentPools.insert(stPool.GetName());
        }

        UNIT_ASSERT_VALUES_EQUAL(presentPools.size(), poolNames.size());
        UNIT_ASSERT_VALUES_EQUAL(presentPools, poolNames);
    };
}

TCheckFunc DomainCoordinators(TVector<ui64> coordinators) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& processingParams = pathDescr.GetDomainDescription().GetProcessingParams();

        UNIT_ASSERT_VALUES_EQUAL(processingParams.CoordinatorsSize(), coordinators.size());
        TVector<ui64> actual(processingParams.GetCoordinators().begin(),processingParams.GetCoordinators().end());
        UNIT_ASSERT_EQUAL(actual, coordinators);
    };
}

TCheckFunc DomainMediators(TVector<ui64> mediators) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& processingParams = pathDescr.GetDomainDescription().GetProcessingParams();

        UNIT_ASSERT_VALUES_EQUAL(processingParams.MediatorsSize(), mediators.size());
        TVector<ui64> actual(processingParams.GetMediators().begin(), processingParams.GetMediators().end());
        UNIT_ASSERT_EQUAL(actual, mediators);
    };
}

TCheckFunc DomainSchemeshard(ui64 schemeshard) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& processingParams = pathDescr.GetDomainDescription().GetProcessingParams();

        if (schemeshard) {
            UNIT_ASSERT_VALUES_EQUAL(processingParams.GetSchemeShard(), schemeshard);
        } else {
            UNIT_ASSERT(!processingParams.HasSchemeShard());
        }
    };
}

TCheckFunc DomainSettings(ui32 planResolution, ui32 timeCastBucketsPerMediator) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);

        const auto& pathDescr = record.GetPathDescription();
        const auto& processingParams = pathDescr.GetDomainDescription().GetProcessingParams();

        UNIT_ASSERT_VALUES_EQUAL(processingParams.GetPlanResolution(), planResolution);
        UNIT_ASSERT_VALUES_EQUAL(processingParams.GetTimeCastBucketsPerMediator(), timeCastBucketsPerMediator);
    };
}

TCheckFunc DatabaseSizeIs(ui64 bytes) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);

        const auto& pathDescr = record.GetPathDescription();
        const auto& totalSize = pathDescr.GetDomainDescription().GetDiskSpaceUsage().GetTables().GetTotalSize();

        UNIT_ASSERT_VALUES_EQUAL(totalSize, bytes);
    };
}

void SubdomainWithNoEmptyStoragePools(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    const auto& descr = record.GetPathDescription();
    UNIT_ASSERT(descr.GetDomainDescription().StoragePoolsSize() > 0);
}

void NotFinished(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    PathExist(record);

    if (record.HasPathDescription()) {
        NKikimrSchemeOp::TPathDescription descr = record.GetPathDescription();
        if (descr.HasSelf()) {
            UNIT_ASSERT(!descr.GetSelf().GetCreateFinished());
        }
    }
}

void Finished(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    PathExist(record);

    if (record.HasPathDescription()) {
        NKikimrSchemeOp::TPathDescription descr = record.GetPathDescription();
        if (descr.HasSelf()) {
            UNIT_ASSERT(descr.GetSelf().GetCreateFinished());
        }
    }
}

TCheckFunc ExtractVolumeConfig(NKikimrBlockStore::TVolumeConfig* config) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);

        UNIT_ASSERT(record.HasPathDescription());
        const auto& descr = record.GetPathDescription();

        UNIT_ASSERT(descr.HasBlockStoreVolumeDescription());
        const auto& volume = descr.GetBlockStoreVolumeDescription();
        UNIT_ASSERT(volume.HasVolumeConfig());
        config->CopyFrom(volume.GetVolumeConfig());
    };
}

TCheckFunc CheckMountToken(const TString& name, const TString& expectedOwner) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);

        UNIT_ASSERT(record.HasPathDescription());
        const auto& descr = record.GetPathDescription();

        UNIT_ASSERT(descr.HasSelf());
        const auto& self = descr.GetSelf();
        UNIT_ASSERT_STRINGS_EQUAL(self.GetName(), name);

        UNIT_ASSERT(descr.HasBlockStoreVolumeDescription());
        const auto& volume = descr.GetBlockStoreVolumeDescription();
        UNIT_ASSERT_STRINGS_EQUAL(volume.GetName(), name);
        UNIT_ASSERT_STRINGS_EQUAL(volume.GetMountToken(), expectedOwner);
    };
}

TCheckFunc UserAttrsEqual(TUserAttrs attrs) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();

        TUserAttrs requered = attrs;
        std::sort(requered.begin(), requered.end());

        TUserAttrs present;
        for (const auto& item: pathDescr.GetUserAttributes()) {
            present.emplace_back(item.GetKey(), item.GetValue());
        }
        std::sort(present.begin(), present.end());

        TUserAttrs diff;
        std::set_difference(present.begin(), present.end(),
                            requered.begin(), requered.end(),
                            std::back_inserter(diff));
        UNIT_ASSERT_C(diff.empty(),
                      diff.size() << " items are different, for example" <<
                          " name# '" << diff.front().first << "'" <<
                          " value# '" << diff.front().second << "'" <<
                          " the item is extra or has a different value in listing");

        diff.clear();
        std::set_difference(requered.begin(), requered.end(),
                            present.begin(), present.end(),
                            std::back_inserter(diff));
        UNIT_ASSERT_C(diff.empty(),
                      diff.size() << " items are different, for example" <<
                          " name# '" << diff.front().first << "' " <<
                          " value# '" << diff.front().second << "' " <<
                          " the item is missed");
    };
}

TCheckFunc UserAttrsHas(TUserAttrs attrs) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();

        TUserAttrs requered = attrs;
        std::sort(requered.begin(), requered.end());

        TUserAttrs present;
        for (const auto& item: pathDescr.GetUserAttributes()) {
            present.emplace_back(item.GetKey(), item.GetValue());
        }
        std::sort(present.begin(), present.end());

        TUserAttrs diff;
        std::set_difference(requered.begin(), requered.end(),
                            present.begin(), present.end(),
                            std::back_inserter(diff));
        UNIT_ASSERT_C(diff.empty(),
                      diff.size() << " items are different, for example missed in listing " <<
                          "name# '" << diff.front().first << "' " <<
                          "value# '" << diff.front().second << "'");
    };
}

void IsTable(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
    const auto& pathDescr = record.GetPathDescription();
    const auto& selfPath = pathDescr.GetSelf();
    UNIT_ASSERT_VALUES_EQUAL(selfPath.GetPathType(), NKikimrSchemeOp::EPathTypeTable);
}

TCheckFunc CheckColumns(const TString& name, const TSet<TString>& columns, const TSet<TString>& droppedColumns, const TSet<TString> keyColumns,
                        NKikimrSchemeOp::EPathState pathState) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT(record.HasPathDescription());
        NKikimrSchemeOp::TPathDescription descr = record.GetPathDescription();

        UNIT_ASSERT(descr.HasSelf());
        auto self = descr.GetSelf();
        UNIT_ASSERT(self.HasCreateFinished());
        TString curName = self.GetName();
        ui32 curPathState = self.GetPathState();
        UNIT_ASSERT_STRINGS_EQUAL(curName, name);
        UNIT_ASSERT_VALUES_EQUAL(curPathState, (ui32)pathState);

        UNIT_ASSERT(descr.HasTable());
        NKikimrSchemeOp::TTableDescription table = descr.GetTable();
        UNIT_ASSERT(table.ColumnsSize());

        for (auto& col : table.GetColumns()) {
            UNIT_ASSERT(col.HasName());
            UNIT_ASSERT(col.HasId());
            UNIT_ASSERT(col.HasTypeId());

            TString name = col.GetName();
            UNIT_ASSERT(columns.contains(name));
            UNIT_ASSERT(!droppedColumns.contains(name));
        }

        for (auto& keyName : table.GetKeyColumnNames()) {
            UNIT_ASSERT(keyColumns.contains(keyName));
        }
    };
}

void CheckBoundaries(const NKikimrScheme::TEvDescribeSchemeResult &record) {
    const NKikimrSchemeOp::TPathDescription& descr = record.GetPathDescription();
    THashMap<ui32, NScheme::TTypeId> colTypes;
    for (const auto& col : descr.GetTable().GetColumns()) {
        colTypes[col.GetId()] = col.GetTypeId();
    }
    TVector<NScheme::TTypeId> keyColTypes;
    for (const auto& ki : descr.GetTable().GetKeyColumnIds()) {
        keyColTypes.push_back(colTypes[ki]);
    }

    UNIT_ASSERT_VALUES_EQUAL(descr.GetTable().SplitBoundarySize() + 1, descr.TablePartitionsSize());
    TString errStr;
    for (ui32 i = 0; i < descr.GetTable().SplitBoundarySize(); ++i) {
        const auto& b = descr.GetTable().GetSplitBoundary(i);
        TVector<TCell> cells;
        NMiniKQL::CellsFromTuple(nullptr, b.GetKeyPrefix(), keyColTypes, false, cells, errStr);
        UNIT_ASSERT_VALUES_EQUAL(errStr, "");

        TString serialized = TSerializedCellVec::Serialize(cells);
        UNIT_ASSERT_NO_DIFF(serialized, descr.GetTablePartitions(i).GetEndOfRangeKeyPrefix());
    }
}

TCheckFunc CheckPartCount(const TString& name, ui32 partCount, ui32 maxParts, ui32 tabletCount, ui32 groupCount,
                          NKikimrSchemeOp::EPathState pathState) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT(record.HasPathDescription());
        NKikimrSchemeOp::TPathDescription descr = record.GetPathDescription();

        UNIT_ASSERT(descr.HasSelf());
        auto self = descr.GetSelf();
        UNIT_ASSERT(self.HasCreateFinished());
        TString curName = self.GetName();
        ui32 curPathState = self.GetPathState();
        UNIT_ASSERT_STRINGS_EQUAL(curName, name);
        UNIT_ASSERT_VALUES_EQUAL(curPathState, (ui32)pathState);

        UNIT_ASSERT(descr.HasPersQueueGroup());
        NKikimrSchemeOp::TPersQueueGroupDescription pqGroup = descr.GetPersQueueGroup();

        UNIT_ASSERT(pqGroup.HasNextPartitionId());
        UNIT_ASSERT(pqGroup.HasTotalGroupCount());
        UNIT_ASSERT(pqGroup.GetTotalGroupCount() <= pqGroup.GetNextPartitionId());
        ui64 totalGroupCount = pqGroup.GetTotalGroupCount();
        ui32 curMaxParts = pqGroup.GetPartitionPerTablet();
        ui32 curPartsCount = pqGroup.PartitionsSize();
        ui32 nextPartId = pqGroup.GetNextPartitionId();

        UNIT_ASSERT_VALUES_EQUAL(curPartsCount, partCount);
        UNIT_ASSERT_VALUES_EQUAL(totalGroupCount, groupCount);
        UNIT_ASSERT_VALUES_EQUAL(curMaxParts, maxParts);
        UNIT_ASSERT_VALUES_EQUAL(nextPartId, partCount);

        TSet<ui32> pqs;
        TSet<ui32> tablets;
        for (size_t i = 0; i < curPartsCount; ++i) {
            auto& part = pqGroup.GetPartitions(i);
            UNIT_ASSERT(part.HasPartitionId());
            UNIT_ASSERT(part.HasTabletId());

            ui32 pq = pqGroup.GetPartitions(i).GetPartitionId();
            ui32 tabletId = pqGroup.GetPartitions(i).GetTabletId();

            pqs.insert(pq);
            tablets.insert(tabletId);
        }

        UNIT_ASSERT_VALUES_EQUAL(partCount, pqs.size());
        UNIT_ASSERT_VALUES_EQUAL(tabletCount, tablets.size());

        if (!pqs.empty()) {
            UNIT_ASSERT_VALUES_EQUAL(*pqs.begin(), 0);
            UNIT_ASSERT_VALUES_EQUAL(*pqs.rbegin(), partCount - 1);
        }
    };
}

TCheckFunc PathVersionEqual(ui64 version) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& self = pathDescr.GetSelf();
        const auto& curVersion = self.GetPathVersion();

        UNIT_ASSERT_EQUAL_C(curVersion, version,
                            "path version mistmach, path with id " << self.GetPathId() <<
                                " has version " << curVersion <<
                                " but expected " << version);
    };
}

TCheckFunc PathVersionOneOf(TSet<ui64> versions) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& self = pathDescr.GetSelf();
        const auto& curVersion = self.GetPathVersion();

        UNIT_ASSERT_C(versions.count(curVersion) > 0,
                      "path version mistmach, path with id " << self.GetPathId() <<
                          " has version " << curVersion <<
                          " but expected one of set");
    };
}

TCheckFunc PathIdEqual(ui64 pathId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& self = pathDescr.GetSelf();
        const auto& curId = self.GetPathId();
        UNIT_ASSERT_VALUES_EQUAL(curId, pathId);
    };
}

TCheckFunc PathIdEqual(TPathId pathId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& self = pathDescr.GetSelf();
        const auto& curId = TPathId(self.GetSchemeshardId(), self.GetPathId());
        UNIT_ASSERT_VALUES_EQUAL(curId, pathId);
    };
}

TCheckFunc PathStringEqual(const TString& expected) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPath(), expected);
    };
}

TCheckFunc PathsInsideDomain(ui64 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& curCount = domain.GetPathsInside();

        UNIT_ASSERT_EQUAL_C(curCount, count,
                            "paths inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                                " has count " << curCount <<
                                " but expected " << count);
    };
}

TCheckFunc PQPartitionsInsideDomain(ui64 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& curCount = domain.GetPQPartitionsInside();

        UNIT_ASSERT_EQUAL_C(curCount, count,
                            "pq partitions inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                                " has count " << curCount <<
                                " but expected " << count);
    };
}

TCheckFunc PathsInsideDomainOneOf(TSet<ui64> variants) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& curCount = domain.GetPathsInside();

        UNIT_ASSERT_C(variants.count(curCount) > 0,
                      "paths inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has version " << curCount <<
                          " but expected one of set");
    };
}

TCheckFunc ShardsInsideDomain(ui64 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& curCount = domain.GetShardsInside();

        UNIT_ASSERT_C(count == curCount,
                      "shards inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has shardsInsize " << curCount <<
                          " but expected " << count);
    };
}

TCheckFunc ShardsInsideDomainOneOf(TSet<ui64> variants) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& curCount = domain.GetShardsInside();

        UNIT_ASSERT_C(variants.count(curCount) > 0,
                      "paths inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has shards " << curCount <<
                          " but expected one of set");
    };
}

TCheckFunc DomainLimitsIs(ui64 maxPaths, ui64 maxShards, ui64 maxPQPartitions) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto& pathLimit = domain.GetPathsLimit();
        const auto& shardsLimit = domain.GetShardsLimit();
        const auto& pqPartitionsLimit = domain.GetPQPartitionsLimit();

        UNIT_ASSERT_C(pathLimit == maxPaths,
                      "paths limit mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has limit " << pathLimit <<
                          " but expected " << maxPaths);

        UNIT_ASSERT_C(shardsLimit == maxShards,
                      "shards limit mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has limit " << shardsLimit <<
                          " but expected " << maxShards);

        UNIT_ASSERT_C(!maxPQPartitions || pqPartitionsLimit == maxPQPartitions,
                      "pq partitions limit mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has limit " << pqPartitionsLimit <<
                          " but expected " << maxPQPartitions);
    };
}

TCheckFunc FreezeStateEqual(NKikimrSchemeOp::EFreezeState expectedState) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
        UNIT_ASSERT(record.HasPathDescription());
        UNIT_ASSERT(record.GetPathDescription().HasTable());
        UNIT_ASSERT(record.GetPathDescription().GetTable().HasPartitionConfig());
        UNIT_ASSERT(record.GetPathDescription().GetTable().GetPartitionConfig().HasFreezeState());
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetFreezeState(), expectedState);
    };
}

TCheckFunc ChildrenCount(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().ChildrenSize(), count);
    };
}

TCheckFunc IndexesCount(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().TableIndexesSize(), count);
    };
}

TCheckFunc IndexType(NKikimrSchemeOp::EIndexType type) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().GetType(), type);
    };
}

TCheckFunc IndexState(NKikimrSchemeOp::EIndexState state) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().GetState(), state);
    };
}

TCheckFunc IndexKeys(const TVector<TString>& keyNames) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().KeyColumnNamesSize(), keyNames.size());
        for (ui32 keyId = 0; keyId < keyNames.size(); ++keyId) {
            UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().GetKeyColumnNames(keyId), keyNames.at(keyId));
        }
    };
}

TCheckFunc IndexDataColumns(const TVector<TString>& dataColumnNames) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().DataColumnNamesSize(), dataColumnNames.size());
        for (ui32 colId = 0; colId < dataColumnNames.size(); ++colId) {
            UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTableIndex().GetDataColumnNames(colId), dataColumnNames.at(colId));
        }
    };
}

void NoChildren(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    ChildrenCount(0)(record);
}

void PathNotExist(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusPathDoesNotExist);
}

void PathExist(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusSuccess);
}

void PathRedirected(const NKikimrScheme::TEvDescribeSchemeResult& record) {
    UNIT_ASSERT_VALUES_EQUAL(record.GetStatus(), NKikimrScheme::StatusRedirectDomain);
}

TCheckFunc CreatedAt(ui64 txId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetSelf().GetCreateTxId(), txId);
    };
}

TCheckFunc PartitionCount(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().TablePartitionsSize(), count);
    };
}

TCheckFunc FollowerCount(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetFollowerCount(), count);
    };
}

TCheckFunc CrossDataCenterFollowerCount(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetCrossDataCenterFollowerCount(), count);
    };
}

TCheckFunc AllowFollowerPromotion(bool val) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetAllowFollowerPromotion(), val);
    };
}

TCheckFunc FollowerGroups(const TVector<NKikimrHive::TFollowerGroup>& followerGroups) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().FollowerGroupsSize(), followerGroups.size());
        for (size_t i = 0; i < followerGroups.size(); ++i) {
            const auto& srcSG = record.GetPathDescription().GetTable().GetPartitionConfig().GetFollowerGroups(i);
            const auto& dstSG = followerGroups[i];

            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetFollowerCount(), dstSG.GetFollowerCount());
            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetAllowLeaderPromotion(), dstSG.GetAllowLeaderPromotion());
            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetAllowClientRead(), dstSG.GetAllowClientRead());

            UNIT_ASSERT_VALUES_EQUAL(srcSG.AllowedNodeIDsSize(), dstSG.AllowedNodeIDsSize());
            for(ui32 i = 0; i < srcSG.AllowedNodeIDsSize(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(srcSG.GetAllowedNodeIDs(i), dstSG.GetAllowedNodeIDs(i));
            }

            UNIT_ASSERT_VALUES_EQUAL(srcSG.AllowedDataCentersSize(), dstSG.AllowedDataCentersSize());
            for(ui32 i = 0; i < srcSG.AllowedDataCentersSize(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(srcSG.GetAllowedDataCenters(i), dstSG.GetAllowedDataCenters(i));
            }
            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetRequireAllDataCenters(), dstSG.GetRequireAllDataCenters());
            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetLocalNodeOnly(), dstSG.GetLocalNodeOnly());
            UNIT_ASSERT_VALUES_EQUAL(srcSG.GetRequireDifferentNodes(), dstSG.GetRequireDifferentNodes());
        }
    };
}

TCheckFunc SizeToSplitEqual(ui32 size) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetPartitioningPolicy().GetSizeToSplit(), size);
    };
}

TCheckFunc MinPartitionsCountEqual(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetPartitioningPolicy().GetMinPartitionsCount(), count);
    };
}

TCheckFunc MaxPartitionsCountEqual(ui32 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetPartitioningPolicy().GetMaxPartitionsCount(), count);
    };
}

TCheckFunc PartitioningByLoadStatus(bool status) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetPartitioningPolicy().GetSplitByLoadSettings().GetEnabled(), status);
    };
}

void NoBackupInFly(const NKikimrScheme::TEvDescribeSchemeResult &record) {
    UNIT_ASSERT(!record.GetPathDescription().HasBackupProgress());
}

TCheckFunc HasBackupInFly(ui64 txId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT(record.GetPathDescription().HasBackupProgress());
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetBackupProgress().GetTxId(), txId);
    };
}

TCheckFunc BackupHistoryCount(ui64 count) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().LastBackupResultSize(), count);
    };
}

TCheckFunc ColumnFamiliesCount(ui32 size) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().ColumnFamiliesSize(), size);
    };
}

TCheckFunc ColumnFamiliesHas(ui32 familyId) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        bool has = false;
        for (const auto& x: record.GetPathDescription().GetTable().GetPartitionConfig().GetColumnFamilies()) {
            if (x.GetId() == familyId) {
                has = true;
                break;
            }
        }
        UNIT_ASSERT(has);
    };
}

TCheckFunc ColumnFamiliesHas(ui32 familyId, const TString& familyName) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        bool has = false;
        for (const auto& x: record.GetPathDescription().GetTable().GetPartitionConfig().GetColumnFamilies()) {
            if (x.GetId() == familyId && x.GetName() == familyName) {
                has = true;
                break;
            }
        }
        UNIT_ASSERT(has);
    };
}

TCheckFunc KeyBloomFilterStatus(bool status) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(record.GetPathDescription().GetTable().GetPartitionConfig().GetEnableFilterByKey(), status);
    };
}

TCheckFunc HasTtlEnabled(const TString& columnName, const TDuration& expireAfter, NKikimrSchemeOp::TTTLSettings::EUnit columnUnit) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& ttl = record.GetPathDescription().GetTable().GetTTLSettings();
        UNIT_ASSERT(ttl.HasEnabled());
        UNIT_ASSERT_VALUES_EQUAL(ttl.GetEnabled().GetColumnName(), columnName);
        UNIT_ASSERT_VALUES_EQUAL(ttl.GetEnabled().GetColumnUnit(), columnUnit);
        UNIT_ASSERT_VALUES_EQUAL(ttl.GetEnabled().GetExpireAfterSeconds(), expireAfter.Seconds());
    };
}

TCheckFunc HasTtlDisabled() {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& ttl = record.GetPathDescription().GetTable().GetTTLSettings();
        UNIT_ASSERT(ttl.HasDisabled());
    };
}

TCheckFunc IsBackupTable(bool value) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_VALUES_EQUAL(value, record.GetPathDescription().GetTable().GetIsBackup());
    };
}

TCheckFunc HasOlapTableSchemaPreset(const TString& presetName) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT_VALUES_EQUAL(table.GetSchemaPresetName(), presetName);
    };
}

TCheckFunc HasOlapTableSchemaVersion(ui64 schemaVersion) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT_VALUES_EQUAL(table.GetSchema().GetVersion(), schemaVersion);
    };
}

TCheckFunc HasOlapTableTtlSettingsVersion(ui64 ttlSettingsVersion) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT_VALUES_EQUAL(table.GetTtlSettings().GetVersion(), ttlSettingsVersion);
    };
}

TCheckFunc HasOlapTableTtlSettingsEnabled(const TString& columnName, const TDuration& expireAfter) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT(table.HasTtlSettings());
        const auto& ttl = table.GetTtlSettings();
        UNIT_ASSERT(ttl.HasEnabled());
        UNIT_ASSERT_VALUES_EQUAL(ttl.GetEnabled().GetColumnName(), columnName);
        UNIT_ASSERT_VALUES_EQUAL(ttl.GetEnabled().GetExpireAfterSeconds(), expireAfter.Seconds());
    };
}

TCheckFunc HasOlapTableTtlSettingsDisabled() {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT(table.HasTtlSettings());
        const auto& ttl = table.GetTtlSettings();
        UNIT_ASSERT(ttl.HasDisabled());
    };
}

TCheckFunc HasOlapTableTtlSettingsTiering(ui32 tierNo, const TString& tierName, const TString& columnName,
                                          const TDuration& evictAfter) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& table = record.GetPathDescription().GetColumnTableDescription();
        UNIT_ASSERT(table.HasTtlSettings());
        const auto& ttl = table.GetTtlSettings();
        UNIT_ASSERT(ttl.HasTiering());
        UNIT_ASSERT(ttl.GetTiering().TiersSize() > tierNo);
        const auto& tier = ttl.GetTiering().GetTiers()[tierNo];
        UNIT_ASSERT_VALUES_EQUAL(tier.GetName(), tierName);
        UNIT_ASSERT(tier.HasEviction());
        UNIT_ASSERT_VALUES_EQUAL(tier.GetEviction().GetColumnName(), columnName);
        UNIT_ASSERT_VALUES_EQUAL(tier.GetEviction().GetExpireAfterSeconds(), evictAfter.Seconds());
    };
}

void CheckEffectiveRight(const NKikimrScheme::TEvDescribeSchemeResult& record, const TString& right, bool mustHave) {
    const auto& self = record.GetPathDescription().GetSelf();
    TSecurityObject src(self.GetOwner(), self.GetEffectiveACL(), false);

    NACLib::TSecurityObject required;
    required.FromString(right);

    for (const auto& requeriedAce : required.GetACL().GetACE()) {
        bool has = false;

        for (const auto& srcAce: src.GetACL().GetACE()) {
            if (srcAce.GetAccessType() == requeriedAce.GetAccessType() &&
                srcAce.GetAccessRight() == requeriedAce.GetAccessRight() &&
                srcAce.GetSID() == requeriedAce.GetSID() &&
                srcAce.GetInheritanceType() == requeriedAce.GetInheritanceType())
            {
                has = true;
            }
        }

        UNIT_ASSERT_C(!(has ^ mustHave), "" << (mustHave ? "no " : "") << "ace found"
            << ", got " << src.ShortDebugString()
            << ", required " << required.ShortDebugString());
    }
}

TCheckFunc HasEffectiveRight(const TString& right) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        CheckEffectiveRight(record, right, true);
    };
}

TCheckFunc HasNotEffectiveRight(const TString& right) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        CheckEffectiveRight(record, right, false);
    };
}

TCheckFunc KesusConfigIs(ui64 self_check_period_millis, ui64 session_grace_period_millis) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& config = record.GetPathDescription().GetKesus().GetConfig();
        UNIT_ASSERT_VALUES_EQUAL(config.self_check_period_millis(), self_check_period_millis);
        UNIT_ASSERT_VALUES_EQUAL(config.session_grace_period_millis(), session_grace_period_millis);
    };
}

TCheckFunc DatabaseQuotas(ui64 dataStreamShards) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        UNIT_ASSERT_C(IsGoodDomainStatus(record.GetStatus()), "Unexpected status: " << record.GetStatus());

        const auto& pathDescr = record.GetPathDescription();
        const auto& domain = pathDescr.GetDomainDescription();
        const auto count = domain.GetDatabaseQuotas().data_stream_shards_quota();

        UNIT_ASSERT_C(count == dataStreamShards,
                      "data stream shards inside domain count mistmach, domain with id " << domain.GetDomainKey().GetPathId() <<
                          " has data stream shards " << count <<
                          " but expected " << dataStreamShards);
    };
}

TCheckFunc PartitionKeys(TVector<TString> lastShardKeys) {
    return [=] (const NKikimrScheme::TEvDescribeSchemeResult& record) {
        const auto& pathDescr = record.GetPathDescription();
        UNIT_ASSERT_VALUES_EQUAL(lastShardKeys.size(), pathDescr.TablePartitionsSize());
        for (size_t i = 0; i < lastShardKeys.size(); ++i) {
            UNIT_ASSERT_STRING_CONTAINS(pathDescr.GetTablePartitions(i).GetEndOfRangeKeyPrefix(), lastShardKeys[i]);
        }
    };
}

} // NLs
} // NSchemeShardUT_Private

Y_DECLARE_OUT_SPEC(inline, NKikimrSchemeOp::TTTLSettings::EUnit, o, x) {
    o << NKikimrSchemeOp::TTTLSettings::EUnit_Name(x);
}

Y_DECLARE_OUT_SPEC(inline, NKikimrScheme::EStatus, o, x) {
    o << NKikimrScheme::EStatus_Name(x);
}

Y_DECLARE_OUT_SPEC(inline, NKikimrSchemeOp::EPathType, o, x) {
    o << NKikimrSchemeOp::EPathType_Name(x);
}

Y_DECLARE_OUT_SPEC(inline, NKikimrSchemeOp::EFreezeState, o, x) {
    o << NKikimrSchemeOp::EFreezeState_Name(x);
}

Y_DECLARE_OUT_SPEC(inline, NKikimrSchemeOp::EIndexType, o, x) {
    o << NKikimrSchemeOp::EIndexType_Name(x);
}

Y_DECLARE_OUT_SPEC(inline, NKikimrSchemeOp::EIndexState, o, x) {
    o << NKikimrSchemeOp::EIndexState_Name(x);
}
