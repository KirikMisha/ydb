LIBRARY()

OWNER( 
    cthulhu 
    g:kikimr 
) 

PEERDIR(
    ydb/library/schlab/schine 
    ydb/library/schlab/schoot 
)

SRCS(
    defs.h
    schemu.h
    schemu.cpp
)

END()

RECURSE() 
