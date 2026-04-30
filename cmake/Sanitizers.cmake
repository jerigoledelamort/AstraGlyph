option(ASTRAGLYPH_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

function(astraglyph_enable_sanitizers target)
  if(NOT ASTRAGLYPH_ENABLE_SANITIZERS)
    return()
  endif()

  if(MSVC)
    message(WARNING "ASTRAGLYPH_ENABLE_SANITIZERS is not configured for MSVC yet")
    return()
  endif()

  target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
