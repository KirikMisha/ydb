LIBRARY()

LICENSE(
    BSD-3-Clause AND
    GPL-3.0-or-later
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

IF (NOT MUSL)
    NO_RUNTIME()
ENDIF()

NO_COMPILER_WARNINGS()

ADDINCL(
    GLOBAL contrib/tools/bison/lib
)

IF (OS_WINDOWS)
    ADDINCL(
        GLOBAL contrib/tools/bison/lib/platform/win64
    )
ELSE()
    ADDINCL(
        GLOBAL contrib/tools/bison/lib/platform/posix
    )
ENDIF()

IF (OS_DARWIN)
    CFLAGS(
        -D_XOPEN_SOURCE=600
    )
ENDIF()

IF (NOT OS_WINDOWS)
    CFLAGS(
        GLOBAL -Dregcomp=gnu_regcomp
        GLOBAL -Dregerror=gnu_regerror
        GLOBAL -Dregfree=gnu_regfree
        GLOBAL -Dregexec=gnu_regexec
    )
ENDIF()

SRCS(
    abitset.c
    argmatch.c
    basename-lgpl.c
    basename.c
    binary-io.c
    bitrotate.c
    bitset.c
    bitset_stats.c
    bitsetv-print.c
    bitsetv.c
    c-ctype.c
    c-strcasecmp.c
    c-strncasecmp.c
    cloexec.c
    close.c
    close-stream.c
    closeout.c
    concat-filename.c
    dirname-lgpl.c
    dirname.c
    dup-safer-flag.c
    dup-safer.c
    dup2.c
    ebitset.c
    error.c
    exitfail.c
    fatal-signal.c
    fcntl.c
    fd-hook.c
    fd-safer-flag.c
    fd-safer.c
    fopen-safer.c
    fstat.c
    get-errno.c
    getdtablesize.c
    getopt.c
    getopt1.c
    hash.c
    itold.c
    isnanl.c
    isnand.c
    lbitset.c
    localcharset.c
    lseek.c
    lstat.c
    malloc.c
    malloca.c
    mbswidth.c
    mbrtowc.c
    mkstemp-safer.c
    nl_langinfo.c
    pipe-safer.c
    pipe2-safer.c
    printf-args.c
    printf-frexp.c
    printf-frexpl.c
    printf-parse.c
    progname.c
    quotearg.c
    raise.c
    rawmemchr.c
    readlink.c
    rename.c
    rmdir.c
    secure_getenv.c
    sig-handler.c
    spawn-pipe.c
    stat.c
    stpcpy.c
    strchrnul.c
    strdup.c
    stripslash.c
    tempname.c
    timevar.c
    unistd.c
    vasnprintf.c
    vbitset.c
    wait-process.c
    wctype-h.c
    xalloc-die.c
    xconcat-filename.c
    xmalloc.c
    xmalloca.c
    xmemdup0.c
    xsize.c
    xstrndup.c
    xvasprintf.c
)

IF (NOT MUSL)
    SRCS(
        fseterr.c
    )
ENDIF()

IF (NOT OS_LINUX)
    SRCS(
        pipe2.c
        strverscmp.c
    )
ENDIF()

IF (NOT OS_WINDOWS)
    SRCS(
        stdio-write.c
    )
ENDIF()

IF (OS_WINDOWS)
    SRCS(
        frexp.c
        wcrtomb.c
        perror.c
        strstr.c
        mkstemp.c
        strsignal.c
        mkdtemp.c
        fseeko.c
        fopen.c
        ftello.c
        localeconv.c
        msvc-inval.c
        msvc-nothrow.c
        open.c
        sigaction.c
        sigprocmask.c
        snprintf.c
        strndup.c
        waitpid.c
        wcwidth.c
        uniwidth/width.c
    )
ENDIF()

IF (NOT OS_LINUX OR MUSL)
    SRCS(
        obstack.c
        obstack_printf.c
    )
ENDIF()

IF (OS_CYGWIN OR OS_LINUX)
    #not need it
ELSE()
    SRCS(
        fpending.c
    )
ENDIF()

END()
