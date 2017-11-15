 # add a target to generate API documentation with Doxygen
find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${BUILD_PATH}/Doxyfile)
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)
    add_custom_command(TARGET Main POST_BUILD COMMAND ${CMAKE_COMMAND} ${DOXYGEN_EXECUTABLE} ${doxyfile} WORKING_DIRECTORY ${BUILD_PATH} COMMENT "Generating API documentation with 
    Doxygen" VERBATIM)
 #   add_custom_target(TARGET Main POST_BUILD COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile} WORKING_DIRECTORY ${BUILD_PATH} COMMENT "Generating API documentation with Doxygen" VERBATIM)
    install(DIRECTORY ${BUILD_PATH}/doc/html DESTINATION ${DOCUMENTATION_OUTPUT_PATH})
endif()
