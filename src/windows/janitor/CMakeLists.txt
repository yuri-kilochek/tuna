include(Windows/EmbedFile)

add_executable(_tuna_janitor EXCLUDE_FROM_ALL)

set_target_properties(_tuna_janitor PROPERTIES OUTPUT_NAME tuna_janitor)

target_sources(_tuna_janitor PRIVATE main.c)

function(_tuna_embed_janitor TARGET)
    _tuna_embed_file("${TARGET}" tuna_priv_embedded_janitor
        "$<TARGET_FILE:_tuna_janitor>"
    )
endfunction()
