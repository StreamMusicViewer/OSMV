# Generates embedded_logo.cpp from binary assets at build time.
# Required CMake -D variables: LOGO_PNG, TRAY_ARGB, OUT_CPP

function(_osmv_read_hex_array input_path out_var out_size_var)
    if(NOT EXISTS "${input_path}")
        message(FATAL_ERROR "Missing required logo asset: ${input_path}")
    endif()
    file(SIZE "${input_path}" file_size)
    file(READ "${input_path}" hex_data HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," c_bytes "${hex_data}")
    string(REGEX REPLACE "((0x[0-9a-f][0-9a-f],){24})" "\\1\n    " formatted "${c_bytes}")
    set(${out_var} "${formatted}" PARENT_SCOPE)
    set(${out_size_var} "${file_size}" PARENT_SCOPE)
endfunction()

_osmv_read_hex_array("${LOGO_PNG}" logo_bytes logo_size)
_osmv_read_hex_array("${TRAY_ARGB}" tray_bytes tray_size)

get_filename_component(out_dir "${OUT_CPP}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")

file(WRITE "${OUT_CPP}"
"// Auto-generated at build time by cmake/embed_logo.cmake. Do not edit.\n"
"#include \"common/embedded_logo.hpp\"\n\n"
"namespace osmv::assets {\n\n"
"const size_t kLogoPngSize = ${logo_size};\n"
"const uint8_t kLogoPng[] = {\n    ${logo_bytes}\n};\n\n"
"const int kTrayIconSize = 32;\n"
"const uint8_t kTrayIconArgb32[] = {\n    ${tray_bytes}\n};\n\n"
"} // namespace osmv::assets\n"
)

