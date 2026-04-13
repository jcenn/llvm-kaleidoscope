; ModuleID = 'main module'
source_filename = "main module"

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 0, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %0 = add i32 %x1, 1
  store i32 %0, ptr %x, align 4
  %x2 = load i32, ptr %x, align 4
  %1 = add i32 %x2, 2
  store i32 %1, ptr %x, align 4
  %x3 = load i32, ptr %x, align 4
  ret i32 %x3
}
