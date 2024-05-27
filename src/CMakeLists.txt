configure_file(version.hpp.in version.hpp @ONLY)

set(_cg3ide_src
	inlines.hpp types.hpp ${CMAKE_CURRENT_BINARY_DIR}/version.hpp GotoLine.hpp GrammarEditor.hpp GrammarHighlighter.hpp GrammarState.hpp OptionsDialog.hpp StreamHighlighter.hpp
	GotoLine.ui GrammarEditor.ui OptionsDialog.ui
	main.cpp GotoLine.cpp GrammarEditor.cpp GrammarHighlighter.cpp OptionsDialog.cpp StreamHighlighter.cpp
)
set(_cg3processor_src
	inlines.hpp Processor.hpp
	Processor.ui
	Processor.cpp
    )

if (APPLE)
	set_source_files_properties("../cg3ide.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	set(MACOSX_BUNDLE_BUNDLE_NAME "CG-3 IDE")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.grammarsoft.CG-3 IDE")
	set(MACOSX_BUNDLE_INFO_STRING "IDE for Constraint Grammar v3")
	set(MACOSX_BUNDLE_ICON_FILE "cg3ide.icns")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
	set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
	add_executable(cg3ide MACOSX_BUNDLE ${_cg3ide_src} "../cg3ide.icns")

	add_executable(cg3processor MACOSX_BUNDLE ${_cg3processor_src} "../cg3ide.icns")
	set_target_properties(cg3processor PROPERTIES
		MACOSX_BUNDLE_BUNDLE_NAME "CG-3 IDE Processor"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.grammarsoft.CG-3 IDE Processor"
	    )
else()
	add_executable(cg3ide ${_cg3ide_src})
	add_executable(cg3processor ${_cg3processor_src})
endif()

target_include_directories(cg3ide PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cg3ide ${QT_LIBS})
target_include_directories(cg3processor PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cg3processor ${QT_LIBS})

install(TARGETS cg3ide cg3processor
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
