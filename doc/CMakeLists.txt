find_program(PDFLATEX_EXECUTABLE pdflatex)
if(NOT PDFLATEX_EXECUTABLE)
    message(FATAL_ERROR "pdflatex not found. Install a LaTeX distribution to generate the PDF documentation.")
endif()

find_program(PANDOC_EXECUTABLE pandoc)
if(NOT PANDOC_EXECUTABLE)
    message(FATAL_ERROR "pandoc not found. Install Pandoc to generate the Markdown documentation.")
endif()

set(TEX_FILE ${GKICK_DOC_DIR}/Geonkick_User_Guide.tex)
set(CONFIGURED_TEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/Geonkick_User_Guide.tex)
set(GEONKICK_DOC_VERSION ${PROJECT_VERSION})
set(DIAGRAM_FILE ${GKICK_DOC_DIR}/diagram.png)
set(DIAGRAM_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/diagram.png)
set(PDF_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/Geonkick_User_Guide-${GEONKICK_DOC_VERSION}.pdf)
set(MARKDOWN_OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/Geonkick_User_Guide.md)

configure_file(${TEX_FILE} ${CONFIGURED_TEX_FILE} COPYONLY)
file(READ ${CONFIGURED_TEX_FILE} CONFIGURED_TEX_SOURCE)
string(REPLACE "@GEONKICK_DOC_VERSION@" "${GEONKICK_DOC_VERSION}" CONFIGURED_TEX_SOURCE "${CONFIGURED_TEX_SOURCE}")
file(WRITE ${CONFIGURED_TEX_FILE} "${CONFIGURED_TEX_SOURCE}")
configure_file(${DIAGRAM_FILE} ${DIAGRAM_OUTPUT_FILE} COPYONLY)

add_custom_command(
    OUTPUT ${PDF_OUTPUT_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -jobname=Geonkick_User_Guide-${GEONKICK_DOC_VERSION} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${CONFIGURED_TEX_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -jobname=Geonkick_User_Guide-${GEONKICK_DOC_VERSION} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${CONFIGURED_TEX_FILE}
    COMMAND ${PDFLATEX_EXECUTABLE} -jobname=Geonkick_User_Guide-${GEONKICK_DOC_VERSION} -interaction=batchmode -halt-on-error -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${CONFIGURED_TEX_FILE}
    DEPENDS ${CONFIGURED_TEX_FILE} ${DIAGRAM_OUTPUT_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating PDF documentation"
    VERBATIM
)

add_custom_command(
    OUTPUT ${MARKDOWN_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND}
        -DTEX_FILE=${CONFIGURED_TEX_FILE}
        -DPANDOC_EXECUTABLE=${PANDOC_EXECUTABLE}
        -DOUTPUT_FILE=${MARKDOWN_OUTPUT_FILE}
        -DDOC_VERSION=${GEONKICK_DOC_VERSION}
        -P ${GKICK_DOC_DIR}/generate_markdown.cmake
    DEPENDS ${CONFIGURED_TEX_FILE} ${GKICK_DOC_DIR}/generate_markdown.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating Markdown documentation"
    VERBATIM
)

add_custom_target(
    GenerateDocumentation ALL
    DEPENDS ${PDF_OUTPUT_FILE} ${MARKDOWN_OUTPUT_FILE}
)

install(FILES ${PDF_OUTPUT_FILE} ${MARKDOWN_OUTPUT_FILE} ${DIAGRAM_OUTPUT_FILE}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/geonkick)
