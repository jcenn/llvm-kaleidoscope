; ModuleID = 'main module'
source_filename = "main module"

define i32 @add(i32 %x, i32 %y) {
entry:
  %0 = add i32 %x, %y
  ret i32 %0
}

define i32 @sub(i32 %x, i32 %y) {
entry:
  %0 = sub i32 %x, %y
  ret i32 %0
}

define i32 @main() {
entry:
  %call_tmp = call i32 @sub(i32 1, i32 1)
  %call_tmp1 = call i32 @add(i32 1, i32 3)
  %call_tmp2 = call i32 @sub(i32 5, i32 1)
  %0 = add i32 %call_tmp1, %call_tmp2
  ret i32 %0
}
