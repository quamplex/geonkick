foreach(required_variable TEX_FILE PANDOC_EXECUTABLE OUTPUT_FILE DOC_VERSION)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be set to generate Markdown documentation.")
    endif()
endforeach()

file(READ "${TEX_FILE}" TEX_SOURCE)
string(FIND "${TEX_SOURCE}" "\\begin{document}" DOCUMENT_START)
string(FIND "${TEX_SOURCE}" "\\end{document}" DOCUMENT_END)
if(DOCUMENT_START EQUAL -1 OR DOCUMENT_END EQUAL -1 OR DOCUMENT_END LESS DOCUMENT_START)
    message(FATAL_ERROR "Could not locate the document body in ${TEX_FILE}.")
endif()

string(LENGTH "\\begin{document}" DOCUMENT_MARKER_LENGTH)
math(EXPR DOCUMENT_START "${DOCUMENT_START} + ${DOCUMENT_MARKER_LENGTH}")
math(EXPR DOCUMENT_LENGTH "${DOCUMENT_END} - ${DOCUMENT_START}")
string(SUBSTRING "${TEX_SOURCE}" ${DOCUMENT_START} ${DOCUMENT_LENGTH} DOCUMENT_BODY)

string(FIND "${DOCUMENT_BODY}" "\\begin{titlepage}" TITLEPAGE_START)
string(FIND "${DOCUMENT_BODY}" "\\end{titlepage}" TITLEPAGE_END)
if(NOT TITLEPAGE_START EQUAL -1 AND NOT TITLEPAGE_END EQUAL -1)
    string(LENGTH "\\end{titlepage}" TITLEPAGE_END_MARKER_LENGTH)
    math(EXPR TITLEPAGE_END "${TITLEPAGE_END} + ${TITLEPAGE_END_MARKER_LENGTH}")
    string(SUBSTRING "${DOCUMENT_BODY}" 0 ${TITLEPAGE_START} BEFORE_TITLEPAGE)
    string(SUBSTRING "${DOCUMENT_BODY}" ${TITLEPAGE_END} -1 AFTER_TITLEPAGE)
    set(DOCUMENT_BODY "${BEFORE_TITLEPAGE}${AFTER_TITLEPAGE}")
endif()

foreach(command
        "\\hypersetup{pageanchor=false}"
        "\\hypersetup{pageanchor=true}"
        "\\pagenumbering{roman}"
        "\\pagenumbering{arabic}"
        "\\tableofcontents"
        "\\clearpage"
        "\\small"
        "\\normalsize")
    string(REPLACE "${command}" "" DOCUMENT_BODY "${DOCUMENT_BODY}")
endforeach()

set(TEMP_TEX_FILE "${OUTPUT_FILE}.tex")
file(WRITE "${TEMP_TEX_FILE}"
    "\\documentclass{article}\n\\begin{document}\n${DOCUMENT_BODY}\n\\end{document}\n")

execute_process(
    COMMAND "${PANDOC_EXECUTABLE}"
        --from=latex
        --to=markdown
        --shift-heading-level-by=1
        --standalone
        --toc
        --toc-depth=3
        --wrap=none
        "${TEMP_TEX_FILE}"
        "--output=${OUTPUT_FILE}.body"
    RESULT_VARIABLE PANDOC_RESULT
    ERROR_VARIABLE PANDOC_ERROR
)
if(NOT PANDOC_RESULT EQUAL 0)
    file(REMOVE "${TEMP_TEX_FILE}" "${OUTPUT_FILE}.body")
    message(FATAL_ERROR "Pandoc failed to generate Markdown documentation: ${PANDOC_ERROR}")
endif()

file(READ "${OUTPUT_FILE}.body" MARKDOWN_BODY)
file(WRITE "${OUTPUT_FILE}"
    "# Geonkick User Guide\n\n"
    "**Author:** Iurie Nistor  \n"
    "**Version:** ${DOC_VERSION}  \n"
    "**Website:** <https://quamplex.com/geonkick>  \n"
    "**License:** [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/deed.en)\n\n"
    "${MARKDOWN_BODY}")
file(REMOVE "${TEMP_TEX_FILE}" "${OUTPUT_FILE}.body")
