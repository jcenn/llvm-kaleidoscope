; ModuleID = 'main module'
source_filename = "main module"

define i32 @add(i32 %x, i32 %y) {
entry:
  %0 = add i32 %x, %y
  ret i32 %0
}

define i32 @main() {
entry:
  %tmp_call = call i32 @add(i32 1, i32 2)
  ret i32 %tmp_call
}
