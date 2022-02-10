LIBRARY()

OWNER(
    g:kikimr 
    g:logbroker
)

SRCS(
    codecs.h
    codecs.cpp
)

PEERDIR(
    library/cpp/streams/zstd
    ydb/library/yql/public/issue/protos 
    ydb/public/api/grpc/draft 
    ydb/public/api/protos 
)

PROVIDES(pq_codecs_2)

END()
