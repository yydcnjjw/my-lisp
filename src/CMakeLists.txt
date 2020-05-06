find_package(BISON)
find_package(FLEX)


set(CMAKE_C_FLAGS "-fno-builtin -fno-stack-protector -mcmodel=large -m64 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-fno-builtin -fno-stack-protector -mcmodel=large -m64 -Wall -Wextra")
# set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,-gc-sections")
# set(CMAKE_EXE_LINKER_FLAGS "-nostdlib")

BISON_TARGET(MyParser my_lisp.y ${CMAKE_CURRENT_BINARY_DIR}/my_lisp.tab.c)
FLEX_TARGET(MyScanner my_lisp.l ${CMAKE_CURRENT_BINARY_DIR}/my_lisp.lex.c)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("/home/yydcnjjw/workspace/code/project/my-os/my-os/include")
include_directories("/home/yydcnjjw/workspace/code/project/my-os/my-os/arch/x86/include")

add_executable(my-lisp
  my_lisp_main.c
  my_lisp.c
  os.c
  number.c
  strtod.c
  strtox.c
  ${FLEX_MyScanner_OUTPUTS}  
  ${BISON_MyParser_OUTPUTS}
  )
target_link_libraries(my-lisp
  -lm
  )
