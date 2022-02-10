#pragma once

#include <ydb/core/protos/pqconfig.pb.h> 
#include <library/cpp/string_utils/base64/base64.h>
#include <util/datetime/base.h>

namespace NKikimr::NDataStreams::V1 {

  class TShardIterator {
  public:
      static constexpr ui64 LIFETIME_MS = 5*60*1000;

      TShardIterator(const TString& iteratorStr) {
          try {
              TString decoded;
              Base64Decode(iteratorStr, decoded);
              Valid = Proto.ParseFromString(decoded) && IsAlive(TInstant::Now().MilliSeconds());
          } catch (std::exception&) {
              Valid = false;
          }
      }

      TShardIterator(const TString& streamName, const TString& streamArn,
                     ui32 shardId, ui64 readTimestamp, ui32 sequenceNumber) {
          Proto.SetStreamName(streamName);
          Proto.SetStreamArn(streamArn);
          Proto.SetShardId(shardId);
          Proto.SetReadTimestampMs(readTimestamp);
          Proto.SetSequenceNumber(sequenceNumber);
          Proto.SetCreationTimestampMs(TInstant::Now().MilliSeconds());
          Valid = true;
      }

      TString Serialize() const {
          TString data;
          bool result = Proto.SerializeToString(&data);
          Y_VERIFY(result);
          TString encoded;
          Base64Encode(data, encoded);
          return encoded;
      }

      TString GetStreamName() const {
          return Proto.GetStreamName();
      }

      TString GetStreamArn() const {
          return Proto.GetStreamArn();
      }

      ui32 GetShardId() const {
          return Proto.GetShardId();
      }

      ui64 GetReadTimestamp() const {
          return Proto.GetReadTimestampMs();
      }

      ui32 GetSequenceNumber() const {
          return Proto.GetSequenceNumber();
      }

      bool IsAlive(ui64 now) const {
          return now >= Proto.GetCreationTimestampMs() && now -
              Proto.GetCreationTimestampMs() < LIFETIME_MS;
      }

      bool IsValid() const {
          return Valid;
      }

  private:
      bool Valid;
      NKikimrPQ::TYdsShardIterator Proto;
  };

} // namespace NKikimr::NDataStreams::V1
