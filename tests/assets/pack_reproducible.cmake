# Builds the runtime pack again from the same sources and compares it with
# the one the build made: packs must be byte for byte reproducible.
# EMULATOR (optional) runs bbassets when it is cross-compiled (Wine).
foreach(var BBASSETS SOURCE PACK OUTPUT)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "pack_reproducible.cmake: ${var} is not set")
    endif()
endforeach()

execute_process(COMMAND ${EMULATOR} "${BBASSETS}" pack "${SOURCE}" "${OUTPUT}" RESULT_VARIABLE status OUTPUT_QUIET)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "bbassets pack failed (${status})")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${PACK}" "${OUTPUT}" RESULT_VARIABLE differ)
file(REMOVE "${OUTPUT}")
if(NOT differ EQUAL 0)
    message(FATAL_ERROR "the pack built again differs from ${PACK}")
endif()
