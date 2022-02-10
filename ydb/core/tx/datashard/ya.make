LIBRARY()

OWNER(
    ddoarn
    g:kikimr
)

SRCS(
    alter_cdc_stream_unit.cpp
    alter_table_unit.cpp
    backup_unit.cpp
    build_and_wait_dependencies_unit.cpp
    build_data_tx_out_rs_unit.cpp
    build_distributed_erase_tx_out_rs_unit.cpp
    build_kqp_data_tx_out_rs_unit.cpp
    build_scheme_tx_out_rs_unit.cpp
    change_collector_async_index.cpp
    change_collector_base.cpp
    change_collector_cdc_stream.cpp
    change_collector_helpers.cpp
    change_collector.cpp
    change_exchange.cpp
    change_exchange_split.cpp
    change_record.cpp
    change_sender.cpp
    change_sender_async_index.cpp
    change_sender_cdc_stream.cpp
    change_sender_common_ops.cpp
    check_commit_writes_tx_unit.cpp
    check_data_tx_unit.cpp
    check_distributed_erase_tx_unit.cpp
    check_scheme_tx_unit.cpp
    check_snapshot_tx_unit.cpp
    complete_data_tx_unit.cpp
    completed_operations_unit.cpp
    create_cdc_stream_unit.cpp
    create_persistent_snapshot_unit.cpp
    create_table_unit.cpp
    create_volatile_snapshot_unit.cpp
    datashard__build_index.cpp
    datashard__cancel_tx_proposal.cpp
    datashard__compact_borrowed.cpp
    datashard__compaction.cpp
    datashard__cleanup_borrowed.cpp
    datashard__cleanup_in_rs.cpp
    datashard__cleanup_tx.cpp
    datashard__conditional_erase_rows.cpp
    datashard__engine_host.cpp
    datashard__engine_host.h
    datashard__get_state_tx.cpp
    datashard__schema_changed.cpp
    datashard__migrate_schemeshard.cpp
    datashard__init.cpp
    datashard__monitoring.cpp
    datashard__mon_reset_schema_version.cpp
    datashard__op_rows.cpp
    datashard__plan_step.cpp
    datashard__progress_resend_rs.cpp
    datashard__progress_tx.cpp
    datashard__propose_tx_base.cpp
    datashard__readset.cpp
    datashard__read_iterator.cpp
    datashard__read_columns.cpp
    datashard__s3.cpp
    datashard__s3_download_txs.cpp
    datashard__s3_upload_txs.cpp
    datashard__kqp_scan.cpp
    datashard__snapshot_txs.cpp
    datashard__stats.cpp
    datashard__store_table_path.cpp
    datashard__store_scan_state.cpp
    datashard_change_receiving.cpp
    datashard_change_sender_activation.cpp
    datashard_change_sending.cpp
    datashard_counters.cpp
    datashard_loans.cpp
    datashard_locks.h
    datashard_locks.cpp
    datashard_split_dst.cpp
    datashard_split_src.cpp
    datashard_switch_mvcc_state.cpp
    datashard_trans_queue.cpp
    datashard_trans_queue.h
    datashard_outreadset.cpp
    datashard_outreadset.h
    datashard_active_transaction.cpp
    datashard_active_transaction.h
    datashard_common_upload.cpp
    datashard_direct_transaction.cpp
    datashard_direct_transaction.h
    datashard_direct_erase.cpp
    datashard_direct_upload.cpp
    datashard_distributed_erase.cpp
    datashard_failpoints.cpp
    datashard_failpoints.h
    datashard_dep_tracker.cpp
    datashard_dep_tracker.h
    datashard_pipeline.cpp
    datashard_pipeline.h
    datashard_s3_downloads.cpp
    datashard_s3_uploads.cpp
    datashard_schema_snapshots.cpp
    datashard_snapshots.cpp
    datashard_unsafe_upload.cpp
    datashard_user_table.cpp
    datashard_user_table.h
    datashard_impl.h
    datashard_kqp_compute.cpp
    datashard_kqp_compute.h
    datashard_kqp_effects.cpp
    datashard_kqp_lookup_table.cpp
    datashard_kqp_read_table.cpp
    datashard_kqp_upsert_rows.cpp
    datashard_kqp_delete_rows.cpp
    datashard_kqp.cpp
    datashard_kqp.h
    datashard_repl_apply.cpp
    datashard_repl_offsets.cpp
    datashard_repl_offsets_client.cpp
    datashard_repl_offsets_server.cpp
    datashard_subdomain_path_id.cpp
    datashard_txs.h
    datashard.cpp
    datashard.h
    defs.h
    direct_tx_unit.cpp
    drop_cdc_stream_unit.cpp
    drop_index_notice_unit.cpp
    drop_persistent_snapshot_unit.cpp
    drop_table_unit.cpp
    drop_volatile_snapshot_unit.cpp
    erase_rows_condition.cpp
    execute_commit_writes_tx_unit.cpp
    execute_data_tx_unit.cpp
    execute_distributed_erase_tx_unit.cpp
    execute_kqp_data_tx_unit.cpp
    execute_kqp_scan_tx_unit.cpp
    execution_unit.cpp
    execution_unit.h
    execution_unit_ctors.h
    execution_unit_kind.h
    export_common.cpp
    export_iface.cpp 
    export_iface.h 
    export_scan.cpp
    finalize_build_index_unit.cpp
    finish_propose_unit.cpp
    initiate_build_index_unit.cpp
    key_conflicts.cpp
    key_conflicts.h
    load_and_wait_in_rs_unit.cpp
    load_tx_details_unit.cpp
    make_scan_snapshot_unit.cpp
    make_snapshot_unit.cpp
    move_table_unit.cpp
    operation.cpp
    operation.h
    plan_queue_unit.cpp
    prepare_data_tx_in_rs_unit.cpp
    prepare_distributed_erase_tx_in_rs_unit.cpp
    prepare_kqp_data_tx_in_rs_unit.cpp
    prepare_scheme_tx_in_rs_unit.cpp
    progress_queue.h
    protect_scheme_echoes_unit.cpp
    read_table_scan.h
    read_table_scan.cpp
    read_table_scan_unit.cpp
    receive_snapshot_unit.cpp
    range_avl_tree.cpp
    range_ops.cpp
    range_treap.cpp
    read_iterator.h
    restore_unit.cpp
    setup_sys_locks.h
    store_and_send_out_rs_unit.cpp
    store_commit_writes_tx_unit.cpp
    store_data_tx_unit.cpp
    store_distributed_erase_tx_unit.cpp
    store_scheme_tx_unit.cpp
    store_snapshot_tx_unit.cpp
    wait_for_plan_unit.cpp
    wait_for_stream_clearance_unit.cpp
    upload_stats.cpp
)

GENERATE_ENUM_SERIALIZATION(change_exchange.h)
 
GENERATE_ENUM_SERIALIZATION(change_record.h)
 
GENERATE_ENUM_SERIALIZATION(datashard.h)
 
GENERATE_ENUM_SERIALIZATION(datashard_active_transaction.h)
 
GENERATE_ENUM_SERIALIZATION(datashard_s3_upload.h)
 
GENERATE_ENUM_SERIALIZATION(execution_unit.h)
 
GENERATE_ENUM_SERIALIZATION(execution_unit_kind.h)
 
GENERATE_ENUM_SERIALIZATION(operation.h)

RESOURCE(
    index.html datashard/index.html
)

PEERDIR(
    library/cpp/actors/core
    library/cpp/containers/flat_hash
    library/cpp/html/pcdata
    library/cpp/string_utils/quote
    ydb/core/actorlib_impl 
    ydb/core/base 
    ydb/core/engine 
    ydb/core/engine/minikql 
    ydb/core/formats 
    ydb/core/io_formats 
    ydb/core/kqp/runtime 
    ydb/core/persqueue/partition_key_range 
    ydb/core/persqueue/writer 
    ydb/core/protos 
    ydb/core/tablet 
    ydb/core/tablet_flat 
    ydb/core/util 
    ydb/core/wrappers 
    ydb/core/ydb_convert 
    ydb/library/aclib 
    ydb/library/binary_json 
    ydb/library/dynumber 
    ydb/public/api/protos 
    ydb/public/lib/deprecated/kicli 
    ydb/library/yql/dq/actors/compute
)

YQL_LAST_ABI_VERSION() 

IF (OS_WINDOWS)
    CFLAGS( 
        -DKIKIMR_DISABLE_S3_OPS 
    ) 
ELSE()
    SRCS(
        export_s3_buffer.cpp
        export_s3_uploader.cpp
        import_s3.cpp
    )
ENDIF()

END()
 
RECURSE_FOR_TESTS( 
    ut_background_compaction
    ut_build_index
    ut_change_collector
    ut_change_exchange
    ut_compaction
    ut_erase_rows
    ut_followers
    ut_init
    ut_keys
    ut_kqp
    ut_kqp_errors
    ut_kqp_scan
    ut_locks
    ut_minikql
    ut_minstep
    ut_order
    ut_range_avl_tree
    ut_range_ops
    ut_range_treap
    ut_read_iterator
    ut_read_table
    ut_reassign
    ut_replication
    ut_rs
    ut_snapshot
    ut_stats
    ut_upload_rows
) 
