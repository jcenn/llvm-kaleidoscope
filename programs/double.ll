; ModuleID = 'main module'
source_filename = "main module"

define i32 @main() {
entry:
  %a = alloca double, align 8
  store double 3.000000e+00, ptr %a, align 8
  %a1 = load double, ptr %a, align 8
  %0 = fadd double %a1, 1.000000e+00
  store double %0, ptr %a, align 8
  %a2 = load double, ptr %a, align 8
  %1 = fcmp oeq double %a2, 3.000000e+00
  br i1 %1, label %then, label %merge

then:                                             ; preds = %entry
  ret i32 1

merge:                                            ; preds = %entry
  ret i32 0
}
