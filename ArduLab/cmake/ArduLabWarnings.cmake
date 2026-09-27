# ArduLab warning policy.
# Applied to every module target so that all modules compile under the same
# strictness. Warnings become errors only when ARDULAB_WARNINGS_AS_ERRORS=ON.

function(ardulab_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /utf-8
            $<$<BOOL:${ARDULAB_WARNINGS_AS_ERRORS}>:/WX>
        )
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            $<$<BOOL:${ARDULAB_WARNINGS_AS_ERRORS}>:-Werror>
        )
    endif()
endfunction()
