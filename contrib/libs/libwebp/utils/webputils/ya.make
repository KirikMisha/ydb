# Generated by devtools/yamaker.

LIBRARY()

WITHOUT_LICENSE_TEXTS()

LICENSE(BSD-3-Clause)

PEERDIR(
    contrib/libs/libwebp/utils/webputilsdecode
)

ADDINCL(
    contrib/libs/libwebp/utils
    contrib/libs/libwebp/webp
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DHAVE_CONFIG_H
)

SRCDIR(contrib/libs/libwebp/utils)

SRCS(
    bit_writer_utils.c
    huffman_encode_utils.c
    quant_levels_utils.c
)

END()