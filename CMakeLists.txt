cmake_minimum_required(VERSION 3.16)

# Version from configure.ac AC_INIT([alsaplayer],[X.Y.Z],...)
file(STRINGS "${CMAKE_SOURCE_DIR}/configure.ac" _ac_init REGEX "^AC_INIT")
set(PROJECT_VERSION "0.99.82")
if(_ac_init MATCHES "AC_INIT\\(\\[alsaplayer\\],\\[([0-9.]+)\\]")
  set(PROJECT_VERSION "${CMAKE_MATCH_1}")
endif()

project(alsaplayer VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(AlsaPlayerPlugin)

option(ENABLE_GTK3 "Build GTK3 interface" ON)
option(ENABLE_ALSA "Build ALSA output" ON)
option(ENABLE_JACK "Build JACK output" ON)
option(ENABLE_OSS "Build OSS output" ON)
option(ENABLE_MAD "Build MAD input" ON)
option(ENABLE_FLAC "Build FLAC input" ON)
option(ENABLE_VORBIS "Build Ogg Vorbis input" ON)
option(ENABLE_MIKMOD "Build MikMod input" ON)
option(ENABLE_SNDFILE "Build sndfile input" ON)
option(ENABLE_CDDA "Build CDDA input" ON)
option(ENABLE_OPENGL "Build OpenGL scope" ON)
option(ENABLE_SYSTRAY "GTK3 status icon" OFF)
option(ENABLE_NLS "Native language support" OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

set(ADDON_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/alsaplayer")

pkg_check_modules(GLIB2 glib-2.0)
if(GLIB2_FOUND)
  set(HAVE_GLIB2 1)
endif()

if(ENABLE_GTK3)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  set(HAVE_GTK3 1)
  if(ENABLE_SYSTRAY)
    set(HAVE_SYSTRAY 1)
  endif()
endif()

if(ENABLE_ALSA)
  pkg_check_modules(ALSA alsa)
  if(ALSA_FOUND)
    set(HAVE_ALSA 1)
  else()
    message(WARNING "ALSA not found; disabling")
    set(ENABLE_ALSA OFF)
  endif()
endif()

if(ENABLE_JACK)
  pkg_check_modules(JACK jack)
  if(JACK_FOUND)
    set(HAVE_JACK 1)
  else()
    message(WARNING "JACK not found; disabling")
    set(ENABLE_JACK OFF)
  endif()
endif()

if(ENABLE_OSS)
  include(CheckIncludeFile)
  check_include_file("linux/soundcard.h" HAVE_LINUX_SOUNDCARD_H)
  if(HAVE_LINUX_SOUNDCARD_H)
    set(HAVE_OSS 1)
  else()
    set(ENABLE_OSS OFF)
  endif()
endif()

if(ENABLE_MAD)
  pkg_check_modules(MAD mad)
  if(NOT MAD_FOUND)
    find_library(MAD_LIB mad)
    find_path(MAD_INC mad.h)
    if(MAD_LIB)
      set(MAD_LIBRARIES ${MAD_LIB})
      set(MAD_INCLUDE_DIRS ${MAD_INC})
      set(MAD_FOUND TRUE)
    endif()
  endif()
  if(MAD_FOUND)
    set(HAVE_LIBMAD 1)
  else()
    message(WARNING "libmad not found; disabling")
    set(ENABLE_MAD OFF)
  endif()
endif()

if(ENABLE_FLAC)
  pkg_check_modules(FLAC flac)
  if(FLAC_FOUND)
    set(HAVE_FLC 1)
  else()
    find_library(FLAC_LIB FLAC)
    if(FLAC_LIB)
      set(HAVE_FLC 1)
      set(FLAC_LIBRARIES ${FLAC_LIB} m)
      set(FLAC_FOUND TRUE)
    else()
      set(ENABLE_FLAC OFF)
    endif()
  endif()
endif()

if(ENABLE_VORBIS)
  pkg_check_modules(OGG ogg)
  pkg_check_modules(VORBIS vorbis)
  pkg_check_modules(VORBISFILE vorbisfile)
  if(OGG_FOUND AND VORBIS_FOUND AND VORBISFILE_FOUND)
    set(HAVE_OGGVORBIS 1)
  else()
    set(ENABLE_VORBIS OFF)
  endif()
endif()

if(ENABLE_MIKMOD)
  pkg_check_modules(MIKMOD libmikmod)
  if(MIKMOD_FOUND)
    set(HAVE_MIKMOD 1)
  else()
    set(ENABLE_MIKMOD OFF)
  endif()
endif()

if(ENABLE_SNDFILE)
  pkg_check_modules(SNDFILE sndfile)
  if(SNDFILE_FOUND)
    set(HAVE_SNDFILE 1)
  else()
    set(ENABLE_SNDFILE OFF)
  endif()
endif()

if(ENABLE_CDDA)
  include(CheckIncludeFile)
  check_include_file("linux/cdrom.h" _HAVE_LINUX_CDROM_H)
  if(_HAVE_LINUX_CDROM_H)
    set(HAVE_LINUX_CDROM 1)
    set(HAVE_LINUX_CDROM_H 1)
  else()
    message(WARNING "linux/cdrom.h not found; disabling CDDA")
    set(ENABLE_CDDA OFF)
  endif()
endif()

if(ENABLE_OPENGL)
  find_package(OpenGL)
  if(OpenGL_FOUND)
    set(HAVE_OPENGL 1)
  else()
    set(ENABLE_OPENGL OFF)
  endif()
endif()

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
  @ONLY
)

add_library(alsaplayer_lib SHARED
  libalsaplayer/message.c
  libalsaplayer/ap_string.c
)
set_target_properties(alsaplayer_lib PROPERTIES
  OUTPUT_NAME alsaplayer
  VERSION 0.2.0
  SOVERSION 0
)
target_include_directories(alsaplayer_lib PUBLIC
  ${CMAKE_SOURCE_DIR}/libalsaplayer
  ${CMAKE_SOURCE_DIR}/alsaplayer
  ${CMAKE_BINARY_DIR}
)
target_compile_definitions(alsaplayer_lib PRIVATE HAVE_CONFIG_H)
install(TARGETS alsaplayer_lib
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES
  alsaplayer/input_plugin.h
  alsaplayer/output_plugin.h
  alsaplayer/scope_plugin.h
  alsaplayer/interface_plugin.h
  alsaplayer/stream_info.h
  alsaplayer/CorePlayer.h
  alsaplayer/Playlist.h
  alsaplayer/SampleBuffer.h
  alsaplayer/AlsaNode.h
  alsaplayer/Effects.h
  alsaplayer/utilities.h
  alsaplayer/alsaplayer_error.h
  alsaplayer/AlsaSubscriber.h
  alsaplayer/prefs.h
  alsaplayer/AlsaPlayer.h
  alsaplayer/control.h
  alsaplayer/reader.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/alsaplayer
)

add_executable(alsaplayer
  app/AlsaSubscriber.cpp
  app/AlsaNode.cpp
  app/Effects.cpp
  app/Main.cpp
  app/CorePlayer.cpp
  app/Playlist.cpp
  app/SampleBuffer.cpp
  app/reverbst.cpp
  app/fft.c
  app/convolve.c
  app/utilities.c
  app/ControlSocket.cpp
  app/prefs.c
  app/reader.cpp
)
target_include_directories(alsaplayer PRIVATE
  ${CMAKE_SOURCE_DIR}/alsaplayer
  ${CMAKE_SOURCE_DIR}/libalsaplayer
  ${CMAKE_BINARY_DIR}
)
target_compile_definitions(alsaplayer PRIVATE
  HAVE_CONFIG_H
  _REENTRANT
  ADDON_DIR="${ADDON_DIR}"
  LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)
target_link_libraries(alsaplayer PRIVATE
  alsaplayer_lib
  Threads::Threads
  ${CMAKE_DL_LIBS}
)
target_link_options(alsaplayer PRIVATE -rdynamic)
install(TARGETS alsaplayer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

alsa_player_plugin(file DIR reader SOURCES reader/file/file.c)
alsa_player_plugin(http DIR reader SOURCES reader/http/http.c LIBS alsaplayer_lib)

alsa_player_plugin(null_out DIR output SOURCES output/null/null.c)

if(ENABLE_ALSA)
  alsa_player_plugin(alsa_out DIR output
    SOURCES output/alsa/alsa.c
    LIBS ${ALSA_LIBRARIES}
    INCLUDES ${ALSA_INCLUDE_DIRS}
  )
  target_compile_options(alsa_out PRIVATE ${ALSA_CFLAGS_OTHER})
endif()

if(ENABLE_JACK)
  alsa_player_plugin(jack_out DIR output
    SOURCES output/jack/jack.cpp
    LIBS ${JACK_LIBRARIES}
    INCLUDES ${JACK_INCLUDE_DIRS}
  )
  target_compile_options(jack_out PRIVATE ${JACK_CFLAGS_OTHER})
endif()

if(ENABLE_OSS)
  alsa_player_plugin(oss_out DIR output SOURCES output/oss/oss.c)
endif()

if(ENABLE_MAD)
  set(_mad_libs ${MAD_LIBRARIES})
  if(GLIB2_FOUND)
    list(APPEND _mad_libs ${GLIB2_LIBRARIES})
  endif()
  alsa_player_plugin(mad_in DIR input
    SOURCES input/mad/mad_engine.c input/mad/xing.c
    LIBS ${_mad_libs}
    INCLUDES ${MAD_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS}
  )
endif()

if(ENABLE_FLAC)
  alsa_player_plugin(flac_in DIR input
    SOURCES
      input/flac/FlacEngine.cpp
      input/flac/FlacPlugin.cpp
      input/flac/FlacStream.cpp
      input/flac/FlacTag.cpp
      input/flac/FlacSeekableStream.cpp
      input/flac/FlacMetadataTag.cpp
      input/flac/FlacId3v1Tag.cpp
    LIBS ${FLAC_LIBRARIES}
    INCLUDES ${FLAC_INCLUDE_DIRS}
  )
endif()

if(ENABLE_VORBIS)
  alsa_player_plugin(vorbis_in DIR input
    SOURCES input/vorbis/vorbis_engine.c
    LIBS ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${OGG_LIBRARIES}
    INCLUDES ${VORBIS_INCLUDE_DIRS} ${VORBISFILE_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS}
  )
endif()

if(ENABLE_MIKMOD)
  alsa_player_plugin(mod DIR input
    SOURCES input/mikmod/mikmod_engine.c input/mikmod/drv_alsaplayer.c
    LIBS ${MIKMOD_LIBRARIES}
    INCLUDES ${MIKMOD_INCLUDE_DIRS}
  )
endif()

if(ENABLE_SNDFILE)
  alsa_player_plugin(sndfile_in DIR input
    SOURCES input/sndfile/sndfile_engine.c
    LIBS ${SNDFILE_LIBRARIES}
    INCLUDES ${SNDFILE_INCLUDE_DIRS}
  )
endif()

if(ENABLE_CDDA)
  alsa_player_plugin(cdda DIR input
    SOURCES input/cdda/cdda_engine.c
    DEFINES HAVE_LINUX_CDROM_H HAVE_LINUX_CDROM
  )
endif()

alsa_player_plugin(text_interface DIR interface SOURCES interface/text/text.cpp)
alsa_player_plugin(daemon_interface DIR interface SOURCES interface/daemon/daemon.cpp)

if(ENABLE_GTK3)
  set(_gtk3_sources
    interface/gtk3/gtk.cpp
    interface/gtk3/gtk_interface.cpp
    interface/gtk3/PlaylistWindow.cpp
    interface/gtk3/ScopesWindow.cpp
    interface/gtk3/AboutWindow.cpp
    interface/gtk3/info_window.cpp
    interface/gtk3/PreferencesWindow.cpp
  )
  if(ENABLE_SYSTRAY)
    list(APPEND _gtk3_sources interface/gtk3/StatusIcon.cpp)
  endif()
  alsa_player_plugin(gtk3_interface DIR interface
    SOURCES ${_gtk3_sources}
    LIBS ${GTK3_LIBRARIES} ${CMAKE_DL_LIBS}
    INCLUDES ${GTK3_INCLUDE_DIRS}
  )
  target_compile_options(gtk3_interface PRIVATE ${GTK3_CFLAGS_OTHER} -Wno-deprecated-declarations)
  target_include_directories(gtk3_interface PRIVATE ${CMAKE_SOURCE_DIR}/interface/gtk3)
endif()

if(ENABLE_OPENGL)
  alsa_player_plugin(oglspectrum DIR scopes2
    SOURCES scopes2/opengl_spectrum/opengl_spectrum.c
    LIBS OpenGL::GL OpenGL::GLU
  )
endif()

# pkg-config paths must stay relative to ${prefix}. Under Nix (and some
# other packagers) CMAKE_INSTALL_LIBDIR / INCLUDEDIR are absolute full
# paths; joining them with ${prefix}/ produces broken //store/... entries.
set(_pc_libdir "${CMAKE_INSTALL_LIBDIR}")
set(_pc_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
if(IS_ABSOLUTE "${_pc_libdir}")
  file(RELATIVE_PATH _pc_libdir "${CMAKE_INSTALL_PREFIX}" "${_pc_libdir}")
endif()
if(IS_ABSOLUTE "${_pc_includedir}")
  file(RELATIVE_PATH _pc_includedir "${CMAKE_INSTALL_PREFIX}" "${_pc_includedir}")
endif()
# Fall back to conventional relative dirs if relative_path failed
if(_pc_libdir STREQUAL "" OR _pc_libdir MATCHES "^\\.\\.")
  set(_pc_libdir "lib")
endif()
if(_pc_includedir STREQUAL "" OR _pc_includedir MATCHES "^\\.\\.")
  set(_pc_includedir "include")
endif()

file(WRITE ${CMAKE_BINARY_DIR}/alsaplayer.pc
"prefix=${CMAKE_INSTALL_PREFIX}
libdir=\${prefix}/${_pc_libdir}
includedir=\${prefix}/${_pc_includedir}

Name: alsaplayer
Description: PCM player library
Version: ${PROJECT_VERSION}
Libs: -L\${libdir} -lalsaplayer
Cflags: -I\${includedir}/alsaplayer
")
install(FILES ${CMAKE_BINARY_DIR}/alsaplayer.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(EXISTS ${CMAKE_SOURCE_DIR}/extra/alsaplayer.desktop)
  install(FILES extra/alsaplayer.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

message(STATUS "AlsaPlayer ${PROJECT_VERSION}")
message(STATUS "  ADDON_DIR: ${ADDON_DIR}")
message(STATUS "  GTK3: ${ENABLE_GTK3}")
message(STATUS "  ALSA: ${ENABLE_ALSA}  JACK: ${ENABLE_JACK}  OSS: ${ENABLE_OSS}")
message(STATUS "  MAD: ${ENABLE_MAD}  FLAC: ${ENABLE_FLAC}  Vorbis: ${ENABLE_VORBIS}")
message(STATUS "  MikMod: ${ENABLE_MIKMOD}  sndfile: ${ENABLE_SNDFILE}  CDDA: ${ENABLE_CDDA}")
message(STATUS "  OpenGL scope: ${ENABLE_OPENGL}")
