; ModuleID = 'main module'
source_filename = "main module"

declare i32 @putchar(i32)

define i32 @main() {
entry:
  %tmp_call = call i32 @putchar(i32 67)
  %tmp_call1 = call i32 @putchar(i32 10)
  ret i32 0
}
