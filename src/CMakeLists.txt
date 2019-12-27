find_package(BISON)
find_package(FLEX)

BISON_TARGET(MyParser my_lisp.y ${CMAKE_CURRENT_BINARY_DIR}/my_lisp.tab.c)
FLEX_TARGET(MyScanner my_lisp.l  ${CMAKE_CURRENT_BINARY_DIR}/my_lisp.lex.c)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("/home/yydcnjjw/workspace/code/project/my-os/my-os/include")

add_executable(my-lisp
  my_lisp_main.c
  my_lisp.c
  ${FLEX_MyScanner_OUTPUTS}  
  ${BISON_MyParser_OUTPUTS}
  )
