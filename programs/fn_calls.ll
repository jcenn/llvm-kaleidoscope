; ModuleID = 'main module'
source_filename = "main module"

define i32 @foo() {
entry:
  ret i32 9
}

define i32 @main() {
entry:
  %call_tmp = call i32 @foo()
  %call_tmp1 = call i32 @foo()
  %0 = sub i32 %call_tmp1, 2
  %1 = add i32 %call_tmp, %0
  ret i32 %1
}
