LIBRARY()

OWNER(
    g:kikimr 
    g:logbroker
)

SRCS(
    read_session_messages.cpp
    common.cpp
    write_session.cpp
    read_session.cpp
    persqueue.cpp
    persqueue_impl.cpp
)

PEERDIR(
    library/cpp/containers/disjoint_interval_tree
    library/cpp/grpc/client
    library/cpp/monlib/dynamic_counters 
    library/cpp/string_utils/url
    ydb/library/persqueue/obfuscate 
    ydb/public/api/grpc/draft 
    ydb/public/sdk/cpp/client/impl/ydb_internal/make_request 
    ydb/public/sdk/cpp/client/ydb_common_client/impl 
    ydb/public/sdk/cpp/client/ydb_driver 
)

END()
