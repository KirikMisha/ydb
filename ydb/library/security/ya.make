LIBRARY()

OWNER( 
    hcpp 
    g:kikimr 
) 

PEERDIR(
    library/cpp/digest/crc32c
    ydb/public/sdk/cpp/client/ydb_types/credentials 
)

SRCS(
    util.cpp
    ydb_credentials_provider_factory.cpp
)

END()

RECURSE_FOR_TESTS( 
    ut
)
