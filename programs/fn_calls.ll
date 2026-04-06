; ModuleID = 'main module'
source_filename = "main module"

define i32 @add(i32 %x, i32 %y) {
entry:
  %0 = add i32 %y, %x
  %1 = add i32 %x, %0
  ret i32 %1
}

define i32 @main() {
entry:
  %call_tmp = call i32 @add(i32 1, i32 2)
  ret i32 %call_tmp
}
