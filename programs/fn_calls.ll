; ModuleID = 'main module'
source_filename = "main module"

define i32 @foo() {
entry:
  ret i32 9
}

define i32 @main() {
entry:
  %call_tmp = call i32 @foo()
  ret i32 %call_tmp
}
