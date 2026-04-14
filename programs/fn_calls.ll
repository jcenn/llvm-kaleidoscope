; ModuleID = 'main module'
source_filename = "main module"

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

define void @test() {
entry:
  %a = alloca i32, align 4
  %tmp_call = call i32 @add(i32 1, i32 1)
  store i32 %tmp_call, ptr %a, align 4
  ret void
}

define i32 @main() {
entry:
  call void @test()
  %tmp_call = call i32 @sub(i32 1, i32 1)
  %tmp_call1 = call i32 @add(i32 1, i32 3)
  %tmp_call2 = call i32 @sub(i32 5, i32 1)
  %0 = add i32 %tmp_call1, %tmp_call2
  ret i32 %0
}
