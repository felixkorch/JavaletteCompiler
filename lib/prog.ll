declare void @printString(i8*)

declare void @printInt(i32)

declare void @printDouble(double)

declare double @readDouble()

declare i32 @readInt()

declare { i32, i32* }* @multiArray(i32, i32, i32*)

define i32 @main() {
main_entry:
  %0 = alloca { i32, [0 x { i32, [0 x i32]* }*]* }*, align 8
  %1 = alloca [2 x i32], align 4
  %2 = getelementptr [2 x i32], [2 x i32]* %1, i32 0, i32 0
  store i32 2, i32* %2, align 4
  %3 = getelementptr [2 x i32], [2 x i32]* %1, i32 0, i32 1
  store i32 2, i32* %3, align 4
  %4 = bitcast [2 x i32]* %1 to i32*
  %5 = call { i32, i32* }* @multiArray(i32 2, i32 4, i32* %4)
  %6 = bitcast { i32, i32* }* %5 to { i32, [0 x { i32, [0 x i32]* }*]* }*
  store { i32, [0 x { i32, [0 x i32]* }*]* }* %6, { i32, [0 x { i32, [0 x i32]* }*]* }** %0, align 8
  %7 = alloca i32, align 4
  store i32 0, i32* %7, align 4
  %8 = load { i32, [0 x { i32, [0 x i32]* }*]* }*, { i32, [0 x { i32, [0 x i32]* }*]* }** %0, align 8
  %9 = bitcast { i32, [0 x { i32, [0 x i32]* }*]* }* %8 to { i32, i32* }*
  %10 = ptrtoint { i32, i32* }* %9 to i64
  %11 = sub i64 %10, 0
  %12 = sdiv exact i64 %11, ptrtoint ({ i32, i32* }* getelementptr ({ i32, i32* }, { i32, i32* }* null, i32 1) to i64)
  %13 = icmp ne i64 %12, 0
  br i1 %13, label %label_0, label %label_1

label_0:                                          ; preds = %main_entry
  %14 = getelementptr { i32, i32* }, { i32, i32* }* %9, i32 0, i32 0
  %15 = load i32, i32* %14, align 4
  store i32 %15, i32* %7, align 4
  br label %label_1

label_1:                                          ; preds = %label_0, %main_entry
  %16 = load i32, i32* %7, align 4
  call void @printInt(i32 %16)
  ret i32 0
}