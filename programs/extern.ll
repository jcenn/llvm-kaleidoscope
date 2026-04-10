; ModuleID = 'main module'
source_filename = "main module"

declare i32 @time(i32)

declare i32 @rand()

declare void @srand(i32)

declare i32 @pow(i32, i32)

define void @init_rand() {
entry:
  %tmp_call = call i32 @time(i32 0)
  call void @srand(i32 %tmp_call)
  ret void
}

define i32 @main() {
entry:
  call void @init_rand()
  %tmp_call = call i32 @pow(i32 2, i32 2)
  ret i32 %tmp_call
}
