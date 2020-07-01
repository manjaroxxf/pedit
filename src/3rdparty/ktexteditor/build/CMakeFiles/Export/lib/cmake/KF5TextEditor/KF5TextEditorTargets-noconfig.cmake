#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KF5::TextEditor" for configuration ""
set_property(TARGET KF5::TextEditor APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(KF5::TextEditor PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_NOCONFIG "Qt5::Qml;Qt5::PrintSupport;KF5::I18n;KF5::Archive;KF5::GuiAddons;KF5::ItemViews;KF5::SonnetCore;KF5::SyntaxHighlighting;KF5::AuthCore"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libKF5TextEditor.so.5.72.0"
  IMPORTED_SONAME_NOCONFIG "libKF5TextEditor.so.5"
  )

list(APPEND _IMPORT_CHECK_TARGETS KF5::TextEditor )
list(APPEND _IMPORT_CHECK_FILES_FOR_KF5::TextEditor "${_IMPORT_PREFIX}/lib/libKF5TextEditor.so.5.72.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
