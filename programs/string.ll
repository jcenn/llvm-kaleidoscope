; ModuleID = 'main module'
source_filename = "main module"

@str_literal = private unnamed_addr constant [13 x i8] c"hello world\0A\00", align 1

declare i32 @printf(ptr)

define i32 @main() {
entry:
  %tmp_call = call i32 @printf(ptr @str_literal)
  ret i32 0
}
