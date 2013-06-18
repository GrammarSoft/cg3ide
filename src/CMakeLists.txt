set(cg3ide_MOC_HPPS GotoLine.hpp GrammarEditor.hpp GrammarHighlighter.hpp OptionsDialog.hpp StreamHighlighter.hpp)
set(cg3processor_MOC_HPPS Processor.hpp)
set(cg3ide_UI_XMLS GotoLine.ui GrammarEditor.ui OptionsDialog.ui)
set(cg3processor_UI_XMLS Processor.ui)

qt5_wrap_cpp(cg3ide_MOCS ${cg3ide_MOC_HPPS})
qt5_wrap_cpp(cg3processor_MOCS ${cg3processor_MOC_HPPS})
qt5_wrap_ui(cg3ide_UIS ${cg3ide_UI_XMLS})
qt5_wrap_ui(cg3processor_UIS ${cg3processor_UI_XMLS})

add_executable(cg3ide main.cpp GotoLine.cpp GrammarEditor.cpp GrammarHighlighter.cpp OptionsDialog.cpp StreamHighlighter.cpp ${cg3ide_MOCS} ${cg3ide_UIS})
add_executable(cg3processor Processor.cpp ${cg3processor_MOCS} ${cg3processor_UIS})

target_link_libraries(cg3ide ${Qt5Widgets_LIBRARIES})
target_link_libraries(cg3processor ${Qt5Widgets_LIBRARIES})

install(TARGETS cg3ide cg3processor RUNTIME DESTINATION bin)

