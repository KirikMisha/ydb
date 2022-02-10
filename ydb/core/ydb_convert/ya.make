LIBRARY()

OWNER(g:kikimr) 

SRCS(
    column_families.cpp
    table_settings.cpp
    table_description.cpp
    ydb_convert.cpp
)

PEERDIR(
    ydb/core/base 
    ydb/core/engine 
    ydb/core/protos 
    ydb/core/scheme 
    ydb/library/binary_json 
    ydb/library/dynumber 
    ydb/library/mkql_proto/protos 
    ydb/library/yql/minikql/dom 
    ydb/library/yql/public/udf 
    ydb/public/api/protos 
)

YQL_LAST_ABI_VERSION() 

END() 

RECURSE_FOR_TESTS( 
    ut 
) 
