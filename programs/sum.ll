; ModuleID = 'main module'
source_filename = "main module"

define i32 @sum(i32 %x) {
entry:
  %ifcond = icmp eq i32 %x, 0
  br i1 %ifcond, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 0

merge:                                            ; preds = %entry
  %0 = sub i32 %x, 1
  %tmp_call = call i32 @sum(i32 %0)
  %1 = add i32 %x, %tmp_call
  ret i32 %1
}

define i32 @main() {
entry:
  %tmp_call = call i32 @sum(i32 5)
  ret i32 %tmp_call
}
