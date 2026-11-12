# Generates embedded_fonts.cpp from binary TTF assets at build time.
# Required CMake -D variables: INTER_TTF, FA_TTF, OUT_CPP

function(_osmv_read_hex_array input_path out_var out_size_var)
    if(NOT EXISTS "${input_path}")
        message(FATAL_ERROR "Missing required font asset: ${input_path}")
    endif()
    file(SIZE "${input_path}" file_size)
    file(READ "${input_path}" hex_data HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," c_bytes "${hex_data}")
    string(REGEX REPLACE "((0x[0-9a-f][0-9a-f],){24})" "\\1\n    " formatted "${c_bytes}")
    set(${out_var} "${formatted}" PARENT_SCOPE)
    set(${out_size_var} "${file_size}" PARENT_SCOPE)
endfunction()

_osmv_read_hex_array("${INTER_TTF}" inter_bytes inter_size)
_osmv_read_hex_array("${FA_TTF}" fa_bytes fa_size)

get_filename_component(out_dir "${OUT_CPP}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")

file(WRITE "${OUT_CPP}"
"// Auto-generated at build time by cmake/embed_fonts.cmake. Do not edit.\n"
"#include \"common/embedded_fonts.hpp\"\n\n"
"namespace osmv::assets {\n\n"
"const size_t kInterFontTtfSize = ${inter_size};\n"
"const unsigned char kInterFontTtf[] = {\n    ${inter_bytes}\n};\n\n"
"const size_t kFaSolidTtfSize = ${fa_size};\n"
"const unsigned char kFaSolidTtf[] = {\n    ${fa_bytes}\n};\n\n"
"} // namespace osmv::assets\n"
)

