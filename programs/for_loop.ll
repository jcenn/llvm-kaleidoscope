; ModuleID = 'main module'
source_filename = "main module"

declare i32 @putchar(i32)

define i32 @main() {
entry:
  br label %for_header

for_header:                                       ; preds = %loop, %entry
  br i1 false, label %loop, label %after

loop:                                             ; preds = %for_header
  %tmp_call = call i32 @putchar(i32 10)
  br label %for_header

after:                                            ; preds = %for_header
  ret i32 0
}
