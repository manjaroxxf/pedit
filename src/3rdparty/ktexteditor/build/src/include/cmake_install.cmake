# Install script for directory: /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/KF5/KTextEditor/ktexteditor" TYPE FILE FILES
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/annotationinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/codecompletionmodelcontrollerinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/movingcursor.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/range.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/texthintinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/cursor.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/markinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/movinginterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/inlinenote.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/inlinenoteprovider.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/inlinenoteinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/abstractannotationitemdelegate.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/document.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/movingrange.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/view.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/attribute.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/command.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/documentcursor.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/message.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/movingrangefeedback.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/sessionconfiginterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/codecompletioninterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/configinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/editor.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/codecompletionmodel.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/configpage.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/modificationinterface.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/application.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/mainwindow.h"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/include/ktexteditor/plugin.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/KF5/KTextEditor/KTextEditor" TYPE FILE FILES
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/AnnotationInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/CodeCompletionModelControllerInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MovingCursor"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Range"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/TextHintInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Cursor"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MarkInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MovingInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/InlineNote"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/InlineNoteProvider"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/InlineNoteInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/AbstractAnnotationItemDelegate"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Document"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MovingRange"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/View"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Attribute"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Command"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/DocumentCursor"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Message"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MovingRangeFeedback"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/SessionConfigInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/CodeCompletionInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/ConfigInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Editor"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/CodeCompletionModel"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/ConfigPage"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/ModificationInterface"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Application"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/MainWindow"
    "/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/include/KTextEditor/Plugin"
    )
endif()

