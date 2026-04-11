; ModuleID = 'main module'
source_filename = "main module"

define i32 @fib(i32 %x) {
entry:
  %0 = sub i32 %x, 1
  %ifcond = icmp eq i32 %0, 0
  br i1 %ifcond, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 1

merge:                                            ; preds = %entry
  %1 = sub i32 %x, 2
  %ifcond1 = icmp eq i32 %1, 0
  br i1 %ifcond1, label %then2, label %merge3

then2:                                            ; preds = %merge
  ret i32 1

merge3:                                           ; preds = %merge
  %2 = sub i32 %x, 1
  %tmp_call = call i32 @fib(i32 %2)
  %3 = sub i32 %x, 2
  %tmp_call4 = call i32 @fib(i32 %3)
  %4 = add i32 %tmp_call, %tmp_call4
  ret i32 %4
}

define i32 @main() {
entry:
  %tmp_call = call i32 @fib(i32 12)
  ret i32 %tmp_call
}
