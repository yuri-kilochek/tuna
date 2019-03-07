function(add_resource_library _arl_TARGET _arl_SOURCE _arl_INCLUDE)
    cmake_parse_arguments(PARSE_ARGV 1 _arl
        ""  # zero-value
        "NAME SIZE_NAME"  # one-value
        ""  # multi-value
    )
endfunction()
