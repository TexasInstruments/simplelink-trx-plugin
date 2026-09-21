cmake_minimum_required(VERSION ${TI_MIN_CMAKE_VERSION})

# Common list of sources in all maclib variants
set(SOURCES_TRXHOST
    trx_host.c
    handlers/trx_host_handlers.c
    private/trx_host_private.c
    transport/spi_transport.c
    utility/list/list.c
    utility/buffer_manager/buffer_manager.c
    utility/utils.c
)
