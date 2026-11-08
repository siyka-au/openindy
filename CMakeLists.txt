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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg SerialPort Network WebSockets LinguistTools)

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

# --- Leica plugin dependency ---
# A first-class instrument family living entirely outside the core repos -
# Stage 5 of the OpenIndy plan. Same FetchContent/local-dir-override shape
# as the default plugin above; PluginRepository scans plugins/ for every
# .so it finds, so a second plugin needs nothing beyond being built and
# deployed there.

set(OI_LEICA_PLUGIN_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../openindy-leica-plugin" CACHE PATH
    "Local OpenIndy-LeicaPlugin checkout to build against instead of fetching from git")

if(EXISTS "${OI_LEICA_PLUGIN_LOCAL_DIR}/CMakeLists.txt")
    FetchContent_Declare(OpenIndyLeicaPlugin SOURCE_DIR "${OI_LEICA_PLUGIN_LOCAL_DIR}")
else()
    FetchContent_Declare(OpenIndyLeicaPlugin
        GIT_REPOSITORY git@github.com:siyka-au/openindy-leica-plugin.git
        GIT_TAG        master
    )
endif()
FetchContent_MakeAvailable(OpenIndyLeicaPlugin)

# --- appui library ---

add_library(OpenIndyApp SHARED
    controller/clipboard_util.cpp
    controller/qt_job_adapter.cpp
    controller/qt_job_adapter.h
    controller/qt_function_adapter.cpp
    controller/qt_function_adapter.h
    controller/qt_station_adapter.cpp
    controller/qt_station_adapter.h
    controller/qt_executor.cpp
    controller/qt_executor.h
    controller/config/feature_table_column_config.cpp
    controller/config/feature_table_sorting_config.cpp
    controller/config/measurement_config_manager.cpp
    controller/config/observation_table_column_config.cpp
    controller/config/parameter_display_config.cpp
    controller/config/project_config.cpp
    controller/config/reading_table_column_config.cpp
    controller/config/sensor_configuration_manager.cpp
    controller/config/trafo_param_table_column_config.cpp
    controller/console.cpp
    controller/tool.cpp
    controller/tool.h
    controller/job_connect.h
    controller/controller.cpp
    controller/exchange/data_exchanger.cpp
    controller/exchange/observation_importer.cpp
    controller/exchange/project_exchanger.cpp
    controller/feature_sorter.cpp
    controller/feature_updater.cpp
    controller/feature_util.cpp
    controller/measure_behavior_logic.cpp
    controller/model_manager.cpp
    controller/network/oi_request_handler.cpp
    controller/network/oi_web_socket.cpp
    controller/network/oi_web_socket_server.cpp
    controller/network/oi_web_socket_streamer.cpp
    controller/oi_meta_data.cpp
    controller/plugin_copier.cpp
    controller/plugin_manager.cpp
    controller/plugin_host.cpp
    controller/qt_progress_sink.cpp
    controller/plugin_repository.cpp
    controller/settings.cpp
    controller/sort_list_by_name.cpp
    controller/trafo_controller.cpp
    models/active_feature_functions_model.cpp
    models/available_elements_tree_view_proxy_model.cpp
    models/available_functions_list_model.cpp
    models/available_functions_list_proxy_model.cpp
    models/bundle_geometries_model.cpp
    models/bundle_parameter_table_proxy_model.cpp
    models/bundle_stations_model.cpp
    models/feature_difference_proxy_model.cpp
    models/feature_difference_table_model.cpp
    models/feature_table_model.cpp
    models/feature_table_proxy_model.cpp
    models/feature_tree_view_model.cpp
    models/function_statistic_model.cpp
    models/function_table_model.cpp
    models/function_table_proxy_model.cpp
    models/function_weight_delegate.cpp
    models/function_weight_proxy_model.cpp
    models/function_weights_table_model.cpp
    models/measurement_configuration_model.cpp
    models/measurement_configuration_proxy_model.cpp
    models/observation_model.cpp
    models/observation_proxy_model.cpp
    models/plugin_tree_view_model.cpp
    models/reading_model.cpp
    models/reading_proxy_model.cpp
    models/sensor_accuracy_model.cpp
    models/sensor_configuration_model.cpp
    models/sensor_configuration_proxy_model.cpp
    models/sensor_parameters_model.cpp
    models/sensor_table_model.cpp
    models/sensor_table_proxy_model.cpp
    models/trafo_param_table_proxy_model.cpp
    models/treeitems/bundle_geometry_item.cpp
    models/treeitems/bundle_station_item.cpp
    models/treeitems/feature_tree_item.cpp
    models/treeitems/function_tree_item.cpp
    models/treeitems/plugin_tree_item.cpp
    models/used_elements_model.cpp
    ui/actions/tool_action.cpp
    ui/delegates/bundle_overview_delegate.cpp
    ui/delegates/feature_table_delegate.cpp
    ui/delegates/measurement_configuration_list_delegate.cpp
    ui/delegates/observation_table_delegate.cpp
    ui/delegates/reading_table_delegate.cpp
    ui/delegates/sensor_accuracy_delegate.cpp
    ui/delegates/sensor_configuration_list_delegate.cpp
    ui/delegates/sensor_parameters_delegate.cpp
    ui/delegates/trafo_param_table_delegate.cpp
    ui/dialogs/about_dialog.cpp
    ui/dialogs/actual_properties_dialog.cpp
    ui/dialogs/create_feature_dialog.cpp
    ui/dialogs/export_dialog.cpp
    ui/dialogs/feature_functions_dialog.cpp
    ui/dialogs/import_nominal_dialog.cpp
    ui/dialogs/loading_dialog.cpp
    ui/dialogs/measure_behavior_dialog.cpp
    ui/dialogs/measurement_configuration_dialog.cpp
    ui/dialogs/move_sensor_dialog.cpp
    ui/dialogs/nominal_properties_dialog.cpp
    ui/dialogs/plugin_loader_dialog.cpp
    ui/dialogs/plugin_manager_dialog.cpp
    ui/dialogs/sensor_configuration_dialog.cpp
    ui/dialogs/sensor_task_info_dialog.cpp
    ui/dialogs/settings_dialog.cpp
    ui/dialogs/show_licenses_dialog.cpp
    ui/dialogs/station_properties_dialog.cpp
    ui/dialogs/trafo_param_properties_dialog.cpp
    ui/dialogs/gui_util.cpp
    ui/dialogs/watch_window_dialog.cpp
    ui/dialogs/watch_window_util.cpp
    ui/main_window.cpp
    ui/selectionmodels/multi_selection_model.cpp
    ui/widgets/plugin_info_widget.cpp
    ui/widgets/scalar_parameter_widget.cpp
    ui/widgets/station_status.cpp

    controller/clipboard_util.h
    controller/config/feature_table_column_config.h
    controller/config/feature_table_sorting_config.h
    controller/config/measurement_config_manager.h
    controller/config/observation_table_column_config.h
    controller/config/parameter_display_config.h
    controller/config/project_config.h
    controller/config/reading_table_column_config.h
    controller/config/sensor_configuration_manager.h
    controller/config/trafo_param_table_column_config.h
    controller/console.h
    controller/controller.h
    controller/controller_sensor_actions.h
    controller/exchange/data_exchanger.h
    controller/exchange/observation_importer.h
    controller/exchange/project_exchanger.h
    controller/feature_sorter.h
    controller/feature_updater.h
    controller/feature_util.h
    controller/measure_behavior_logic.h
    controller/measure_behavior_types.h
    controller/model_manager.h
    controller/network/oi_request_handler.h
    controller/network/oi_web_socket.h
    controller/network/oi_web_socket_server.h
    controller/network/oi_web_socket_streamer.h
    controller/param/exchange_params.h
    controller/plugin_copier.h
    controller/plugin_manager.h
    controller/plugin_host.h
    controller/qt_progress_sink.h
    controller/plugin_info.h
    controller/plugin_repository.h
    controller/settings.h
    controller/sort_list_by_name.h
    controller/trafo_controller.h
    models/active_feature_functions_model.h
    models/available_elements_tree_view_proxy_model.h
    models/available_functions_list_model.h
    models/available_functions_list_proxy_model.h
    models/bundle_geometries_model.h
    models/bundle_parameter_table_proxy_model.h
    models/bundle_stations_model.h
    models/feature_difference_proxy_model.h
    models/feature_difference_table_model.h
    models/feature_table_model.h
    models/feature_table_proxy_model.h
    models/feature_tree_view_model.h
    models/function_statistic_model.h
    models/function_table_model.h
    models/function_table_proxy_model.h
    models/function_weight_delegate.h
    models/function_weight_proxy_model.h
    models/function_weights_table_model.h
    models/measurement_configuration_model.h
    models/measurement_configuration_proxy_model.h
    models/observation_model.h
    models/observation_proxy_model.h
    models/plugin_tree_view_model.h
    models/reading_model.h
    models/reading_proxy_model.h
    models/sensor_accuracy_model.h
    models/sensor_configuration_model.h
    models/sensor_configuration_proxy_model.h
    models/sensor_parameters_model.h
    models/sensor_table_model.h
    models/sensor_table_proxy_model.h
    models/trafo_param_table_proxy_model.h
    models/treeitems/bundle_geometry_item.h
    models/treeitems/bundle_station_item.h
    models/treeitems/feature_tree_item.h
    models/treeitems/function_tree_item.h
    models/treeitems/plugin_tree_item.h
    models/used_elements_model.h
    ui/actions/tool_action.h
    ui/delegates/bundle_overview_delegate.h
    ui/delegates/feature_table_delegate.h
    ui/delegates/measurement_configuration_list_delegate.h
    ui/delegates/observation_table_delegate.h
    ui/delegates/reading_table_delegate.h
    ui/delegates/sensor_accuracy_delegate.h
    ui/delegates/sensor_configuration_list_delegate.h
    ui/delegates/sensor_parameters_delegate.h
    ui/delegates/trafo_param_table_delegate.h
    ui/dialogs/about_dialog.h
    ui/dialogs/actual_properties_dialog.h
    ui/dialogs/create_feature_dialog.h
    ui/dialogs/export_dialog.h
    ui/dialogs/feature_functions_dialog.h
    ui/dialogs/import_nominal_dialog.h
    ui/dialogs/loading_dialog.h
    ui/dialogs/measure_behavior_dialog.h
    ui/dialogs/measurement_configuration_dialog.h
    ui/dialogs/move_sensor_dialog.h
    ui/dialogs/nominal_properties_dialog.h
    ui/dialogs/plugin_loader_dialog.h
    ui/dialogs/plugin_manager_dialog.h
    ui/dialogs/sensor_configuration_dialog.h
    ui/dialogs/sensor_task_info_dialog.h
    ui/dialogs/settings_dialog.h
    ui/dialogs/show_licenses_dialog.h
    ui/dialogs/station_properties_dialog.h
    ui/dialogs/trafo_param_properties_dialog.h
    ui/dialogs/gui_util.h
    ui/dialogs/watch_window_dialog.h
    ui/dialogs/watch_window_util.h
    ui/main_window.h
    ui/selectionmodels/multi_selection_model.h
    ui/widgets/plugin_info_widget.h
    ui/widgets/scalar_parameter_widget.h
    ui/widgets/station_status.h

    ui/dialogs/about_dialog.ui
    ui/dialogs/actual_properties_dialog.ui
    ui/dialogs/create_feature_dialog.ui
    ui/dialogs/export_dialog.ui
    ui/dialogs/feature_functions_dialog.ui
    ui/dialogs/import_nominal_dialog.ui
    ui/dialogs/loading_dialog.ui
    ui/dialogs/measure_behavior_dialog.ui
    ui/dialogs/measurement_configuration_dialog.ui
    ui/dialogs/move_sensor_dialog.ui
    ui/dialogs/nominal_properties_dialog.ui
    ui/dialogs/plugin_loader_dialog.ui
    ui/dialogs/plugin_manager_dialog.ui
    ui/dialogs/sensor_configuration_dialog.ui
    ui/dialogs/sensor_task_info_dialog.ui
    ui/dialogs/settings_dialog.ui
    ui/dialogs/show_licenses_dialog.ui
    ui/dialogs/station_properties_dialog.ui
    ui/dialogs/trafo_param_properties_dialog.ui
    ui/dialogs/watch_window_dialog.ui
    ui/main_window.ui
    ui/widgets/station_status.ui

    res/res.qrc
)

qt_add_translations(OpenIndyApp
    TS_FILES
    i18n/openindy_en_AU.ts
    RESOURCE_PREFIX "/i18n"
)

# If you are deploying to Linux (e.g., via /usr/share/) or an embedded system, you might want to\
# install .qm files as external assets so Linux package managers can handle them.
#
# MyProject/
# ├── CMakeLists.txt
# ├── main.cpp
# └── translations/
#     ├── myproject_en.ts
#     └── myproject_fr.ts
#
# qt_add_translations(MyProject
#     TS_FILES
#         translations/myproject_en.ts
#         translations/myproject_fr.ts
#     OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations"
# )
# # Install them to the system directory during 'make install'
# install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations/"
#         DESTINATION "share/myproject/translations")

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
        Qt6::WebSockets
        OpenIndyCore
        OpenIndyMath
    PRIVATE
        # the protobuf project format; its runtime lives in this library only
        OpenIndyCodec
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

# make sure the plugins are actually built before openIndy's POST_BUILD step
# below tries to package them (a plain add_subdirectory/FetchContent dependency
# alone doesn't order a *targeted* `--target openIndy` build against it)
add_dependencies(openIndy OpenIndyDefaultPlugin OpenIndyLeicaPlugin)

# runtime resources the app expects next to its own executable (qmake used
# to copy these on `make install`; mirror that here so a plain build is
# directly runnable without a separate install step)
add_custom_command(TARGET openIndy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res-rt/config" "$<TARGET_FILE_DIR:openIndy>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res-rt/plugins" "$<TARGET_FILE_DIR:openIndy>/plugins"
    # package each plugin itself - PluginRepository scans plugins/
    # directly, no separate registration step needed
    COMMAND ${CMAKE_COMMAND}
        -DPLUGIN_SO=$<TARGET_FILE:OpenIndyDefaultPlugin>
        -DAPP_DIR=$<TARGET_FILE_DIR:openIndy>
        -P "${openindydefaultplugin_SOURCE_DIR}/cmake/DeployToApp.cmake"
    COMMAND ${CMAKE_COMMAND}
        -DPLUGIN_SO=$<TARGET_FILE:OpenIndyLeicaPlugin>
        -DAPP_DIR=$<TARGET_FILE_DIR:openIndy>
        -P "${openindyleicaplugin_SOURCE_DIR}/cmake/DeployToApp.cmake"
    VERBATIM
)

# --- tests ---
#
# These suites predate the qmake->CMake migration and were never carried over,
# so the app had no automated verification at all. The sources are the asset;
# the old .pro files referenced a layout that no longer exists.

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Test)

# pure logic, no GUI
add_executable(tst_featuresorter test/feature_sorter/feature_sorter_test.cpp)
target_compile_definitions(tst_featuresorter PRIVATE
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/feature_sorter")
target_link_libraries(tst_featuresorter PRIVATE Qt6::Test OpenIndyApp OpenIndyCore)
add_test(NAME featuresorter COMMAND tst_featuresorter)

# mock-based; exercises the controller's sensor-action surface
add_executable(tst_measurebehavior
    test/measure_behavior/measure_behavior_test.cpp
    test/measure_behavior/test_controller_sensor_actions.cpp
    test/measure_behavior/test_measure_behavior_logic.cpp
)
target_include_directories(tst_measurebehavior PRIVATE test/measure_behavior)
target_compile_definitions(tst_measurebehavior PRIVATE
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/measure_behavior")
target_link_libraries(tst_measurebehavior PRIVATE Qt6::Test OpenIndyApp OpenIndyCore)
add_test(NAME measurebehavior COMMAND tst_measurebehavior)

# GUI suites - need an offscreen platform plugin in headless environments
add_executable(tst_watchwindow test/watch_window/tst_watch_window_test.cpp)
target_compile_definitions(tst_watchwindow PRIVATE
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/watch_window")
target_link_libraries(tst_watchwindow PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_test(NAME watchwindow COMMAND tst_watchwindow)
set_tests_properties(watchwindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_dialogs test/dialogs/dialogs_test.cpp)
target_compile_definitions(tst_dialogs PRIVATE
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/dialogs")
target_link_libraries(tst_dialogs PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyCodec)
add_test(NAME dialogs COMMAND tst_dialogs)
set_tests_properties(dialogs PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# pure logic, no GUI - Group/PointGroup save/load through the real
# ProjectExchanger entry points (Stage 7e)
add_executable(tst_projectexchange test/project_exchange/tst_project_exchange.cpp)
target_link_libraries(tst_projectexchange PRIVATE Qt6::Test OpenIndyApp OpenIndyCore)
add_test(NAME projectexchange COMMAND tst_projectexchange)

# websocket remote-control protocol (protobuf): requests go into
# OiRequestHandler as bytes, the answers are decoded from what it sends back
add_executable(tst_requesthandler test/request_handler/tst_request_handler.cpp)
target_link_libraries(tst_requesthandler PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyCodec)
add_test(NAME requesthandler COMMAND tst_requesthandler)

# config files (JSON): the shipped ones load and say what the old config files said;
# bad files fall back
add_executable(tst_configfiles test/config_files/tst_config_files.cpp)
target_compile_definitions(tst_configfiles PRIVATE
    RESDIR="${CMAKE_CURRENT_SOURCE_DIR}/res-rt/config")
target_include_directories(tst_configfiles PRIVATE controller/config)
target_link_libraries(tst_configfiles PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyCodec)
add_test(NAME configfiles COMMAND tst_configfiles)

# UI verification checkpoint - Controller::generateOwnedNormalLine()/
# generateOwnedOriginPoint(), the new feature-table context-menu actions,
# through a real PluginRepository scan (not ModelManager::testInit()'s fake
# seam), so this needs the plugins/ directory the post-build copy step
# already deploys next to every test binary in this file
add_executable(tst_ownedsubgeometryui test/owned_subgeometry_ui/tst_owned_subgeometry_ui.cpp)
target_link_libraries(tst_ownedsubgeometryui PRIVATE Qt6::Test OpenIndyApp OpenIndyCore)
add_test(NAME ownedsubgeometryui COMMAND tst_ownedsubgeometryui)

# QtJobAdapter: the OiJob observer turned back into the Qt signals
add_executable(tst_qtjobadapter test/qt_job_adapter/tst_qt_job_adapter.cpp)
target_link_libraries(tst_qtjobadapter PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_test(NAME qtjobadapter COMMAND tst_qtjobadapter)

# QtFunctionAdapter: the Function observer turned back into the Qt signals
add_executable(tst_qtfunctionadapter test/qt_function_adapter/tst_qt_function_adapter.cpp)
target_link_libraries(tst_qtfunctionadapter PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_test(NAME qtfunctionadapter COMMAND tst_qtfunctionadapter)

# QtStationAdapter / QtExecutor: the sensor path's observer and executor on the UI thread
add_executable(tst_qtstationadapter test/qt_station_adapter/tst_qt_station_adapter.cpp)
target_link_libraries(tst_qtstationadapter PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_test(NAME qtstationadapter COMMAND tst_qtstationadapter)

# DataExchanger: reports of an exchange plugin's worker thread reach the UI thread
add_executable(tst_dataexchanger test/data_exchanger/tst_data_exchanger.cpp)
target_link_libraries(tst_dataexchanger PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_test(NAME dataexchanger COMMAND tst_dataexchanger)

# compute plugins are loaded through core's oi::PluginLoader (PluginHost)
add_executable(tst_pluginhost test/plugin_host/tst_plugin_host.cpp)
target_compile_definitions(tst_pluginhost PRIVATE
    DEFAULT_PLUGIN_PATH="$<TARGET_FILE:OpenIndyDefaultPlugin>"
    LEICA_PLUGIN_PATH="$<TARGET_FILE:OpenIndyLeicaPlugin>"
    PLUGIN_DIR="$<TARGET_FILE_DIR:OpenIndyDefaultPlugin>"
    NOT_A_PLUGIN_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/plugin_host/tst_plugin_host.cpp")
target_link_libraries(tst_pluginhost PRIVATE Qt6::Test OpenIndyApp OpenIndyCore OpenIndyMath)
add_dependencies(tst_pluginhost OpenIndyDefaultPlugin OpenIndyLeicaPlugin)
add_test(NAME pluginhost COMMAND tst_pluginhost)
