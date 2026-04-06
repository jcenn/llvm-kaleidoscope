; ModuleID = 'main module'
source_filename = "main module"

declare i32 @rand()

declare i32 @time(i32)

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

define i32 @rn() {
entry:
  %call_tmp = call i32 @rand()
  ret i32 %call_tmp
}

define i32 @main() {
entry:
  %call_tmp = call i32 @time(i32 0)
  ret i32 %call_tmp
}
