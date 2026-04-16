; ModuleID = 'main module'
source_filename = "main module"

@str_literal = private unnamed_addr constant [19 x i8] c"number %d is even\0A\00", align 1
@str_literal.1 = private unnamed_addr constant [18 x i8] c"number %d is odd\0A\00", align 1

declare void @printf(ptr, i32)

define i32 @main() {
entry:
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %for_header

for_header:                                       ; preds = %merge, %entry
  %i1 = load i32, ptr %i, align 4
  %0 = icmp slt i32 %i1, 5
  br i1 %0, label %loop, label %after

loop:                                             ; preds = %for_header
  %i2 = load i32, ptr %i, align 4
  %1 = srem i32 %i2, 2
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %then, label %else

then:                                             ; preds = %loop
  %i3 = load i32, ptr %i, align 4
  call void @printf(ptr @str_literal, i32 %i3)
  br label %merge

else:                                             ; preds = %loop
  %i4 = load i32, ptr %i, align 4
  call void @printf(ptr @str_literal.1, i32 %i4)
  br label %merge

merge:                                            ; preds = %else, %then
  %i5 = load i32, ptr %i, align 4
  %3 = add i32 %i5, 1
  store i32 %3, ptr %i, align 4
  br label %for_header

after:                                            ; preds = %for_header
  ret i32 0
}
