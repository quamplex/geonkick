find_program(PDFLATEX_EXECUTABLE pdflatex)

if(NOT PDFLATEX_EXECUTABLE)
    message(FATAL_ERROR "pdflatex not found. Please install a LaTeX distribution to generate documentation.")
endif()

set(TEX_FILE ${GKICK_DOC_DIR}/Geonkick_User_Guide.tex)
set(PDF_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/Geonkick_User_Guide.pdf)

configure_file(${GKICK_DOC_DIR}/diagram.png ${CMAKE_CURRENT_BINARY_DIR}/diagram.png COPYONLY)

add_custom_command(
    OUTPUT ${PDF_OUTPUT_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${TEX_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${TEX_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${TEX_FILE}
    DEPENDS ${TEX_FILE} ${CMAKE_CURRENT_BINARY_DIR}/diagram.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating PDF documentation"
)

add_custom_target(
    GenerateDocumentation ALL
    DEPENDS ${PDF_OUTPUT_FILE}
)

install(FILES ${PDF_OUTPUT_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/geonkick)
