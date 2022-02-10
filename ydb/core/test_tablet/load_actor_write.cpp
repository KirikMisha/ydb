#include "load_actor_impl.h"

#include <ydb/core/util/lz4_data_generator.h>

namespace NKikimr::NTestShard {

    void TLoadActor::GenerateKeyValue(TString *key, TString *value, bool *isInline) {
        const size_t len = GenerateRandomSize(Settings.GetSizes(), isInline) + sizeof(ui64);
        ui64 seed = TAppData::RandomProvider->GenRand64();
        TString data = FastGenDataForLZ4(len, seed);
        char *charData = data.Detach();
        for (size_t i = 0; i < Min<size_t>(sizeof(seed), data.size()); ++i) {
            charData[i] = *(reinterpret_cast<char*>(&seed) + i);
        }
        *key = MD5::Calc(data);
        *value = std::move(data);
    }

    void TLoadActor::IssueWrite() {
        TString key, value;
        bool isInline;
        do {
            GenerateKeyValue(&key, &value, &isInline);
        } while (Keys.count(key));

        auto ev = CreateRequest();
        auto& r = ev->Record;
        auto *write = r.AddCmdWrite();
        write->SetKey(key);
        write->SetValue(value);
        if (isInline) {
            write->SetStorageChannel(NKikimrClient::TKeyValueRequest::INLINE);
        }

        STLOG(PRI_INFO, TEST_SHARD, TS12, "writing data", (TabletId, TabletId), (Key, key), (Size, value.size()));

        auto [wifIt, wifInserted] = WritesInFlight.try_emplace(r.GetCookie(), key);
        Y_VERIFY(wifInserted);
        Y_VERIFY(wifIt->second.KeysInQuery.size() == 1);

        auto [it, inserted] = Keys.try_emplace(key, value.size());
        Y_VERIFY(inserted);
        RegisterTransition(*it, ::NTestShard::TStateServer::ABSENT, ::NTestShard::TStateServer::WRITE_PENDING, std::move(ev));

        ++KeysWritten;
        BytesProcessed += value.size();
    }

    void TLoadActor::ProcessWriteResult(ui64 cookie, const google::protobuf::RepeatedPtrField<NKikimrClient::TKeyValueResponse::TWriteResult>& results) {
        if (const auto wifIt = WritesInFlight.find(cookie); wifIt != WritesInFlight.end()) {
            TWriteInfo& info = wifIt->second;
            const TDuration latency = TDuration::Seconds(info.Timer.Passed());
            WriteLatency.Add(latency);
            Y_VERIFY(info.KeysInQuery.size() == (size_t)results.size(), "%zu/%d", info.KeysInQuery.size(), results.size());
            for (size_t i = 0; i < info.KeysInQuery.size(); ++i) {
                const auto& res = results[i];
                Y_VERIFY_S(res.GetStatus() == NKikimrProto::OK, "TabletId# " << TabletId << " CmdWrite failed Status# "
                    << NKikimrProto::EReplyStatus_Name(NKikimrProto::EReplyStatus(res.GetStatus())));

                const auto it = Keys.find(info.KeysInQuery[i]);
                Y_VERIFY_S(it != Keys.end(), "Key# " << info.KeysInQuery[i] << " not found in Keys dict");
                TKeyInfo& k = it->second;
                WriteSpeed.Add(TActivationContext::Now(), k.Len);

                RegisterTransition(*it, ::NTestShard::TStateServer::WRITE_PENDING, ::NTestShard::TStateServer::CONFIRMED);
            }
            WritesInFlight.erase(wifIt);
        }
    }

} // NKikimr::NTestShard
