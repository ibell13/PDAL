#
# ZLIB support
#
option(WITH_ZLIB
    "Build support for compression/decompression with zlib/deflate." TRUE)
if (WITH_ZLIB)
    find_package(ZLIB REQUIRED)
    set_package_properties(ZLIB PROPERTIES TYPE REQUIRED
        PURPOSE "Compression support in BPF")
    if(ZLIB_FOUND)
        set(PDAL_HAVE_ZLIB 1)
    endif(ZLIB_FOUND)
endif(WITH_ZLIB)
