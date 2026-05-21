cmake_minimum_required(VERSION 3.20)
project(raytracer VERSION 3.0.0 LANGUAGES CXX)
option(RAYTRACER_BONUS "Build advanced rendering extensions from bonus/" ON)
option(RAYTRACER_WERROR "Treat every compiler warning as an error" ON)
option(RAYTRACER_SANITIZE "Enable address and undefined behavior sanitizers" OFF)
option(RAYTRACER_REQUIRE_GUI "Require both native studio and progressive viewer dependencies" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Unique inline objects otherwise mark plugins NODELETE and retain their allocations.
  add_compile_options(-fno-gnu-unique)
endif()
file(GLOB_RECURSE RT_CORE CONFIGURE_DEPENDS src/*.cpp)
list(FILTER RT_CORE EXCLUDE REGEX "/main.cpp$")
add_library(raytracer_engine STATIC ${RT_CORE})
target_include_directories(raytracer_engine PUBLIC include)
target_compile_options(raytracer_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
if(RAYTRACER_WERROR)
  target_compile_options(raytracer_engine PUBLIC -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
if(RAYTRACER_BONUS)
  target_compile_definitions(raytracer_engine PRIVATE RT_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  find_package(Threads REQUIRED)
  file(GLOB_RECURSE RT_EXTENSIONS CONFIGURE_DEPENDS bonus/src/*.cpp)
  target_sources(raytracer_engine PRIVATE ${RT_EXTENSIONS})
  target_include_directories(raytracer_engine PUBLIC bonus/include)
  target_compile_definitions(raytracer_engine PUBLIC RT_BONUS=1)
  target_link_libraries(raytracer_engine PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
  set_target_properties(raytracer_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
  find_package(SDL2 2.0.17 QUIET CONFIG)
  find_package(ZLIB QUIET)
  if(SDL2_FOUND AND ZLIB_FOUND)
    add_library(raytracer_imgui STATIC
      bonus/vendor/imgui/imgui.cpp bonus/vendor/imgui/imgui_draw.cpp
      bonus/vendor/imgui/imgui_tables.cpp bonus/vendor/imgui/imgui_widgets.cpp
      bonus/vendor/imgui/backends/imgui_impl_sdl2.cpp
      bonus/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
      bonus/vendor/imgui/misc/cpp/imgui_stdlib.cpp)
    target_include_directories(raytracer_imgui SYSTEM PUBLIC bonus/vendor/imgui)
    target_link_libraries(raytracer_imgui PUBLIC SDL2::SDL2)
    set_target_properties(raytracer_imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(raytracer_engine PUBLIC raytracer_imgui ZLIB::ZLIB)
    target_compile_definitions(raytracer_engine PUBLIC RT_NATIVE_STUDIO=1)
  endif()
  find_package(SFML 3 COMPONENTS Graphics Window System QUIET CONFIG)
  if(SFML_FOUND)
    target_compile_definitions(raytracer_engine PUBLIC RT_SFML=1)
    target_link_libraries(raytracer_engine PUBLIC SFML::Graphics SFML::Window SFML::System)
  else()
    find_package(SFML 2.5 COMPONENTS graphics window system QUIET)
    if(SFML_FOUND)
      target_compile_definitions(raytracer_engine PUBLIC RT_SFML=1)
      target_link_libraries(raytracer_engine PUBLIC sfml-graphics sfml-window sfml-system)
    endif()
  endif()
endif()
if(RAYTRACER_REQUIRE_GUI AND (NOT RAYTRACER_BONUS OR NOT SDL2_FOUND OR NOT ZLIB_FOUND OR NOT SFML_FOUND))
  message(FATAL_ERROR "RAYTRACER_REQUIRE_GUI needs bonuses, SDL2 >= 2.0.17, zlib and SFML >= 2.5")
endif()
if(RAYTRACER_SANITIZE)
  target_compile_options(raytracer_engine PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(raytracer_engine PUBLIC -fsanitize=address,undefined)
endif()
if(RAYTRACER_BONUS)
  add_library(raytracer_example MODULE bonus/plugins/PluginSphere.cpp)
  target_link_libraries(raytracer_example PRIVATE raytracer_engine)
  add_custom_target(stage_plugins ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raytracer_example> ${CMAKE_SOURCE_DIR}/plugins/raytracer_example.so
    DEPENDS raytracer_example)
  set_target_properties(raytracer_example PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
endif()
add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer PRIVATE raytracer_engine)
set_target_properties(raytracer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# Stage the selected build at the required repository root on every normal build.
# Keeping the actual target in its build directory prevents core/bonus test collisions.
add_custom_target(stage_raytracer ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raytracer> ${CMAKE_SOURCE_DIR}/raytracer
  DEPENDS raytracer)
if(APPLE AND RAYTRACER_BONUS AND SDL2_FOUND AND ZLIB_FOUND)
  add_executable(raytracer_studio MACOSX_BUNDLE bonus/app/main.cpp)
  target_link_libraries(raytracer_studio PRIVATE raytracer_engine)
  set_target_properties(raytracer_studio PROPERTIES
    OUTPUT_NAME "Atelier"
    MACOSX_BUNDLE_BUNDLE_NAME "Atelier Raytracer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.epitech.raytracer.studio"
    MACOSX_BUNDLE_BUNDLE_VERSION "3.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "3.0")
  add_dependencies(raytracer_studio raytracer_example)
  add_custom_command(TARGET raytracer_studio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/bonus/assets"
      "$<TARGET_BUNDLE_CONTENT_DIR:raytracer_studio>/Resources/bonus/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/bonus/scenes"
      "$<TARGET_BUNDLE_CONTENT_DIR:raytracer_studio>/Resources/bonus/scenes"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:raytracer_studio>/Resources/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:raytracer_example>
      "$<TARGET_BUNDLE_CONTENT_DIR:raytracer_studio>/Resources/plugins/raytracer_example.so")
endif()
include(CTest)
if(BUILD_TESTING)
  if(RAYTRACER_BONUS)
    add_executable(thread_lifecycle_tests tests/thread_lifecycle.cpp)
    target_link_libraries(thread_lifecycle_tests PRIVATE raytracer_engine)
    add_test(NAME thread_lifecycle COMMAND thread_lifecycle_tests)
    add_library(bad_abi MODULE tests/plugins/BadAbi.cpp)
    target_compile_options(bad_abi PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
    set_target_properties(bad_abi PROPERTIES PREFIX "" SUFFIX ".so" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test_plugins")
    add_executable(extended_tests tests/extended.cpp)
    target_link_libraries(extended_tests PRIVATE raytracer_engine)
    add_dependencies(extended_tests raytracer_example)
    add_test(NAME extended_features COMMAND extended_tests ${CMAKE_SOURCE_DIR} $<TARGET_FILE:raytracer_example>)
    add_executable(plugin_lifecycle_tests tests/plugin_lifecycle.cpp)
    target_link_libraries(plugin_lifecycle_tests PRIVATE raytracer_engine)
    add_dependencies(plugin_lifecycle_tests raytracer_example bad_abi)
    add_test(NAME plugin_lifecycle COMMAND plugin_lifecycle_tests
      $<TARGET_FILE:raytracer_example> $<TARGET_FILE:bad_abi>)
    add_executable(lighting_quality_tests tests/lighting_quality.cpp)
    target_link_libraries(lighting_quality_tests PRIVATE raytracer_engine)
    add_dependencies(lighting_quality_tests raytracer_example)
    add_test(NAME lighting_quality COMMAND lighting_quality_tests $<TARGET_FILE:raytracer_example>)
    add_executable(native_studio_tests tests/native_studio.cpp)
    target_link_libraries(native_studio_tests PRIVATE raytracer_engine)
    add_test(NAME native_studio_model COMMAND native_studio_tests ${CMAKE_SOURCE_DIR})
    add_executable(studio_controls_tests tests/studio_controls.cpp)
    target_link_libraries(studio_controls_tests PRIVATE raytracer_engine)
    add_test(NAME studio_controls COMMAND studio_controls_tests ${CMAKE_SOURCE_DIR})
    add_executable(pbr_tests tests/pbr.cpp)
    target_link_libraries(pbr_tests PRIVATE raytracer_engine)
    add_test(NAME pbr_physics COMMAND pbr_tests)
    add_executable(legacy_adapter_tests tests/testLegacyAdapter.cpp)
    target_link_libraries(legacy_adapter_tests PRIVATE raytracer_engine)
    add_test(NAME legacy_adapter COMMAND legacy_adapter_tests ${CMAKE_SOURCE_DIR}/tests/legacy)
  endif()
  add_executable(performance_regression_tests tests/performance_regressions.cpp)
  target_link_libraries(performance_regression_tests PRIVATE raytracer_engine)
  add_test(NAME performance_regressions COMMAND performance_regression_tests)
  add_executable(raytracer_tests tests/tests.cpp)
  target_link_libraries(raytracer_tests PRIVATE raytracer_engine)
  add_test(NAME raytracer_unit COMMAND raytracer_tests)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    if(RAYTRACER_BONUS)
      add_executable(resource_paths_tests tests/asset_paths.cpp)
      target_link_libraries(resource_paths_tests PRIVATE raytracer_engine)
      add_test(NAME resource_paths COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tests/resource_paths.py $<TARGET_FILE:resource_paths_tests> ${CMAKE_SOURCE_DIR})
      add_test(NAME systems_features COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/systems.py $<TARGET_FILE:raytracer> ${CMAKE_SOURCE_DIR} $<TARGET_FILE:bad_abi> $<TARGET_FILE:raytracer_example>)
      find_program(XVFB_RUN xvfb-run)
      if(SDL2_FOUND AND ZLIB_FOUND AND XVFB_RUN)
        add_test(NAME native_studio_window COMMAND ${XVFB_RUN} -a ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/native_gui.py $<TARGET_FILE:raytracer>)
      endif()
      if(SFML_FOUND AND XVFB_RUN)
        add_test(NAME gui_lifecycle COMMAND ${XVFB_RUN} -a ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/gui.py $<TARGET_FILE:raytracer>)
      endif()
    endif()
    add_test(NAME raytracer_cli COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/integration.py $<TARGET_FILE:raytracer> ${CMAKE_SOURCE_DIR} ${RAYTRACER_BONUS})
  endif()
endif()

if(BUILD_TESTING)
  get_property(RT_TESTS DIRECTORY PROPERTY TESTS)
  set_tests_properties(${RT_TESTS} PROPERTIES TIMEOUT 120)
endif()

if(BUILD_TESTING AND Python3_FOUND AND NOT RAYTRACER_SANITIZE)
  find_program(VALGRIND_EXECUTABLE valgrind)
  if(VALGRIND_EXECUTABLE)
    function(rt_memory name target)
      add_test(NAME memory_${name} COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/tools/memory_check.py" --valgrind "${VALGRIND_EXECUTABLE}"
        --log "${CMAKE_BINARY_DIR}/memcheck/${name}" -- $<TARGET_FILE:${target}> ${ARGN})
      set_tests_properties(memory_${name} PROPERTIES TIMEOUT 300)
    endfunction()
    rt_memory(unit raytracer_tests)
    rt_memory(performance performance_regression_tests)
    rt_memory(cli raytracer "${CMAKE_SOURCE_DIR}/tests/scenes/memory-core.json")
    add_test(NAME memory_cli_error COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_SOURCE_DIR}/tools/memory_check.py" --valgrind "${VALGRIND_EXECUTABLE}"
      --log "${CMAKE_BINARY_DIR}/memcheck/cli_error" --expect-exit 84 -- $<TARGET_FILE:raytracer>
      "${CMAKE_BINARY_DIR}/memcheck/missing-input.json")
    set_tests_properties(memory_cli_error PROPERTIES TIMEOUT 300)
    if(RAYTRACER_BONUS)
      rt_memory(thread_lifecycle thread_lifecycle_tests)
      rt_memory(pbr pbr_tests)
      rt_memory(extended extended_tests "${CMAKE_SOURCE_DIR}" $<TARGET_FILE:raytracer_example>)
      rt_memory(lighting lighting_quality_tests $<TARGET_FILE:raytracer_example>)
      rt_memory(studio_model native_studio_tests "${CMAKE_SOURCE_DIR}")
      rt_memory(studio_controls studio_controls_tests "${CMAKE_SOURCE_DIR}")
      rt_memory(legacy legacy_adapter_tests "${CMAKE_SOURCE_DIR}/tests/legacy")
      rt_memory(plugin_lifecycle plugin_lifecycle_tests
        $<TARGET_FILE:raytracer_example> $<TARGET_FILE:bad_abi>)
      if(SDL2_FOUND AND ZLIB_FOUND AND XVFB_RUN)
        add_test(NAME memory_native_window COMMAND ${CMAKE_COMMAND} -E env
          GALLIUM_DRIVER=softpipe SDL_FRAMEBUFFER_ACCELERATION=0
          DBUS_SESSION_BUS_ADDRESS=unix:path=/tmp/epitech-memory-no-session-bus
          ${XVFB_RUN} -a ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/memory_check.py"
          --valgrind "${VALGRIND_EXECUTABLE}" --allow-reachable
          --log "${CMAKE_BINARY_DIR}/memcheck/native_window" -- $<TARGET_FILE:raytracer>
          --studio "${CMAKE_SOURCE_DIR}/tests/scenes/memory.json" --frames 3
          --screenshot "${CMAKE_BINARY_DIR}/memcheck/native-frame.png")
        set_tests_properties(memory_native_window PROPERTIES TIMEOUT 300)
      endif()
      if(SFML_FOUND AND XVFB_RUN)
        add_test(NAME memory_sfml_window COMMAND ${CMAKE_COMMAND} -E env
          GALLIUM_DRIVER=softpipe DBUS_SESSION_BUS_ADDRESS=unix:path=/tmp/epitech-memory-no-session-bus
          ${XVFB_RUN} -a ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/memory_check.py"
          --valgrind "${VALGRIND_EXECUTABLE}" --allow-reachable
          --log "${CMAKE_BINARY_DIR}/memcheck/sfml_window" -- $<TARGET_FILE:raytracer>
          --gui --frames 3 "${CMAKE_SOURCE_DIR}/tests/scenes/memory.json")
        set_tests_properties(memory_sfml_window PROPERTIES TIMEOUT 300)
      endif()

    endif()
  endif()
endif()
