function(astraglyph_setup_project_options)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" ON)
endfunction()

function(astraglyph_apply_project_options target)
  target_compile_features(${target} PRIVATE cxx_std_20)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()
