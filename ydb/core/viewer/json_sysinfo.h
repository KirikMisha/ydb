#pragma once
#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/interconnect.h>
#include <library/cpp/actors/core/mon.h>
#include <ydb/core/protos/services.pb.h> 
#include <ydb/core/node_whiteboard/node_whiteboard.h> 
#include "json_wb_req.h"

namespace NKikimr {
namespace NViewer {

template <>
class TWhiteboardMerger<TEvWhiteboard::TEvSystemStateResponse> {
public:
    static THolder<TEvWhiteboard::TEvSystemStateResponse> MergeResponses(TMap<ui32, THolder<TEvWhiteboard::TEvSystemStateResponse>>& responses, const TString&) {
        THolder<TEvWhiteboard::TEvSystemStateResponse> result = MakeHolder<TEvWhiteboard::TEvSystemStateResponse>();
        ui64 minResponseTime = 0;
        auto* field = result->Record.MutableSystemStateInfo();
        field->Reserve(responses.size());
        for (auto it = responses.begin(); it != responses.end(); ++it) {
            if (it->second != nullptr && it->second->Record.SystemStateInfoSize() > 0) {
                auto* element = field->Add();
                element->Swap(it->second->Record.MutableSystemStateInfo(0));
                element->SetNodeId(it->first);
                if (minResponseTime == 0 || it->second->Record.GetResponseTime() < minResponseTime) {
                    minResponseTime = it->second->Record.GetResponseTime();
                }
            }
        }
        result->Record.SetResponseTime(minResponseTime);
        return result;
    }
};

template <>
struct TWhiteboardInfo<TEvWhiteboard::TEvSystemStateResponse> {
    using TResponseType = TEvWhiteboard::TEvSystemStateResponse;
    using TElementType = NKikimrWhiteboard::TSystemStateInfo;

    static constexpr bool StaticNodesOnly = false;

    static ::google::protobuf::RepeatedPtrField<TElementType>* GetElementsField(TResponseType* response) {
        return response->Record.MutableSystemStateInfo();
    }

    static TString GetDefaultMergeField() {
        return "NodeId";
    }

    static THolder<TResponseType> MergeResponses(TMap<ui32, THolder<TResponseType>>& responses, const TString& fields = GetDefaultMergeField()) {
        return TWhiteboardMerger<TResponseType>::MergeResponses(responses, fields);
    }
};

using TJsonSysInfo = TJsonWhiteboardRequest<TEvWhiteboard::TEvSystemStateRequest, TEvWhiteboard::TEvSystemStateResponse>;

template <>
struct TJsonRequestSummary<TJsonSysInfo> {
    static TString GetSummary() {
        return "\"System information\"";
    }
};

template <>
struct TJsonRequestDescription<TJsonSysInfo> {
    static TString GetDescription() {
        return "\"Returns system information\"";
    }
};

}
}
