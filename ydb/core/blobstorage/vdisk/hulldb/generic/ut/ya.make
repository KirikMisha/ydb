UNITTEST_FOR(ydb/core/blobstorage/vdisk/hulldb/generic) 
 
OWNER(g:kikimr) 
 
FORK_SUBTESTS() 
 
TIMEOUT(600)
 
SIZE(MEDIUM)
 
PEERDIR( 
    library/cpp/getopt
    library/cpp/svnversion
    ydb/core/base 
    ydb/core/blobstorage/vdisk/common 
    ydb/core/blobstorage/vdisk/hulldb 
) 
 
SRCS( 
    hullds_sst_it_all_ut.cpp 
    # FIXME: add file 
    # blobstorage_hullwritesst_ut.cpp 
) 
 
END() 
