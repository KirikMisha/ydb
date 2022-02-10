#include "proxy_private.h"

#include <ydb/core/protos/services.pb.h> 
#include <ydb/library/yql/public/issue/yql_issue_message.h>

#include <library/cpp/yson/node/node_io.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/log.h>

#include <ydb/core/yq/libs/control_plane_storage/control_plane_storage.h> 
#include <ydb/core/yq/libs/control_plane_storage/events/events.h> 
#include <google/protobuf/util/time_util.h>

#define LOG_E(stream) \
    LOG_ERROR_S(*TlsActivationContext, NKikimrServices::YQL_NODES_MANAGER, stream)
#define LOG_D(stream) \
    LOG_DEBUG_S(*TlsActivationContext, NKikimrServices::YQL_NODES_MANAGER, stream)

namespace NYq {

using namespace NActors;
using namespace NYql;
using namespace NMonitoring;

class TNodesHealthCheckActor
    : public NActors::TActorBootstrapped<TNodesHealthCheckActor>
{
public:
    TNodesHealthCheckActor(
        const NActors::TActorId& sender,
        TIntrusivePtr<ITimeProvider> timeProvider,
        TAutoPtr<TEvents::TEvNodesHealthCheckRequest> ev,
        TDynamicCounterPtr counters)
        : Sender(sender)
        , TimeProvider(timeProvider)
        , Ev(std::move(ev))
        , Counters(std::move(counters))
        , LifetimeDuration(Counters->GetSubgroup("subsystem", "private_api")->GetHistogram("NodesHealthCheckTask",  ExponentialHistogram(10, 2, 50)))
        , StartTime(TInstant::Now())
    {}

    static constexpr char ActorName[] = "YQ_PRIVATE_NODES_HEALTH_CHECK";

    void OnUndelivered(NActors::TEvents::TEvUndelivered::TPtr& ev, const NActors::TActorContext& ctx) {
        LOG_E("TNodesHealthCheckActor::OnUndelivered");
        auto res = MakeHolder<TEvents::TEvNodesHealthCheckResponse>();
        res->Status = Ydb::StatusIds::GENERIC_ERROR;
        res->Issues.AddIssue("UNDELIVERED");
        ctx.Send(ev->Sender, res.Release());
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
        auto res = MakeHolder<TEvents::TEvNodesHealthCheckResponse>();
        res->Status = reqStatus;
        res->Issues.AddIssues(Issues);
        Send(Sender, res.Release());
        PassAway();
    }

    void Bootstrap(const TActorContext&) {
        Become(&TNodesHealthCheckActor::StateFunc);
        auto& req = Ev->Record;
        Tenant = req.tenant();

        Send(NYq::ControlPlaneStorageServiceActorId(),
            new NYq::TEvControlPlaneStorage::TEvNodesHealthCheckRequest(std::move(req)));
    }

private:
    STRICT_STFUNC(
        StateFunc,
        CFunc(NActors::TEvents::TEvPoison::EventType, Die)
        HFunc(NYq::TEvControlPlaneStorage::TEvNodesHealthCheckResponse, HandleResponse)
        HFunc(NActors::TEvents::TEvUndelivered, OnUndelivered)
    )

    void HandleResponse(NYq::TEvControlPlaneStorage::TEvNodesHealthCheckResponse::TPtr& ev, const TActorContext& ctx) {
        auto res = MakeHolder<TEvents::TEvNodesHealthCheckResponse>();
        try {
            const auto& issues = ev->Get()->Issues;
            if (issues) {
                ythrow yexception() << issues.ToString();
            }
            res->Record.ConstructInPlace();
            res->Status = Ydb::StatusIds::SUCCESS;
            res->Record = ev->Get()->Record;
            ctx.Send(Sender, res.Release());
            Die(ctx);
        } catch (...) {
            const auto msg = TStringBuilder() << "Can't do NodesHealthCheck: " << CurrentExceptionMessage();
            Fail(msg);
        }
    }

private:
    const TActorId Sender;
    TIntrusivePtr<ITimeProvider> TimeProvider;
    TAutoPtr<TEvents::TEvNodesHealthCheckRequest> Ev;
    TDynamicCounterPtr Counters;
    const THistogramPtr LifetimeDuration;
    const TInstant StartTime;

    NYql::TIssues Issues;
    TString Tenant;
};

IActor* CreateNodesHealthCheckActor(
    const NActors::TActorId& sender,
    TIntrusivePtr<ITimeProvider> timeProvider,
    TAutoPtr<TEvents::TEvNodesHealthCheckRequest> ev,
    TDynamicCounterPtr counters) {
    return new TNodesHealthCheckActor(
        sender,
        timeProvider,
        std::move(ev),
        std::move(counters));
}

} /* NYq */
