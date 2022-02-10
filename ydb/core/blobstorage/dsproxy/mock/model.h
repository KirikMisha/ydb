#pragma once

#include <ydb/core/base/blobstorage.h> 

namespace NKikimr {
namespace NFake {

    class TProxyDS : public TThrRefBase {
        using TTabletId = ui64;
        using TChannel = ui8;
        using TGeneration = ui32;
        using TStep = ui32;

        struct TBlob {
            TBlob(TString buffer)
                : Buffer(std::move(buffer))
            {}

            TString Buffer;
            bool Keep = false;
            bool DoNotKeep = false;
        };

        struct TBarrier {
            TGeneration RecordGeneration = 0;
            ui32        PerGenerationCounter = 0;
            TGeneration CollectGeneration = 0;
            TStep       CollectStep = 0;

            TBarrier() = default;
            TBarrier(const TBarrier& other) = default;

            TBarrier(TGeneration recordGeneration, ui32 perGenerationCounter, TGeneration collectGeneration,
                    TStep collectStep)
                : RecordGeneration(recordGeneration)
                , PerGenerationCounter(perGenerationCounter)
                , CollectGeneration(collectGeneration)
                , CollectStep(collectStep)
            {}

            std::pair<TGeneration, TStep> MakeCollectPair() const {
                return std::make_pair(CollectGeneration, CollectStep);
            }
        };

        THashMap<TTabletId, TGeneration> Blocks;
        THashMap<std::pair<TTabletId, TChannel>, TBarrier> Barriers;
        THashMap<std::pair<TTabletId, TChannel>, TBarrier> HardBarriers;
        TMap<TLogoBlobID, TBlob> Blobs;

    public: // BS events interface : Handle(event) -> event
        TEvBlobStorage::TEvPutResult* Handle(TEvBlobStorage::TEvPut *msg) {
            // ensure we have full blob id, with PartId set to zero
            const TLogoBlobID& id = msg->Id;
            Y_VERIFY(id == id.FullID());

            // validate put against set blocks
            if (IsBlocked(id.TabletID(), id.Generation())) {
                return new TEvBlobStorage::TEvPutResult(NKikimrProto::BLOCKED, id,
                        GetStorageStatusFlags(), 0, 0.f);
            }

            // check if this blob is not being collected -- writing such blob is a violation of BS contract
            Y_VERIFY(!IsCollectedByBarrier(id), "Id# %s", id.ToString().data());

            // validate that there are no blobs with the same gen/step, channel, cookie, but with different size
            const TLogoBlobID base(id.TabletID(), id.Generation(), id.Step(), id.Channel(), 0, id.Cookie());
            auto iter = Blobs.lower_bound(base);
            if (iter != Blobs.end()) {
                const TLogoBlobID& existing = iter->first;
                Y_VERIFY(
                    id.TabletID() != existing.TabletID() ||
                    id.Generation() != existing.Generation() ||
                    id.Step() != existing.Step() ||
                    id.Cookie() != existing.Cookie() ||
                    id.Channel() != existing.Channel() ||
                    id == existing,
                    "id# %s existing# %s", id.ToString().data(), existing.ToString().data());
                if (id == existing) {
                    Y_VERIFY(iter->second.Buffer == msg->Buffer);
                }
            }

            // put an entry into logo blobs database and reply with success
            Blobs.emplace(id, std::move(msg->Buffer));
            return new TEvBlobStorage::TEvPutResult(NKikimrProto::OK, id, GetStorageStatusFlags(), 0, 0.f);
        }

        TEvBlobStorage::TEvGetResult* Handle(TEvBlobStorage::TEvGet *msg) {
            // prepare result structure holding the returned data
            auto result = std::make_unique<TEvBlobStorage::TEvGetResult>(NKikimrProto::OK, msg->QuerySize, 0u);

            // traverse against requested blobs and process them
            for (ui32 i = 0; i < msg->QuerySize; ++i) {
                const TEvBlobStorage::TEvGet::TQuery& query = msg->Queries[i];

                const TLogoBlobID& id = query.Id;
                Y_VERIFY(id == id.FullID());

                TEvBlobStorage::TEvGetResult::TResponse& response = result->Responses[i];
                response.Id = id;
                response.Shift = query.Shift;
                response.RequestedSize = query.Size;

                auto it = Blobs.find(id);
                if (it != Blobs.end()) {
                    const auto& data = it->second;

                    // we have blob and it's not under GC (otherwise it would be already deleted) -- so we reply
                    // with success and return requested part of blob
                    response.Status = NKikimrProto::OK;

                    // maximum size we can return when seeking to provided Shift
                    const ui32 maxSize = query.Shift < data.Buffer.size() ? data.Buffer.size() - query.Shift : 0;

                    // actual size we are going to return
                    const ui32 size = Min<ui32>(maxSize, !query.Size ? Max<ui32>() : query.Size);

                    // calculate substring; use 0 instead of query.Shift because it may exceed the buffer
                    response.Buffer = data.Buffer.substr(size ? query.Shift : 0, size);
                } else {
                    // ensure this blob is not under GC
                    Y_VERIFY(!IsCollectedByBarrier(id), "Id# %s", id.ToString().data());

                    // reply with NODATA -- we haven't got this blob
                    response.Status = NKikimrProto::NODATA;
                }
            }

            return result.release();
        }

        TEvBlobStorage::TEvBlockResult* Handle(TEvBlobStorage::TEvBlock *msg) {
            NKikimrProto::EReplyStatus status = NKikimrProto::OK;

            auto it = Blocks.find(msg->TabletId);
            if (it == Blocks.end()) {
                Blocks.emplace(msg->TabletId, msg->Generation);
            } else if (msg->Generation <= it->second) {
                status = NKikimrProto::RACE;
            } else {
                it->second = msg->Generation;
            }

            return new TEvBlobStorage::TEvBlockResult(status);
        }

        TEvBlobStorage::TEvDiscoverResult* Handle(TEvBlobStorage::TEvDiscover *msg) {
            ui32 blockedGeneration = 0;
            if (msg->DiscoverBlockedGeneration) {
                auto blockit = Blocks.find(msg->TabletId);
                if (blockit != Blocks.end()) {
                    blockedGeneration = blockit->second;
                }
            }

            std::unique_ptr<TEvBlobStorage::TEvDiscoverResult> result;

            TLogoBlobID id(msg->TabletId, Max<ui32>(), Max<ui32>(), 0, TLogoBlobID::MaxBlobSize, TLogoBlobID::MaxCookie);
            auto it = Blobs.upper_bound(id);
            if (it != Blobs.begin()) {
                --it;
                const TLogoBlobID& lastBlobId = it->first;
                if (lastBlobId.TabletID() == msg->TabletId && lastBlobId.Channel() == 0 &&
                        lastBlobId.Generation() >= msg->MinGeneration) {
                    TString buffer;
                    if (msg->ReadBody) {
                        buffer = it->second.Buffer;
                    }

                    result = std::make_unique<TEvBlobStorage::TEvDiscoverResult>(lastBlobId, msg->MinGeneration, buffer,
                            blockedGeneration);
                }
            }

            if (!result) {
                result = std::make_unique<TEvBlobStorage::TEvDiscoverResult>(NKikimrProto::NODATA, msg->MinGeneration,
                        blockedGeneration);
            }

            return result.release();
        }

        TEvBlobStorage::TEvRangeResult* Handle(TEvBlobStorage::TEvRange *msg) {
            const TLogoBlobID& from = msg->From;
            const TLogoBlobID& to = msg->To;

            Y_VERIFY(from.TabletID() == to.TabletID());
            Y_VERIFY(from.Channel() == to.Channel());
            Y_VERIFY(from.TabletID() == msg->TabletId);

            auto result = std::make_unique<TEvBlobStorage::TEvRangeResult>(NKikimrProto::OK, from, to, 0u);

            auto process = [&](const TLogoBlobID& id, const TString& buffer) {
                result->Responses.emplace_back(id, buffer);
            };

            if (from <= to) {
                // forward scan
                for (auto it = Blobs.lower_bound(from); it != Blobs.end() && it->first <= to; ++it) {
                    process(it->first, it->second.Buffer);
                }
            } else {
                // reverse scan
                for (auto it = Blobs.upper_bound(from); it != Blobs.begin(); ) {
                    --it;
                    if (it->first < to) {
                        break;
                    } else {
                        process(it->first, it->second.Buffer);
                    }
                }
            }

            return result.release();
        }

        TEvBlobStorage::TEvCollectGarbageResult* Handle(TEvBlobStorage::TEvCollectGarbage *msg) {
            if (IsBlocked(msg->TabletId, msg->RecordGeneration) && (msg->CollectGeneration != Max<ui32>() ||
                    msg->CollectStep != Max<ui32>() || Blocks.at(msg->TabletId) != Max<ui32>())) {
                return new TEvBlobStorage::TEvCollectGarbageResult(NKikimrProto::BLOCKED,
                        msg->TabletId, msg->RecordGeneration, msg->PerGenerationCounter, msg->Channel);
            }

            if (msg->Hard) {
                Y_VERIFY(!msg->Keep, "Unexpected Keep in HARD collect, msg# %s", msg->ToString().c_str());
                Y_VERIFY(!msg->DoNotKeep, "Unexpected DoNotKeep in HARD collect, msg# %s", msg->ToString().c_str());
                Y_VERIFY(msg->Collect, "Missing Collect in HARD collect, msg# %s", msg->ToString().c_str());

                std::pair<TTabletId, TChannel> key(msg->TabletId, msg->Channel);
                auto it = HardBarriers.find(key);
                if (it != HardBarriers.end()) {
                    const TBarrier& barrier = it->second;

                    if (msg->RecordGeneration == barrier.RecordGeneration &&
                            msg->PerGenerationCounter == barrier.PerGenerationCounter) {
                        // we already have this record; ensure that the data is the same
                        Y_VERIFY(msg->CollectGeneration == barrier.CollectGeneration &&
                                msg->CollectStep == barrier.CollectStep);
                    } else {
                        Y_VERIFY(std::make_pair(msg->RecordGeneration, msg->PerGenerationCounter) >
                                std::make_pair(barrier.RecordGeneration, barrier.PerGenerationCounter));
                        Y_VERIFY(std::make_pair(msg->CollectGeneration, msg->CollectStep) >=
                                barrier.MakeCollectPair(),
                                "tabletId %" PRIu64 " requested hard collect %" PRIu32 ":%" PRIu32
                                " existing barrier %" PRIu32 ":%" PRIu32 " msg# %s",
                                msg->TabletId, msg->CollectGeneration, msg->CollectStep,
                                barrier.CollectGeneration, barrier.CollectStep, msg->ToString().c_str());
                    }
                }

                // put new hard barrier
                HardBarriers[key] = {msg->RecordGeneration, msg->PerGenerationCounter, msg->CollectGeneration,
                    msg->CollectStep};

                DoCollection(msg->TabletId, msg->Channel, true, msg->CollectGeneration, msg->CollectStep);
            } else {
                if (msg->Keep) {
                    for (const TLogoBlobID& id : *msg->Keep) {
                        auto it = Blobs.find(id);
                        Y_VERIFY(it != Blobs.end(), "Id# %s", id.ToString().data());
                        Y_VERIFY(!IsCollectedByBarrier(id) || (it->second.Keep && !it->second.DoNotKeep),
                            "Id# %s Keep# %s DoNotKeep# %s", id.ToString().data(),
                            it->second.Keep ? "true" : "false",
                            it->second.DoNotKeep ? "true" : "false");
                        Y_VERIFY(!it->second.DoNotKeep);
                        it->second.Keep = true;
                    }
                }

                if (msg->DoNotKeep) {
                    for (const TLogoBlobID& id : *msg->DoNotKeep) {
                        auto it = Blobs.find(id);
                        if (it != Blobs.end()) {
                            it->second.DoNotKeep = true;
                        }
                    }
                }

                if (msg->Collect) {
                    std::pair<TTabletId, TChannel> key(msg->TabletId, msg->Channel);
                    auto it = Barriers.find(key);
                    if (it != Barriers.end()) {
                        const TBarrier& barrier = it->second;

                        if (msg->RecordGeneration == barrier.RecordGeneration &&
                                msg->PerGenerationCounter == barrier.PerGenerationCounter) {
                            // we already have this record; ensure that the data is the same
                            Y_VERIFY(msg->CollectGeneration == barrier.CollectGeneration &&
                                    msg->CollectStep == barrier.CollectStep);
                        } else {
                            Y_VERIFY(std::make_pair(msg->RecordGeneration, msg->PerGenerationCounter) >
                                    std::make_pair(barrier.RecordGeneration, barrier.PerGenerationCounter));
                            Y_VERIFY(std::make_pair(msg->CollectGeneration, msg->CollectStep) >=
                                    barrier.MakeCollectPair(),
                                    "tabletId %" PRIu64 " requested collect %" PRIu32 ":%" PRIu32
                                    " existing barrier %" PRIu32 ":%" PRIu32 " msg# %s",
                                    msg->TabletId, msg->CollectGeneration, msg->CollectStep,
                                    barrier.CollectGeneration, barrier.CollectStep, msg->ToString().c_str());
                        }
                    }

                    // put new barrier
                    Barriers[key] = {msg->RecordGeneration, msg->PerGenerationCounter, msg->CollectGeneration,
                        msg->CollectStep};

                    DoCollection(msg->TabletId, msg->Channel, false, msg->CollectGeneration, msg->CollectStep);
                }
            }

            return new TEvBlobStorage::TEvCollectGarbageResult(NKikimrProto::OK, msg->TabletId,
                    msg->RecordGeneration, msg->PerGenerationCounter, msg->Channel);
        }

    public: // Non-event model interaction methods
        TStorageStatusFlags GetStorageStatusFlags() const noexcept {
            return TStorageStatusFlags(NKikimrBlobStorage::StatusIsValid);
        }

        const TMap<TLogoBlobID, TBlob>& AllMyBlobs() const noexcept {
            return Blobs;
        }

    private:
        // check if provided generation is blocked for specific tablet
        bool IsBlocked(TTabletId tabletId, TGeneration generation) const noexcept {
            auto it = Blocks.find(tabletId);
            return it != Blocks.end() && generation <= it->second;
        }

        // check if provided blob is under garbage collection by barriers
        bool IsCollectedByBarrier(const TLogoBlobID& id) const noexcept {
            auto hardIt = HardBarriers.find(std::make_pair(id.TabletID(), id.Channel()));
            if (hardIt != HardBarriers.end() &&
                    std::make_pair(id.Generation(), id.Step()) <= hardIt->second.MakeCollectPair()) {
                return true;
            }

            auto it = Barriers.find(std::make_pair(id.TabletID(), id.Channel()));
            return it != Barriers.end() &&
                std::make_pair(id.Generation(), id.Step()) <= it->second.MakeCollectPair();
        }

        void DoCollection(ui32 tablet, ui32 channel, bool force, ui32 Gen, ui32 Step) {
            TLogoBlobID id(tablet, 0, 0, channel, 0, 0);
            auto it = Blobs.lower_bound(id);
            while (it != Blobs.end() && it->first.TabletID() == tablet &&
                    it->first.Channel() == channel && std::make_pair(it->first.Generation(),
                    it->first.Step()) <= std::make_pair(Gen, Step)) {
                const auto& data = it->second;
                if (force || data.DoNotKeep || !data.Keep) {
                    it = Blobs.erase(it);
                } else {
                    ++it;
                }

            }
        }
    };
}
}
