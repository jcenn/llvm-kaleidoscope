; ModuleID = 'main module'
source_filename = "main module"

define i32 @fib(i32 %n) {
entry:
  %0 = icmp eq i32 %n, 1
  br i1 %0, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 1

merge:                                            ; preds = %entry
  %1 = icmp eq i32 %n, 2
  br i1 %1, label %then1, label %merge2

then1:                                            ; preds = %merge
  ret i32 1

merge2:                                           ; preds = %merge
  %2 = sub i32 %n, 1
  %tmp_call = call i32 @fib(i32 %2)
  %3 = sub i32 %n, 2
  %tmp_call3 = call i32 @fib(i32 %3)
  %4 = add i32 %tmp_call, %tmp_call3
  ret i32 %4
}

define i32 @main() {
entry:
  %tmp_call = call i32 @fib(i32 12)
  ret i32 %tmp_call
}
