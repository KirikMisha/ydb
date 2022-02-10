#include "proxy_private.h"

#include <ydb/core/protos/services.pb.h> 

#include <ydb/library/yql/public/issue/yql_issue_message.h>

#include <library/cpp/yson/node/node_io.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/protobuf/interop/cast.h>

#include <ydb/core/yq/libs/control_plane_storage/events/events.h> 
#include <ydb/core/yq/libs/control_plane_storage/control_plane_storage.h> 

#define LOG_E(stream) \
    LOG_ERROR_S(*TlsActivationContext, NKikimrServices::YQL_PRIVATE_PROXY, "PrivateWriteTask - ResultId: " << ResultId << ", RequestId: " << RequestId << ", " << stream)
#define LOG_D(stream) \
    LOG_DEBUG_S(*TlsActivationContext, NKikimrServices::YQL_PRIVATE_PROXY, "PrivateWriteTask - ResultId: " << ResultId << ", RequestId: " << RequestId << ",  " << stream)

namespace NYq {

using namespace NActors;
using namespace NMonitoring;


class TWriteTaskRequestActor
    : public NActors::TActorBootstrapped<TWriteTaskRequestActor>
{
public:
    TWriteTaskRequestActor(
        const NActors::TActorId& sender,
        TIntrusivePtr<ITimeProvider> timeProvider,
        TAutoPtr<TEvents::TEvWriteTaskResultRequest> ev,
        TDynamicCounterPtr counters)
        : Sender(sender)
        , TimeProvider(timeProvider)
        , Ev(std::move(ev))
        , Counters(std::move(counters->GetSubgroup("subsystem", "private_api")->GetSubgroup("subcomponent", "WriteTaskResult")))
        , LifetimeDuration(Counters->GetHistogram("LifetimeDurationMs",  ExponentialHistogram(10, 2, 50)))
        , RequestedMBytes(Counters->GetHistogram("RequestedMB",  ExponentialHistogram(6, 2, 3)))
        , StartTime(TInstant::Now())

    {}

    static constexpr char ActorName[] = "YQ_PRIVATE_WRITE_RESULT_TASK";

    void OnUndelivered(NActors::TEvents::TEvUndelivered::TPtr& ev, const NActors::TActorContext& ctx) {
        LOG_E("TWriteTaskRequestActor::OnUndelivered");
        Res->Status = Ydb::StatusIds::GENERIC_ERROR;
        Res->Issues.AddIssue("UNDELIVERED");
        ctx.Send(ev->Sender, Res.Release());
        Die(ctx);
    }

    void PassAway() final {
        LifetimeDuration->Collect((TInstant::Now() - StartTime).MilliSeconds());
        NActors::IActor::PassAway();
    }

    void Fail(const TString& message, Ydb::StatusIds::StatusCode reqStatus = Ydb::StatusIds::INTERNAL_ERROR) {
        Issues.AddIssue(message);
        const auto codeStr = Ydb::StatusIds_StatusCode_Name(reqStatus);
        LOG_E(TStringBuilder()
            << "Failed with code: " << codeStr
            << " Details: " << Issues.ToString());
        Res->Status = reqStatus;
        Res->Issues.AddIssues(Issues);
        Send(Sender, Res.Release());
        PassAway();
    }

    void Bootstrap(const TActorContext&) {
        Become(&TWriteTaskRequestActor::StateFunc);
        const auto& req = Ev->Record;

        Deadline = NProtoInterop::CastFromProto(req.deadline());

        const auto& resultSet = req.result_set();
        ResultId = req.result_id().value();
        const auto& resultSetId = req.result_set_id();
        const auto& startRowIndex = req.offset();
        RequestId = req.request_id();
        LOG_D("Request CP::WriteTaskResult with size: " << req.ByteSize() << " bytes");
        RequestedMBytes->Collect(req.ByteSize() / 1024 / 1024);
        Send(NYq::ControlPlaneStorageServiceActorId(),
            new NYq::TEvControlPlaneStorage::TEvWriteResultDataRequest(ResultId, resultSetId, startRowIndex, Deadline, resultSet), 0, RequestId);
    }

private:
    STRICT_STFUNC(
        StateFunc,
        CFunc(NActors::TEvents::TEvPoison::EventType, Die)
        HFunc(NActors::TEvents::TEvUndelivered, OnUndelivered)
        HFunc(NYq::TEvControlPlaneStorage::TEvWriteResultDataResponse, HandleResponse);
    )

    void HandleResponse(NYq::TEvControlPlaneStorage::TEvWriteResultDataResponse::TPtr& ev, const TActorContext& ctx) {
        LOG_D("Got CP::WriteTaskResult Response");
        const auto& issues = ev->Get()->Issues;
        if (issues) {
            Issues.AddIssues(issues);
            Fail("ControlPlane WriteTaskResult Error", Ydb::StatusIds::GENERIC_ERROR);
            return;
        }
        Res->Record.ConstructInPlace();
        Res->Record->set_request_id(RequestId);
        Res->Issues.AddIssues(Issues);
        Res->Status = Ydb::StatusIds::SUCCESS;
        ctx.Send(Sender, Res.Release());
        Die(ctx);
    }

    const TActorId Sender;
    TIntrusivePtr<ITimeProvider> TimeProvider;
    TAutoPtr<TEvents::TEvWriteTaskResultRequest> Ev;
    TDynamicCounterPtr Counters;
    const THistogramPtr LifetimeDuration;
    const THistogramPtr RequestedMBytes;
    const TInstant StartTime;

    TString ResultId;
    ui64 RequestId = 0;
    TInstant Deadline;

    NYql::TIssues Issues;

    THolder<TEvents::TEvWriteTaskResultResponse> Res = MakeHolder<TEvents::TEvWriteTaskResultResponse>();
};

IActor* CreateWriteTaskResultRequestActor(
    const NActors::TActorId& sender,
    TIntrusivePtr<ITimeProvider> timeProvider,
    TAutoPtr<TEvents::TEvWriteTaskResultRequest> ev,
    TDynamicCounterPtr counters) {
    return new TWriteTaskRequestActor(
        sender,
        timeProvider,
        std::move(ev),
        std::move(counters));
}

} /* NYq */
