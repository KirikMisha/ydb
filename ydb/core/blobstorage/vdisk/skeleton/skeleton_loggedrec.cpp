#include "skeleton_loggedrec.h" 
#include <ydb/core/blobstorage/vdisk/hullop/blobstorage_hull.h> 
#include <ydb/core/blobstorage/vdisk/common/vdisk_response.h> 
#include <ydb/core/blobstorage/vdisk/common/circlebufresize.h> 
 
namespace NKikimr { 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // ILoggedRec -- interface for a record that has been written to the 
    // recovery log. 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    ILoggedRec::ILoggedRec(TLsnSeg seg, bool confirmSyncLogAlso) 
        : Seg(seg) 
        , ConfirmSyncLogAlso(confirmSyncLogAlso) 
    {} 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecVPut -- incapsulates TEvVPut replay action (for small blobs) 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecVPut::TLoggedRecVPut( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            const TLogoBlobID &id, 
            const TIngress &ingress, 
            TRope &&buffer, 
            std::unique_ptr<TEvBlobStorage::TEvVPutResult> result,
            const TActorId &recipient,
            ui64 recipientCookie) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , Id(id) 
        , Ingress(ingress) 
        , Buffer(std::move(buffer)) 
        , Result(std::move(result))
        , Recipient(recipient) 
        , RecipientCookie(recipientCookie) 
    {} 
 
    void TLoggedRecVPut::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        TLogoBlobID genId(Id, 0); 
        hull.AddLogoBlob(ctx, genId, Id.PartId(), Ingress, Buffer, Seg.Point()); 
 
        LOG_DEBUG_S(ctx, NKikimrServices::BS_VDISK_PUT, hull.GetHullCtx()->VCtx->VDiskLogPrefix << "TEvVPut: reply;"
                << " id# " << Id
                << " msg# " << Result->ToString()
                << " Marker# BSVSLR01");
 
        SendVDiskResponse(ctx, Recipient, Result.release(), actor, RecipientCookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecVPut -- incapsulates TEvVPut replay action (for small blobs)
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    TLoggedRecVMultiPutItem::TLoggedRecVMultiPutItem(
            TLsnSeg seg,
            bool confirmSyncLogAlso,
            const TLogoBlobID &id,
            const TIngress &ingress,
            TRope &&buffer,
            std::unique_ptr<TEvVMultiPutItemResult> result,
            const TActorId &recipient,
            ui64 recipientCookie)
        : ILoggedRec(seg, confirmSyncLogAlso)
        , Id(id)
        , Ingress(ingress)
        , Buffer(std::move(buffer))
        , Result(std::move(result))
        , Recipient(recipient)
        , RecipientCookie(recipientCookie)
    {}

    void TLoggedRecVMultiPutItem::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) {
        Y_UNUSED(actor);
        TLogoBlobID genId(Id, 0);
        hull.AddLogoBlob(ctx, genId, Id.PartId(), Ingress, Buffer, Seg.Point());

        LOG_DEBUG_S(ctx, NKikimrServices::BS_VDISK_PUT, hull.GetHullCtx()->VCtx->VDiskLogPrefix
                << "TEvVMultiPut: item reply;"
                << " id# " << Id
                << " msg# " << Result->ToString()
                << " Marker# BSVSLR02");

        ctx.Send(Recipient, Result.release(), RecipientCookie);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // TLoggedRecVPut -- incapsulates TEvVPut replay action (for huge blobs) 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecVPutHuge::TLoggedRecVPutHuge( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            const TActorId &hugeKeeperId,
            TEvHullLogHugeBlob::TPtr ev) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , HugeKeeperId(hugeKeeperId) 
        , Ev(ev) 
    {} 
 
    void TLoggedRecVPutHuge::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        TEvHullLogHugeBlob *msg = Ev->Get(); 
 
        TLogoBlobID genId(msg->LogoBlobID, 0); 
        hull.AddHugeLogoBlob(ctx, genId, msg->Ingress, msg->HugeBlob, Seg.Point()); 
        // notify huge keeper 
        if (msg->HugeBlob != TDiskPart()) {
            ctx.Send(HugeKeeperId, new TEvHullHugeBlobLogged(msg->WriteId, msg->HugeBlob, Seg.Point(), true));
        }
 
        LOG_DEBUG_S(ctx, NKikimrServices::BS_VDISK_PUT, hull.GetHullCtx()->VCtx->VDiskLogPrefix 
                << "TEvVPut: realtime# false result# " << msg->Result->ToString()
                << " Marker# BSVSLR03");
        SendVDiskResponse(ctx, msg->OrigClient, msg->Result.release(), actor, msg->OrigCookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecVBlock -- incapsulates TEvVBlock replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecVBlock::TLoggedRecVBlock( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            ui64 tabletId, 
            ui32 gen, 
            ui64 issuerGuid,
            std::unique_ptr<TEvBlobStorage::TEvVBlockResult> result,
            const TActorId &recipient,
            ui64 recipientCookie) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , TabletId(tabletId) 
        , Gen(gen) 
        , IssuerGuid(issuerGuid)
        , Result(std::move(result))
        , Recipient(recipient) 
        , RecipientCookie(recipientCookie) 
    {} 
 
    void TLoggedRecVBlock::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        auto replySender = [&ctx, &actor] (const TActorId &id, ui64 cookie, IEventBase *msg) {
            SendVDiskResponse(ctx, id, msg, actor, cookie); 
        }; 
 
        hull.AddBlockCmd(ctx, TabletId, Gen, IssuerGuid, Seg.Point(), replySender);
 
        LOG_DEBUG_S(ctx, NKikimrServices::BS_VDISK_BLOCK, hull.GetHullCtx()->VCtx->VDiskLogPrefix 
                << "TEvVBlock: result# " << Result->ToString()
                << " Marker# BSVSLR04");
        SendVDiskResponse(ctx, Recipient, Result.release(), actor, RecipientCookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecVCollectGarbage -- incapsulates TEvVCollectGarbage replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecVCollectGarbage::TLoggedRecVCollectGarbage( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            TBarrierIngress ingress, 
            std::unique_ptr<TEvBlobStorage::TEvVCollectGarbageResult> result,
            TEvBlobStorage::TEvVCollectGarbage::TPtr origEv) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , Ingress(ingress) 
        , Result(std::move(result))
        , OrigEv(origEv) 
    {} 
 
    void TLoggedRecVCollectGarbage::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        NKikimrBlobStorage::TEvVCollectGarbage &record = OrigEv->Get()->Record; 
        hull.AddGCCmd(ctx, record, Ingress, Seg); 
 
        LOG_DEBUG_S(ctx, NKikimrServices::BS_VDISK_GC, hull.GetHullCtx()->VCtx->VDiskLogPrefix 
                << "TEvVCollectGarbage: result# " << Result->ToString()
                << " Marker# BSVSLR05");
        SendVDiskResponse(ctx, OrigEv->Sender, Result.release(), actor, OrigEv->Cookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecLocalSyncData -- incapsulates TEvLocalSyncData replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecLocalSyncData::TLoggedRecLocalSyncData( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            std::unique_ptr<TEvLocalSyncDataResult> result,
            TEvLocalSyncData::TPtr origEv) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , Result(std::move(result))
        , OrigEv(origEv) 
    {} 
 
    void TLoggedRecLocalSyncData::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        auto replySender = [&ctx, &actor] (const TActorId &id, ui64 cookie, IEventBase *msg) {
            SendVDiskResponse(ctx, id, msg, actor, cookie); 
        }; 
 
#ifdef UNPACK_LOCALSYNCDATA 
        hull.AddSyncDataCmd(ctx, std::move(OrigEv->Get()->Extracted), Seg, replySender); 
#else 
        hull.AddSyncDataCmd(ctx, OrigEv->Get()->Data, Seg, replySender); 
#endif 
        SendVDiskResponse(ctx, OrigEv->Sender, Result.release(), actor, OrigEv->Cookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecAnubisOsirisPut -- incapsulates TEvAnubisOsirisPut replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecAnubisOsirisPut::TLoggedRecAnubisOsirisPut( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            const TEvAnubisOsirisPut::THullDbInsert &insert, 
            std::unique_ptr<TEvAnubisOsirisPutResult> result,
            TEvAnubisOsirisPut::TPtr origEv) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , Insert(insert) 
        , Result(std::move(result))
        , OrigEv(origEv) 
    {} 
 
    void TLoggedRecAnubisOsirisPut::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        hull.AddAnubisOsirisLogoBlob(ctx, Insert.Id, Insert.Ingress, Seg); 
        SendVDiskResponse(ctx, OrigEv->Sender, Result.release(), actor, OrigEv->Cookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecPhantoms -- incapsulates TEvDetectedPhantomBlob replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecPhantoms::TLoggedRecPhantoms( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            TEvDetectedPhantomBlob::TPtr origEv) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , OrigEv(origEv) 
    {} 
 
    void TLoggedRecPhantoms::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        Y_UNUSED(actor); 
        TEvDetectedPhantomBlob *msg = OrigEv->Get(); 
        hull.CollectPhantoms(ctx, msg->Phantoms, Seg); 
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecDelLogoBlobDataSyncLog -- incapsulates TEvDelLogoBlobDataSyncLog replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecDelLogoBlobDataSyncLog::TLoggedRecDelLogoBlobDataSyncLog( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            std::unique_ptr<TEvDelLogoBlobDataSyncLogResult> result,
            const TActorId &recipient,
            ui64 recipientCookie) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , Result(std::move(result))
        , Recipient(recipient) 
        , RecipientCookie(recipientCookie) 
    {} 
 
    void TLoggedRecDelLogoBlobDataSyncLog::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        Y_UNUSED(hull); 
        SendVDiskResponse(ctx, Recipient, Result.release(), actor, RecipientCookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecAddBulkSst -- incapsulates TEvAddBulkSst replay action 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    TLoggedRecAddBulkSst::TLoggedRecAddBulkSst( 
            TLsnSeg seg, 
            bool confirmSyncLogAlso, 
            TEvAddBulkSst::TPtr ev) 
        : ILoggedRec(seg, confirmSyncLogAlso) 
        , OrigEv(ev) 
    {} 
 
    void TLoggedRecAddBulkSst::Replay(THull &hull, const TActorContext &ctx, const IActor& actor) { 
        hull.AddBulkSst(ctx, OrigEv->Get()->Essence, Seg); 
        SendVDiskResponse(ctx, OrigEv->Sender, new TEvAddBulkSstResult, actor, OrigEv->Cookie);
    } 
 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    // TLoggedRecsVault -- cookie manager for LoggedRecs that are in flight 
    /////////////////////////////////////////////////////////////////////////////////////////////////////// 
    class TLoggedRecsVault::TImpl { 
    public: 
        TImpl() 
            : Queue(64) 
        {} 
 
        ~TImpl() { 
            while (!Queue.Empty()) { 
                delete Queue.Top().second; 
                Queue.Top().second = nullptr; 
                Queue.Pop(); 
            } 
        } 
 
        intptr_t Put(ILoggedRec *rec) { 
            Y_VERIFY_DEBUG(rec); 
            intptr_t id = ++Counter; 
            Queue.Push(TItem(id, rec)); 
            return id; 
        } 
 
        ILoggedRec *Extract(intptr_t id) { 
            Y_VERIFY((id == Extracted + 1) && !Queue.Empty(), "id# %" PRIu64 " Extracted# %" PRIu64, id, Extracted); 
            Extracted = id; 
 
            TItem item = Queue.Top(); 
            Queue.Pop(); 
            Y_VERIFY(item.first == id); 
            return item.second; 
        } 
 
        std::optional<ui64> GetLastLsnInFlight() const { 
            return Queue.Empty() ? std::optional<ui64>() : std::optional<ui64>(Queue.Back().second->Seg.Last); 
        } 
 
    private: 
        using TItem = std::pair<intptr_t, ILoggedRec*>; 
        using TQueue = TAllocFreeQueue<TItem>; 
        TQueue Queue; 
        intptr_t Counter = 0; 
        intptr_t Extracted = 0; 
    }; 
 
    TLoggedRecsVault::TLoggedRecsVault() 
        : Impl(new TImpl) 
    {} 
 
    TLoggedRecsVault::~TLoggedRecsVault() = default; 
 
    intptr_t TLoggedRecsVault::Put(ILoggedRec *rec) { 
        return Impl->Put(rec); 
    } 
 
    ILoggedRec *TLoggedRecsVault::Extract(intptr_t id) { 
        return Impl->Extract(id); 
    } 
 
    std::optional<ui64> TLoggedRecsVault::GetLastLsnInFlight() const { 
        return Impl->GetLastLsnInFlight(); 
    } 
 
} // NKikimr 
 
