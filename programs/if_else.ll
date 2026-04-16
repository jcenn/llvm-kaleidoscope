; ModuleID = 'main module'
source_filename = "main module"

define i32 @early_return(i32 %x) {
entry:
  %0 = icmp eq i32 %x, 0
  br i1 %0, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 0

merge:                                            ; preds = %entry
  %1 = icmp eq i32 %x, 1
  br i1 %1, label %then1, label %else

then1:                                            ; preds = %merge
  ret i32 1

else:                                             ; preds = %merge
  ret i32 3

merge2:                                           ; No predecessors!
  ret i32 0
}

define i32 @main() {
entry:
  %res = alloca i32, align 4
  store i32 123, ptr %res, align 4
  br i1 true, label %then, label %merge

then:                                             ; preds = %entry
  %tmp_call = call i32 @early_return(i32 5)
  store i32 %tmp_call, ptr %res, align 4
  br label %merge

merge:                                            ; preds = %then, %entry
  %res1 = load i32, ptr %res, align 4
  ret i32 %res1
}
