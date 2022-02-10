LIBRARY()

OWNER( 
    g:yql 
    g:yql_ydb_core 
) 

SRCS(
    yql_expr_schema.cpp
)

PEERDIR(
    library/cpp/yson 
    library/cpp/yson/node 
    ydb/library/yql/ast
    ydb/library/yql/public/issue
    ydb/library/yql/public/udf 
    ydb/library/yql/utils
    ydb/library/yql/providers/common/schema/parser
)

END()
