add_executable(cg3ide
	inlines.hpp types.hpp version.hpp GotoLine.hpp GrammarEditor.hpp GrammarHighlighter.hpp GrammarState.hpp OptionsDialog.hpp StreamHighlighter.hpp
	GotoLine.ui GrammarEditor.ui OptionsDialog.ui
	main.cpp GotoLine.cpp GrammarEditor.cpp GrammarHighlighter.cpp OptionsDialog.cpp StreamHighlighter.cpp
	)
add_executable(cg3processor
	inlines.hpp Processor.hpp
	Processor.ui
	Processor.cpp
	)

target_link_libraries(cg3ide ${QT_LIBS})
target_link_libraries(cg3processor ${QT_LIBS})

install(TARGETS cg3ide cg3processor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
