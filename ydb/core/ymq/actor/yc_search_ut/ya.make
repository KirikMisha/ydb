OWNER(
    komels
    g:kikimr 
    g:sqs
)

UNITTEST()

PEERDIR(
    ydb/core/testlib 
    ydb/core/ymq/actor 
)

SRCS(
    index_events_processor_ut.cpp
    test_events_writer.cpp
)
 
SIZE(MEDIUM)
 
TIMEOUT(300)
 
YQL_LAST_ABI_VERSION()

END()
