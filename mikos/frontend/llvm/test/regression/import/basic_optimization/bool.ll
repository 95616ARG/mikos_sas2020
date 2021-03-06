; ModuleID = 'bool.pp.bc'
source_filename = "bool.cpp"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.14.0"

; CHECK-LABEL: Bundle
; CHECK: target-endianness = little-endian
; CHECK: target-pointer-size = 64 bits
; CHECK: target-triple = x86_64-apple-macosx10.14.0

@b = global i8 1, align 1, !dbg !0
; CHECK: define ui8* @b, align 1, init {
; CHECK: #1 !entry !exit {
; CHECK:   store @b, 1, align 1
; CHECK: }
; CHECK: }

; Function Attrs: noinline nounwind ssp uwtable
define i32 @_Z1fb(i1 zeroext) #0 !dbg !12 {
  call void @llvm.dbg.value(metadata i1 %0, metadata !16, metadata !DIExpression()), !dbg !17
  call void @llvm.trap(), !dbg !18
  unreachable, !dbg !18
}
; CHECK: define si32 @_Z1fb(ui1 %1) {
; CHECK: #1 !entry !exit {
; CHECK:   call @ar.trap()
; CHECK:   unreachable
; CHECK: }
; CHECK: }

; Function Attrs: cold noreturn nounwind
declare void @llvm.trap() #1
; CHECK: declare void @ar.trap()

; Function Attrs: noinline norecurse nounwind ssp uwtable
define i32 @main() #2 !dbg !19 {
  ret i32 0, !dbg !22
}
; CHECK: define si32 @main() {
; CHECK: #1 !entry !exit {
; CHECK:   return 0
; CHECK: }
; CHECK: }

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

attributes #0 = { noinline nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { cold noreturn nounwind }
attributes #2 = { noinline norecurse nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!7, !8, !9, !10}
!llvm.ident = !{!11}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !3, line: 1, type: !6, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "clang version 9.0.0 (tags/RELEASE_900/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5, nameTableKind: GNU)
!3 = !DIFile(filename: "bool.cpp", directory: "/Users/marthaud/ikos/ikos-git/frontend/llvm/test/regression/import/basic_optimization")
!4 = !{}
!5 = !{!0}
!6 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!7 = !{i32 2, !"Dwarf Version", i32 4}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = !{i32 1, !"wchar_size", i32 4}
!10 = !{i32 7, !"PIC Level", i32 2}
!11 = !{!"clang version 9.0.0 (tags/RELEASE_900/final)"}
!12 = distinct !DISubprogram(name: "f", linkageName: "_Z1fb", scope: !3, file: !3, line: 3, type: !13, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !4)
!13 = !DISubroutineType(types: !14)
!14 = !{!15, !6}
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !DILocalVariable(name: "i", arg: 1, scope: !12, file: !3, line: 3, type: !6)
!17 = !DILocation(line: 0, scope: !12)
!18 = !DILocation(line: 3, column: 15, scope: !12)
!19 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 5, type: !20, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !4)
!20 = !DISubroutineType(types: !21)
!21 = !{!15}
!22 = !DILocation(line: 6, column: 3, scope: !19)
