# Invoked from POST_BUILD on ${PROJECT_NAME}_Standalone (see root CMakeLists.txt).
# Copies the signed .app into binaries/macos-arm64 for version control when building
# Release on an arm64-only macOS configuration.

if(NOT NAM_BUILD_CONFIG STREQUAL "Release")
    return()
endif()

set(dest_dir "${NAM_REPO_ROOT}/binaries/macos-arm64")
set(dest_app "${dest_dir}/Neural Amp Modeler.app")

execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${dest_dir}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E rm -rf "${dest_app}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${NAM_BUNDLE}" "${dest_app}"
    RESULT_VARIABLE nam_copy_rc)

if(NOT nam_copy_rc EQUAL 0)
    message(WARNING "CopyStandaloneToBinaries: copy_directory failed (${nam_copy_rc})")
    return()
endif()

# Single-file ZIP for GitHub “raw” download links (.app is a directory and cannot be downloaded directly).
set(dest_zip "${dest_dir}/NeuralAmpModeler-macOS-arm64.zip")
execute_process(COMMAND "${CMAKE_COMMAND}" -E rm -f "${dest_zip}")
execute_process(COMMAND ditto -c -k --keepParent "Neural Amp Modeler.app" "NeuralAmpModeler-macOS-arm64.zip"
    WORKING_DIRECTORY "${dest_dir}"
    RESULT_VARIABLE nam_zip_rc)

if(NOT nam_zip_rc EQUAL 0)
    message(WARNING "CopyStandaloneToBinaries: ditto zip failed (${nam_zip_rc})")
endif()
