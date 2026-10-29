# Warning policy.
#
# All the code (src/, tests/, tools/) is built with a strict warning set and,
# by default, warnings as errors (the 2002 code, built with the compiler's
# default warnings, is gone since Phase 5).

function(bb_set_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        # fopen() and friends are standard and portable; MSVC's "secure"
        # replacements (C4996) are not.
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        if(BB_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual)
        if(BB_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
