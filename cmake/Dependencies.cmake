set(_ASTRAGLYPH_SDL3_VERSION "3.2.16")
set(_ASTRAGLYPH_SDL3_ARCHIVE_NAME "SDL3-${_ASTRAGLYPH_SDL3_VERSION}.zip")
set(_ASTRAGLYPH_SDL3_SOURCE_DIRNAME "SDL3-${_ASTRAGLYPH_SDL3_VERSION}")
set(_ASTRAGLYPH_SDL3_URL "https://github.com/libsdl-org/SDL/releases/download/release-${_ASTRAGLYPH_SDL3_VERSION}/${_ASTRAGLYPH_SDL3_ARCHIVE_NAME}")
set(_ASTRAGLYPH_SDL3_SHA256 "0cc7430fb827c1f843e31b8b26ba7f083b1eeb8f6315a65d3744fd4d25b6c373")

function(astraglyph_make_generator_key input out_var)
  string(REGEX REPLACE "[^A-Za-z0-9]+" "-" key "${input}")
  string(TOLOWER "${key}" key)
  if("${key}" STREQUAL "")
    set(key "default")
  endif()
  set(${out_var} "${key}" PARENT_SCOPE)
endfunction()

function(astraglyph_resolve_c_compiler out_var)
  if(CMAKE_C_COMPILER)
    set(resolved_c_compiler "${CMAKE_C_COMPILER}")
  else()
    get_filename_component(cxx_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(
      resolved_c_compiler
      NAMES gcc cc clang
      PATHS "${cxx_compiler_dir}"
      NO_DEFAULT_PATH
    )
    if(NOT resolved_c_compiler)
      find_program(resolved_c_compiler NAMES gcc cc clang REQUIRED)
    endif()
  endif()

  set(${out_var} "${resolved_c_compiler}" PARENT_SCOPE)
endfunction()

function(astraglyph_sdl3_bootstrap_signature out_var)
  set(signature_lines
    "SDL3_VERSION=${_ASTRAGLYPH_SDL3_VERSION}"
    "SDL_SHARED=OFF"
    "SDL_STATIC=ON"
    "SDL_INSTALL=ON"
    "SDL_TEST_LIBRARY=OFF"
    "SDL_TESTS=OFF"
    "SDL_EXAMPLES=OFF"
    "SDL_AUDIO=OFF"
    "SDL_CAMERA=OFF"
    "SDL_JOYSTICK=OFF"
    "SDL_HAPTIC=OFF"
    "SDL_HIDAPI=OFF"
    "SDL_POWER=OFF"
    "SDL_SENSOR=OFF"
    "SDL_DIALOG=OFF"
    "SDL_GPU=OFF"
    "SDL_RENDER=ON"
    "SDL_VIDEO=ON"
    "SDL_OFFSCREEN=OFF"
    "SDL_OPENGL=OFF"
    "SDL_OPENGLES=OFF"
    "SDL_VULKAN=OFF"
    "SDL_RENDER_GPU=OFF"
    "SDL_DISKAUDIO=OFF"
    "SDL_DUMMYAUDIO=OFF"
    "SDL_DUMMYCAMERA=OFF"
    "SDL_VIRTUAL_JOYSTICK=OFF"
  )
  list(JOIN signature_lines "\n" signature)
  set(${out_var} "${signature}" PARENT_SCOPE)
endfunction()

function(astraglyph_is_local_sdl3_ready install_prefix expected_signature out_var)
  set(config_path "${install_prefix}/lib/cmake/SDL3/SDL3Config.cmake")
  set(stamp_path "${install_prefix}/astraglyph-sdl3.stamp")
  set(is_ready FALSE)

  if(EXISTS "${config_path}" AND EXISTS "${stamp_path}")
    file(READ "${stamp_path}" actual_signature)
    if("${actual_signature}" STREQUAL "${expected_signature}")
      set(is_ready TRUE)
    endif()
  endif()

  set(${out_var} ${is_ready} PARENT_SCOPE)
endfunction()

function(astraglyph_ensure_sdl3_archive archive_path)
  get_filename_component(archive_dir "${archive_path}" DIRECTORY)
  file(MAKE_DIRECTORY "${archive_dir}")

  if(EXISTS "${archive_path}")
    file(SHA256 "${archive_path}" actual_hash)
  else()
    set(actual_hash "")
  endif()

  if(NOT "${actual_hash}" STREQUAL "${_ASTRAGLYPH_SDL3_SHA256}")
    message(STATUS "Downloading SDL3 archive into persistent cache: ${archive_path}")
    if(EXISTS "${archive_path}")
      file(REMOVE "${archive_path}")
    endif()

    file(
      DOWNLOAD
      "${_ASTRAGLYPH_SDL3_URL}"
      "${archive_path}"
      EXPECTED_HASH "SHA256=${_ASTRAGLYPH_SDL3_SHA256}"
      SHOW_PROGRESS
      STATUS download_status
      TIMEOUT 120
      INACTIVITY_TIMEOUT 30
    )
    list(GET download_status 0 download_code)
    list(GET download_status 1 download_message)
    if(NOT download_code EQUAL 0)
      message(FATAL_ERROR "Failed to download SDL3 archive: ${download_message}")
    endif()
  endif()
endfunction()

function(astraglyph_ensure_sdl3_source dependency_root out_var)
  set(archive_path "${dependency_root}/downloads/${_ASTRAGLYPH_SDL3_ARCHIVE_NAME}")
  set(source_root "${dependency_root}/src")
  set(source_dir "${source_root}/${_ASTRAGLYPH_SDL3_SOURCE_DIRNAME}")

  astraglyph_ensure_sdl3_archive("${archive_path}")

  if(NOT EXISTS "${source_dir}/CMakeLists.txt")
    file(MAKE_DIRECTORY "${source_root}")
    if(EXISTS "${source_dir}")
      file(REMOVE_RECURSE "${source_dir}")
    endif()

    message(STATUS "Extracting SDL3 source into persistent cache: ${source_dir}")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E tar xf "${archive_path}"
      WORKING_DIRECTORY "${source_root}"
      RESULT_VARIABLE extract_result
      ERROR_VARIABLE extract_error
    )
    if(NOT extract_result EQUAL 0)
      message(FATAL_ERROR "Failed to extract SDL3 archive: ${extract_error}")
    endif()
  endif()

  set(${out_var} "${source_dir}" PARENT_SCOPE)
endfunction()

function(astraglyph_collect_sdl3_cmake_args install_prefix build_type out_var)
  astraglyph_resolve_c_compiler(resolved_c_compiler)

  set(args
    "-DCMAKE_BUILD_TYPE=${build_type}"
    "-DCMAKE_INSTALL_PREFIX=${install_prefix}"
    "-DCMAKE_C_COMPILER=${resolved_c_compiler}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DSDL_SHARED=OFF"
    "-DSDL_STATIC=ON"
    "-DSDL_INSTALL=ON"
    "-DSDL_TEST_LIBRARY=OFF"
    "-DSDL_TESTS=OFF"
    "-DSDL_EXAMPLES=OFF"
    "-DSDL_AUDIO=OFF"
    "-DSDL_CAMERA=OFF"
    "-DSDL_JOYSTICK=OFF"
    "-DSDL_HAPTIC=OFF"
    "-DSDL_HIDAPI=OFF"
    "-DSDL_POWER=OFF"
    "-DSDL_SENSOR=OFF"
    "-DSDL_DIALOG=OFF"
    "-DSDL_GPU=OFF"
    "-DSDL_RENDER=ON"
    "-DSDL_VIDEO=ON"
    "-DSDL_OFFSCREEN=OFF"
    "-DSDL_OPENGL=OFF"
    "-DSDL_OPENGLES=OFF"
    "-DSDL_VULKAN=OFF"
    "-DSDL_RENDER_GPU=OFF"
    "-DSDL_DISKAUDIO=OFF"
    "-DSDL_DUMMYAUDIO=OFF"
    "-DSDL_DUMMYCAMERA=OFF"
    "-DSDL_VIRTUAL_JOYSTICK=OFF"
  )

  if(CMAKE_MAKE_PROGRAM)
    list(APPEND args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
  endif()

  set(${out_var} "${args}" PARENT_SCOPE)
endfunction()

function(astraglyph_bootstrap_local_sdl3 dependency_root install_prefix build_type)
  astraglyph_ensure_sdl3_source("${dependency_root}" sdl3_source_dir)
  astraglyph_make_generator_key("${CMAKE_GENERATOR}" generator_key)
  string(TOLOWER "${build_type}" build_type_key)
  set(build_dir "${dependency_root}/build/${generator_key}/${build_type_key}/SDL3-${_ASTRAGLYPH_SDL3_VERSION}")

  astraglyph_collect_sdl3_cmake_args("${install_prefix}" "${build_type}" sdl3_cmake_args)
  astraglyph_sdl3_bootstrap_signature(signature)

  if(NOT EXISTS "${build_dir}")
    file(MAKE_DIRECTORY "${build_dir}")
  else()
    message(STATUS "Reusing existing SDL3 build directory: ${build_dir}")
  endif()

  message(STATUS "Bootstrapping persistent local SDL3 package: ${install_prefix}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${sdl3_source_dir}" -B "${build_dir}" -G "${CMAKE_GENERATOR}" ${sdl3_cmake_args}
    RESULT_VARIABLE sdl3_configure_result
    ERROR_VARIABLE sdl3_configure_error
  )
  if(NOT sdl3_configure_result EQUAL 0)
    message(FATAL_ERROR "Failed to configure local SDL3 package: ${sdl3_configure_error}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${build_dir}" --target install
    RESULT_VARIABLE sdl3_build_result
    ERROR_VARIABLE sdl3_build_error
  )
  if(NOT sdl3_build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build/install local SDL3 package: ${sdl3_build_error}")
  endif()

  file(WRITE "${install_prefix}/astraglyph-sdl3.stamp" "${signature}")
endfunction()

function(astraglyph_ensure_json_header dependency_root out_include_dir)
  set(json_include_dir "${dependency_root}/include")
  set(json_path "${json_include_dir}/nlohmann/json.hpp")

  if(NOT EXISTS "${json_path}")
    message(STATUS "Downloading nlohmann/json single header")
    file(MAKE_DIRECTORY "${json_include_dir}/nlohmann")
    file(
      DOWNLOAD
      "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
      "${json_path}"
      STATUS download_status
      TIMEOUT 60
      INACTIVITY_TIMEOUT 15
    )
    list(GET download_status 0 download_code)
    list(GET download_status 1 download_message)
    if(NOT download_code EQUAL 0)
      message(FATAL_ERROR "Failed to download nlohmann/json: ${download_message}")
    endif()
  endif()

  set(${out_include_dir} "${json_include_dir}" PARENT_SCOPE)
endfunction()

function(astraglyph_find_dependencies)
  set(
    ASTRAGLYPH_DEPENDENCY_ROOT
    "${PROJECT_SOURCE_DIR}/.cache/deps"
    CACHE PATH
    "Persistent third-party dependency root reused across clean builds"
  )
  message(STATUS "AstraGlyph dependency root: ${ASTRAGLYPH_DEPENDENCY_ROOT}")

  astraglyph_ensure_json_header("${ASTRAGLYPH_DEPENDENCY_ROOT}" ASTRAGLYPH_JSON_INCLUDE_DIR)
  set(ASTRAGLYPH_JSON_INCLUDE_DIR "${ASTRAGLYPH_JSON_INCLUDE_DIR}" PARENT_SCOPE)

  if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(build_type "Release")
  else()
    set(build_type "${CMAKE_BUILD_TYPE}")
  endif()
  string(TOLOWER "${build_type}" build_type_key)

  set(local_sdl3_prefix "${ASTRAGLYPH_DEPENDENCY_ROOT}/install/${build_type_key}/SDL3-${_ASTRAGLYPH_SDL3_VERSION}")
  astraglyph_sdl3_bootstrap_signature(expected_signature)
  astraglyph_is_local_sdl3_ready("${local_sdl3_prefix}" "${expected_signature}" local_sdl3_ready)

  find_package(SDL3 CONFIG QUIET)

  if(NOT SDL3_FOUND AND local_sdl3_ready)
    message(STATUS "Using persistent local SDL3 package from ${local_sdl3_prefix}")
    find_package(SDL3 CONFIG QUIET PATHS "${local_sdl3_prefix}" NO_DEFAULT_PATH)
  endif()

  if(NOT SDL3_FOUND)
    message(STATUS "SDL3 was not found via find_package, preparing persistent local fallback")
    astraglyph_bootstrap_local_sdl3("${ASTRAGLYPH_DEPENDENCY_ROOT}" "${local_sdl3_prefix}" "${build_type}")
    find_package(SDL3 CONFIG REQUIRED PATHS "${local_sdl3_prefix}" NO_DEFAULT_PATH)
  endif()

  # Resolve actual SDL3 static library filename (MSVC -> SDL3.lib/SDL3-static.lib, MinGW -> libSDL3.a)
  if(EXISTS "${local_sdl3_prefix}/lib/SDL3.lib")
    set(_astraglyph_sdl3_lib "${local_sdl3_prefix}/lib/SDL3.lib")
  elseif(EXISTS "${local_sdl3_prefix}/lib/SDL3-static.lib")
    set(_astraglyph_sdl3_lib "${local_sdl3_prefix}/lib/SDL3-static.lib")
  elseif(EXISTS "${local_sdl3_prefix}/lib/libSDL3.a")
    set(_astraglyph_sdl3_lib "${local_sdl3_prefix}/lib/libSDL3.a")
  else()
    message(FATAL_ERROR "SDL3 static library not found in ${local_sdl3_prefix}/lib")
  endif()

  if(TARGET SDL3::SDL3)
    # On Windows, SDL3 static target may incorrectly link to Unix 'm' library
    # Create a wrapper target to avoid this issue
    if(WIN32)
      if(TARGET SDL3::SDL3-static)
        add_library(astraglyph_sdl3_wrapper INTERFACE IMPORTED)
        # Use known SDL3 include path for Windows
        target_include_directories(astraglyph_sdl3_wrapper INTERFACE 
          "${local_sdl3_prefix}/include"
        )
        # Link to SDL3-static library file directly without inherited link deps
        target_link_libraries(astraglyph_sdl3_wrapper INTERFACE
          ${_astraglyph_sdl3_lib}
          winmm
          setupapi
          imm32
          version
        )
        set(ASTRAGLYPH_SDL3_TARGET astraglyph_sdl3_wrapper PARENT_SCOPE)
      else()
        set(ASTRAGLYPH_SDL3_TARGET SDL3::SDL3 PARENT_SCOPE)
      endif()
    else()
      set(ASTRAGLYPH_SDL3_TARGET SDL3::SDL3 PARENT_SCOPE)
    endif()
    return()
  endif()

  if(TARGET SDL3::SDL3-static)
    # On Windows, SDL3 static target may incorrectly link to Unix 'm' library
    if(WIN32)
      add_library(astraglyph_sdl3_wrapper INTERFACE IMPORTED)
      # Use known SDL3 include path for Windows
      target_include_directories(astraglyph_sdl3_wrapper INTERFACE 
        "${local_sdl3_prefix}/include"
      )
      # Link to SDL3-static library file directly without inherited link deps
      target_link_libraries(astraglyph_sdl3_wrapper INTERFACE
        ${_astraglyph_sdl3_lib}
        winmm
        setupapi
        imm32
        version
      )
      set(ASTRAGLYPH_SDL3_TARGET astraglyph_sdl3_wrapper PARENT_SCOPE)
    else()
      set(ASTRAGLYPH_SDL3_TARGET SDL3::SDL3-static PARENT_SCOPE)
    endif()
    return()
  endif()

  message(FATAL_ERROR "SDL3 target was not created")
endfunction()
