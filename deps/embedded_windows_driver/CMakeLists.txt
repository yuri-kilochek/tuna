add_subdirectory(../embedded_file ../embedded_file)

set(_EMBEDDED_DRIVER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE INTERNAL ""    
)

function(embed_driver TARGET VAR INF_PATH CAT_PATH SYS_PATH)
    # TODO
endfunction()
