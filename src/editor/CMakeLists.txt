# Developer-only entity editor. It is deliberately excluded from installation.
add_library(StonePotatoesEditorCore STATIC EditorSession.cpp)
target_include_directories(StonePotatoesEditorCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(StonePotatoesEditorCore PUBLIC StonePotatoesECS)

add_executable(StonePotatoesEditor main.cpp)
target_link_libraries(StonePotatoesEditor
    PRIVATE StonePotatoesEditorCore StonePotatoesRendering)
set_target_properties(StonePotatoesEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
