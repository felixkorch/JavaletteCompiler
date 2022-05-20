declare void @printString(i8*)

declare void @printInt(i32)

declare void @printDouble(double)

declare double @readDouble()

declare i32 @readInt()

declare i32* @multiArray(i32, i32, i32*)

define i32 @main() {
main_entry:
  %0 = alloca [0 x i32], align 4
  store [0 x i32] 0, [0 x i32]* %0, align 4
  %1 = alloca [1 x i32], align 4
  %2 = getelementptr [1 x i32], [1 x i32]* %1, i32 0, i32 0
  store i32 10, i32* %2, align 4
  %3 = call i32* @multiArray(i32 1, i32 4, [1 x i32]* %1)
  store i32* %3, [0 x i32]* %0, align 8
  %4 = getelementptr [0 x i32], [0 x i32]* %0, i32 0, i32 0
  %5 = load i32, i32* %4, align 4
  call void @printInt(i32 %5)
  ret i32 0
}