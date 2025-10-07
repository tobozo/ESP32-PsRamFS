
set (PFS_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB SRCS ${PFS_ROOT}/src/pfs.c)
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_ADD_INCLUDEDIRS ${PFS_ROOT}/src)
set(COMPONENT_PRIV_REQUIRES esp_psram vfs)
register_component()
