cmake_minimum_required(VERSION 3.16)

project(OpenIndy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg SerialPort Network Xml WebSockets)

# --- version, derived from this repo's own git describe ---

find_package(Git QUIET)

function(oi_git_describe out_var working_dir)
    set(${out_var} "0.0.0" PARENT_SCOPE)
    if(NOT Git_FOUND)
        return()
    endif()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags
        WORKING_DIRECTORY ${working_dir}
        OUTPUT_VARIABLE describe_out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE describe_result
    )
    if(NOT describe_result EQUAL 0 OR NOT describe_out)
        return()
    endif()
    string(REGEX REPLACE "-g[0-9a-f]+$" "" describe_out "${describe_out}")
    string(REGEX REPLACE "-" "." describe_out "${describe_out}")
    set(${out_var} "${describe_out}" PARENT_SCOPE)
endfunction()

oi_git_describe(OI_APP_GIT_VERSION "${CMAKE_CURRENT_SOURCE_DIR}")

# App is the true source of OPENINDY_VERSION; force it so it wins over the
# placeholder default OpenIndy-Core's own CMakeLists sets for standalone
# builds (see OpenIndy-Core's CMakeLists.txt for that mechanism).
set(OPENINDY_VERSION "${OI_APP_GIT_VERSION}" CACHE STRING
    "Version string embedded via the OPENINDY_VERSION define" FORCE)

set(OI_APP_GIT_COMMIT "unknown")
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --verify --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE commit_out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE commit_result
    )
    if(commit_result EQUAL 0 AND commit_out)
        set(OI_APP_GIT_COMMIT "${commit_out}")
    endif()
endif()

# --- Core dependency (brings in Math transitively) ---

include(FetchContent)

set(OI_CORE_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../openindy-core" CACHE PATH
    "Local OpenIndy-Core checkout to build against instead of fetching from git")

if(EXISTS "${OI_CORE_LOCAL_DIR}/CMakeLists.txt")
    FetchContent_Declare(OpenIndyCore SOURCE_DIR "${OI_CORE_LOCAL_DIR}")
else()
    FetchContent_Declare(OpenIndyCore
        GIT_REPOSITORY git@github.com:siyka-au/openindy-core.git
        GIT_TAG        master
    )
endif()
FetchContent_MakeAvailable(OpenIndyCore)

# best-effort cosmetic version strings for the About dialog; both Core and
# Math are git checkouts (local sibling or freshly cloned) either way
oi_git_describe(OI_CORE_GIT_VERSION "${openindycore_SOURCE_DIR}")
set(OI_MATH_LOCAL_DIR_GUESS "${openindycore_SOURCE_DIR}/../openindy-math")
if(EXISTS "${OI_MATH_LOCAL_DIR_GUESS}/.git")
    oi_git_describe(OI_MATH_GIT_VERSION "${OI_MATH_LOCAL_DIR_GUESS}")
else()
    FetchContent_GetProperties(OpenIndyMath)
    if(openindymath_POPULATED)
        oi_git_describe(OI_MATH_GIT_VERSION "${openindymath_SOURCE_DIR}")
    else()
        set(OI_MATH_GIT_VERSION "unknown")
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

# --- default plugin dependency ---
# The app is of little use without it, so it's built alongside openIndy
# (not just deployed by a separate manual step) and packaged into the app's
# own plugins/ dir as part of openIndy's POST_BUILD step below.

set(OI_DEFAULT_PLUGIN_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../openindy-default-plugin" CACHE PATH
    "Local OpenIndy-DefaultPlugin checkout to build against instead of fetching from git")

if(EXISTS "${OI_DEFAULT_PLUGIN_LOCAL_DIR}/CMakeLists.txt")
    FetchContent_Declare(OpenIndyDefaultPlugin SOURCE_DIR "${OI_DEFAULT_PLUGIN_LOCAL_DIR}")
else()
    FetchContent_Declare(OpenIndyDefaultPlugin
        GIT_REPOSITORY git@github.com:siyka-au/openindy-default-plugin.git
        GIT_TAG        master
    )
endif()
FetchContent_MakeAvailable(OpenIndyDefaultPlugin)

# --- appui library ---

add_library(OpenIndyApp SHARED
    controller/clipboardutil.cpp
    controller/config/featuretablecolumnconfig.cpp
    controller/config/featuretablesortingconfig.cpp
    controller/config/measurementconfigmanager.cpp
    controller/config/observationtablecolumnconfig.cpp
    controller/config/parameterdisplayconfig.cpp
    controller/config/projectconfig.cpp
    controller/config/readingtablecolumnconfig.cpp
    controller/config/sensorconfigurationmanager.cpp
    controller/config/trafoparamtablecolumnconfig.cpp
    controller/console.cpp
    controller/controller.cpp
    controller/exchange/dataexchanger.cpp
    controller/exchange/observationimporter.cpp
    controller/exchange/projectexchanger.cpp
    controller/featuresorter.cpp
    controller/featureupdater.cpp
    controller/featureutil.cpp
    controller/measurebehaviorlogic.cpp
    controller/modelmanager.cpp
    controller/network/oirequesthandler.cpp
    controller/network/oiwebsocket.cpp
    controller/network/oiwebsocketserver.cpp
    controller/network/oiwebsocketstreamer.cpp
    controller/oimetadata.cpp
    controller/plugincopier.cpp
    controller/pluginloader.cpp
    controller/pluginrepository.cpp
    controller/settings.cpp
    controller/sortlistbyname.cpp
    controller/trafocontroller.cpp
    models/activefeaturefunctionsmodel.cpp
    models/availableelementstreeviewproxymodel.cpp
    models/availablefunctionslistmodel.cpp
    models/availablefunctionslistproxymodel.cpp
    models/bundlegeometriesmodel.cpp
    models/bundleparametertableproxymodel.cpp
    models/bundlestationsmodel.cpp
    models/featuredifferenceproxymodel.cpp
    models/featuredifferencetablemodel.cpp
    models/featuretablemodel.cpp
    models/featuretableproxymodel.cpp
    models/featuretreeviewmodel.cpp
    models/functionstatisticmodel.cpp
    models/functiontablemodel.cpp
    models/functiontableproxymodel.cpp
    models/functionweightdelegate.cpp
    models/functionweightproxymodel.cpp
    models/functionweightstablemodel.cpp
    models/measurementconfigurationmodel.cpp
    models/measurementconfigurationproxymodel.cpp
    models/observationmodel.cpp
    models/observationproxymodel.cpp
    models/plugintreeviewmodel.cpp
    models/readingmodel.cpp
    models/readingproxymodel.cpp
    models/sensoraccuracymodel.cpp
    models/sensorconfigurationmodel.cpp
    models/sensorconfigurationproxymodel.cpp
    models/sensorparametersmodel.cpp
    models/sensortablemodel.cpp
    models/sensortableproxymodel.cpp
    models/trafoparamtableproxymodel.cpp
    models/treeitems/bundlegeometryitem.cpp
    models/treeitems/bundlestationitem.cpp
    models/treeitems/featuretreeitem.cpp
    models/treeitems/functiontreeitem.cpp
    models/treeitems/plugintreeitem.cpp
    models/usedelementsmodel.cpp
    ui/actions/toolaction.cpp
    ui/delegates/bundleoverviewdelegate.cpp
    ui/delegates/featuretabledelegate.cpp
    ui/delegates/measurementconfigurationlistdelegate.cpp
    ui/delegates/observationtabledelegate.cpp
    ui/delegates/readingtabledelegate.cpp
    ui/delegates/sensoraccuracydelegate.cpp
    ui/delegates/sensorconfigurationlistdelegate.cpp
    ui/delegates/sensorparametersdelegate.cpp
    ui/delegates/trafoparamtabledelegate.cpp
    ui/dialogs/aboutdialog.cpp
    ui/dialogs/actualpropertiesdialog.cpp
    ui/dialogs/createfeaturedialog.cpp
    ui/dialogs/exportdialog.cpp
    ui/dialogs/featurefunctionsdialog.cpp
    ui/dialogs/importnominaldialog.cpp
    ui/dialogs/loadingdialog.cpp
    ui/dialogs/measurebehaviordialog.cpp
    ui/dialogs/measurementconfigurationdialog.cpp
    ui/dialogs/movesensordialog.cpp
    ui/dialogs/nominalpropertiesdialog.cpp
    ui/dialogs/pluginloaderdialog.cpp
    ui/dialogs/pluginmanagerdialog.cpp
    ui/dialogs/sensorconfigurationdialog.cpp
    ui/dialogs/sensortaskinfodialog.cpp
    ui/dialogs/settingsdialog.cpp
    ui/dialogs/showlicensesdialog.cpp
    ui/dialogs/stationpropertiesdialog.cpp
    ui/dialogs/trafoparampropertiesdialog.cpp
    ui/dialogs/uiutil.cpp
    ui/dialogs/watchwindowdialog.cpp
    ui/dialogs/watchwindowutil.cpp
    ui/mainwindow.cpp
    ui/selectionmodels/multiselectionmodel.cpp
    ui/widgets/plugininfowidget.cpp
    ui/widgets/scalarparameterwidget.cpp
    ui/widgets/stationstatus.cpp

    controller/clipboardutil.h
    controller/config/featuretablecolumnconfig.h
    controller/config/featuretablesortingconfig.h
    controller/config/measurementconfigmanager.h
    controller/config/observationtablecolumnconfig.h
    controller/config/parameterdisplayconfig.h
    controller/config/projectconfig.h
    controller/config/readingtablecolumnconfig.h
    controller/config/sensorconfigurationmanager.h
    controller/config/trafoparamtablecolumnconfig.h
    controller/console.h
    controller/controller.h
    controller/controllersensoractions.h
    controller/exchange/dataexchanger.h
    controller/exchange/observationimporter.h
    controller/exchange/projectexchanger.h
    controller/featuresorter.h
    controller/featureupdater.h
    controller/featureutil.h
    controller/measurebehaviorlogic.h
    controller/measurebehaviortypes.h
    controller/modelmanager.h
    controller/network/oirequesthandler.h
    controller/network/oiwebsocket.h
    controller/network/oiwebsocketserver.h
    controller/network/oiwebsocketstreamer.h
    controller/param/exchangeParams.h
    controller/plugincopier.h
    controller/pluginloader.h
    controller/plugininfo.h
    controller/pluginrepository.h
    controller/settings.h
    controller/sortlistbyname.h
    controller/trafocontroller.h
    models/activefeaturefunctionsmodel.h
    models/availableelementstreeviewproxymodel.h
    models/availablefunctionslistmodel.h
    models/availablefunctionslistproxymodel.h
    models/bundlegeometriesmodel.h
    models/bundleparametertableproxymodel.h
    models/bundlestationsmodel.h
    models/featuredifferenceproxymodel.h
    models/featuredifferencetablemodel.h
    models/featuretablemodel.h
    models/featuretableproxymodel.h
    models/featuretreeviewmodel.h
    models/functionstatisticmodel.h
    models/functiontablemodel.h
    models/functiontableproxymodel.h
    models/functionweightdelegate.h
    models/functionweightproxymodel.h
    models/functionweightstablemodel.h
    models/measurementconfigurationmodel.h
    models/measurementconfigurationproxymodel.h
    models/observationmodel.h
    models/observationproxymodel.h
    models/plugintreeviewmodel.h
    models/readingmodel.h
    models/readingproxymodel.h
    models/sensoraccuracymodel.h
    models/sensorconfigurationmodel.h
    models/sensorconfigurationproxymodel.h
    models/sensorparametersmodel.h
    models/sensortablemodel.h
    models/sensortableproxymodel.h
    models/trafoparamtableproxymodel.h
    models/treeitems/bundlegeometryitem.h
    models/treeitems/bundlestationitem.h
    models/treeitems/featuretreeitem.h
    models/treeitems/functiontreeitem.h
    models/treeitems/plugintreeitem.h
    models/usedelementsmodel.h
    ui/actions/toolaction.h
    ui/delegates/bundleoverviewdelegate.h
    ui/delegates/featuretabledelegate.h
    ui/delegates/measurementconfigurationlistdelegate.h
    ui/delegates/observationtabledelegate.h
    ui/delegates/readingtabledelegate.h
    ui/delegates/sensoraccuracydelegate.h
    ui/delegates/sensorconfigurationlistdelegate.h
    ui/delegates/sensorparametersdelegate.h
    ui/delegates/trafoparamtabledelegate.h
    ui/dialogs/aboutdialog.h
    ui/dialogs/actualpropertiesdialog.h
    ui/dialogs/createfeaturedialog.h
    ui/dialogs/exportdialog.h
    ui/dialogs/featurefunctionsdialog.h
    ui/dialogs/importnominaldialog.h
    ui/dialogs/loadingdialog.h
    ui/dialogs/measurebehaviordialog.h
    ui/dialogs/measurementconfigurationdialog.h
    ui/dialogs/movesensordialog.h
    ui/dialogs/nominalpropertiesdialog.h
    ui/dialogs/pluginloaderdialog.h
    ui/dialogs/pluginmanagerdialog.h
    ui/dialogs/sensorconfigurationdialog.h
    ui/dialogs/sensortaskinfodialog.h
    ui/dialogs/settingsdialog.h
    ui/dialogs/showlicensesdialog.h
    ui/dialogs/stationpropertiesdialog.h
    ui/dialogs/trafoparampropertiesdialog.h
    ui/dialogs/uiutil.h
    ui/dialogs/watchwindowdialog.h
    ui/dialogs/watchwindowutil.h
    ui/mainwindow.h
    ui/selectionmodels/multiselectionmodel.h
    ui/widgets/plugininfowidget.h
    ui/widgets/scalarparameterwidget.h
    ui/widgets/stationstatus.h

    ui/dialogs/aboutdialog.ui
    ui/dialogs/actualpropertiesdialog.ui
    ui/dialogs/createfeaturedialog.ui
    ui/dialogs/exportdialog.ui
    ui/dialogs/featurefunctionsdialog.ui
    ui/dialogs/importnominaldialog.ui
    ui/dialogs/loadingdialog.ui
    ui/dialogs/measurebehaviordialog.ui
    ui/dialogs/measurementconfigurationdialog.ui
    ui/dialogs/movesensordialog.ui
    ui/dialogs/nominalpropertiesdialog.ui
    ui/dialogs/pluginloaderdialog.ui
    ui/dialogs/pluginmanagerdialog.ui
    ui/dialogs/sensorconfigurationdialog.ui
    ui/dialogs/sensortaskinfodialog.ui
    ui/dialogs/settingsdialog.ui
    ui/dialogs/showlicensesdialog.ui
    ui/dialogs/stationpropertiesdialog.ui
    ui/dialogs/trafoparampropertiesdialog.ui
    ui/dialogs/watchwindowdialog.ui
    ui/mainwindow.ui
    ui/widgets/stationstatus.ui

    res/res.qrc
)

target_compile_definitions(OpenIndyApp
    PRIVATE
        OI_MAIN_LIB
)

target_include_directories(OpenIndyApp PRIVATE "${CMAKE_BINARY_DIR}/generated")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(OpenIndyApp PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(OpenIndyApp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/models>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/models/treeitems>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controller>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controller/config>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controller/exchange>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controller/param>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controller/network>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/delegates>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/dialogs>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/actions>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/selectionmodels>
)

target_link_libraries(OpenIndyApp
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::SerialPort
        Qt6::Network
        Qt6::Xml
        Qt6::WebSockets
        OpenIndyCore
        OpenIndyMath
    PRIVATE
        GL
        GLU
)

# --- executable ---

add_executable(openIndy main.cpp)

target_compile_definitions(openIndy PRIVATE OI_MAIN_PROGRAM)
target_include_directories(openIndy PRIVATE "${CMAKE_BINARY_DIR}/generated")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(openIndy PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(openIndy PRIVATE OpenIndyApp)

# make sure the plugin is actually built before openIndy's POST_BUILD step
# below tries to package it (a plain add_subdirectory/FetchContent dependency
# alone doesn't order a *targeted* `--target openIndy` build against it)
add_dependencies(openIndy OpenIndyDefaultPlugin)

# runtime resources the app expects next to its own executable (qmake used
# to copy these on `make install`; mirror that here so a plain build is
# directly runnable without a separate install step)
add_custom_command(TARGET openIndy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res-rt/config" "$<TARGET_FILE_DIR:openIndy>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res-rt/example" "$<TARGET_FILE_DIR:openIndy>/example"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res-rt/plugins" "$<TARGET_FILE_DIR:openIndy>/plugins"
    # package the default plugin itself - PluginRepository scans plugins/
    # directly, no separate registration step needed
    COMMAND ${CMAKE_COMMAND}
        -DPLUGIN_SO=$<TARGET_FILE:OpenIndyDefaultPlugin>
        -DAPP_DIR=$<TARGET_FILE_DIR:openIndy>
        -P "${openindydefaultplugin_SOURCE_DIR}/cmake/DeployToApp.cmake"
    VERBATIM
)
