# alsa_player_plugin(<name> DIR <subdir> SOURCES ... [LIBS ...] [INCLUDES ...])
function(alsa_player_plugin name)
  cmake_parse_arguments(ARG "" "DIR" "SOURCES;LIBS;DEFINES;INCLUDES" ${ARGN})
  if(NOT ARG_DIR)
    message(FATAL_ERROR "alsa_player_plugin(${name}): DIR required")
  endif()
  add_library(${name} MODULE ${ARG_SOURCES})
  set_target_properties(${name} PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "${name}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${ARG_DIR}"
  )
  target_compile_definitions(${name} PRIVATE
    _REENTRANT
    HAVE_CONFIG_H
    ADDON_DIR="${ADDON_DIR}"
    ${ARG_DEFINES}
  )
  target_include_directories(${name} PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/alsaplayer
    ${CMAKE_SOURCE_DIR}/libalsaplayer
    ${ARG_INCLUDES}
  )
  if(ARG_LIBS)
    target_link_libraries(${name} PRIVATE ${ARG_LIBS})
  endif()
  install(TARGETS ${name}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/alsaplayer/${ARG_DIR}")
endfunction()
