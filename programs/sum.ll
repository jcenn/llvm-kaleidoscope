; ModuleID = 'main module'
source_filename = "main module"

define i32 @sum(i32 %x) {
entry:
  %0 = icmp eq i32 %x, 1
  br i1 %0, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 %x

merge:                                            ; preds = %entry
  %1 = sub i32 %x, 1
  %tmp_call = call i32 @sum(i32 %1)
  %2 = add i32 %x, %tmp_call
  ret i32 %2
}

define i32 @main() {
entry:
  %tmp_call = call i32 @sum(i32 5)
  ret i32 %tmp_call
}
