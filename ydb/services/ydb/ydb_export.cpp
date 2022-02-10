#include "ydb_export.h"

#include <ydb/core/grpc_services/grpc_helper.h> 
#include <ydb/core/grpc_services/grpc_request_proxy.h> 
#include <ydb/core/grpc_services/rpc_calls.h> 

namespace NKikimr {
namespace NGRpcService {

TGRpcYdbExportService::TGRpcYdbExportService(NActors::TActorSystem *system, TIntrusivePtr<NMonitoring::TDynamicCounters> counters, NActors::TActorId id)
    : ActorSystem_(system)
    , Counters_(counters)
    , GRpcRequestProxyId_(id)
{ }

void TGRpcYdbExportService::InitService(grpc::ServerCompletionQueue *cq, NGrpc::TLoggerPtr logger) {
    CQ_ = cq;
    SetupIncomingRequests(std::move(logger));
}

void TGRpcYdbExportService::SetGlobalLimiterHandle(NGrpc::TGlobalLimiter* limiter) {
    Limiter_ = limiter;
}

bool TGRpcYdbExportService::IncRequest() {
    return Limiter_->Inc();
}

void TGRpcYdbExportService::DecRequest() {
    Limiter_->Dec();
    Y_ASSERT(Limiter_->GetCurrentInFlight() >= 0);
}

void TGRpcYdbExportService::SetupIncomingRequests(NGrpc::TLoggerPtr logger) {
    auto getCounterBlock = CreateCounterCb(Counters_, ActorSystem_);

#ifdef ADD_REQUEST
#error ADD_REQUEST macro already defined
#endif
#define ADD_REQUEST(NAME, IN, OUT, ACTION) \
    MakeIntrusive<TGRpcRequest<Ydb::Export::IN, Ydb::Export::OUT, TGRpcYdbExportService>>(this, &Service_, CQ_, \
        [this](NGrpc::IRequestContextBase *ctx) { \
            NGRpcService::ReportGrpcReqToMon(*ActorSystem_, ctx->GetPeer()); \
            ACTION; \
        }, &Ydb::Export::V1::ExportService::AsyncService::Request ## NAME, \
        #NAME, logger, getCounterBlock("export", #NAME))->Run();

    ADD_REQUEST(ExportToYt, ExportToYtRequest, ExportToYtResponse, {
        ActorSystem_->Send(GRpcRequestProxyId_, new TEvExportToYtRequest(ctx));
    })
    ADD_REQUEST(ExportToS3, ExportToS3Request, ExportToS3Response, {
        ActorSystem_->Send(GRpcRequestProxyId_, new TEvExportToS3Request(ctx));
    })
#undef ADD_REQUEST
}

} // namespace NGRpcService
} // namespace NKikimr
