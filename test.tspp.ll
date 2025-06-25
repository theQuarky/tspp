; ModuleID = 'tspp_module'
source_filename = "tspp_module"

@globalCounter = global i32 0

declare i32 @printf(i8, ...)

declare i32 @puts(i8)

declare i8 @malloc(i64)

declare void @free(i8)

define i32 @main() {
entry:
  ret i32 0
}
