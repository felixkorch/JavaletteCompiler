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
  store i32 3, i32* %2, align 4
  %3 = getelementptr [2 x i32], [2 x i32]* %1, i32 0, i32 1
  store i32 0, i32* %3, align 4
  %4 = bitcast [2 x i32]* %1 to i32*
  %5 = call { i32, i32* }* @multiArray(i32 2, i32 4, i32* %4)
  %6 = bitcast { i32, i32* }* %5 to { i32, [0 x { i32, [0 x i32]* }*]* }*
  store { i32, [0 x { i32, [0 x i32]* }*]* }* %6, { i32, [0 x { i32, [0 x i32]* }*]* }** %0, align 8
  %7 = load { i32, [0 x { i32, [0 x i32]* }*]* }*, { i32, [0 x { i32, [0 x i32]* }*]* }** %0, align 8
  %8 = bitcast { i32, [0 x { i32, [0 x i32]* }*]* }* %7 to { i32, i32* }*
  %9 = getelementptr { i32, i32* }, { i32, i32* }* %8, i32 0, i32 0
  %10 = load i32, i32* %9, align 4
  call void @printInt(i32 %10)
  %11 = alloca { i32, [0 x i32]* }*, align 8
  %12 = load { i32, [0 x { i32, [0 x i32]* }*]* }*, { i32, [0 x { i32, [0 x i32]* }*]* }** %0, align 8
  %13 = getelementptr { i32, [0 x { i32, [0 x i32]* }*]* }, { i32, [0 x { i32, [0 x i32]* }*]* }* %12, i32 0, i32 0
  %14 = load i32, i32* %13, align 4
  %15 = alloca i32, align 4
  store i32 0, i32* %15, align 4
  br label %label_0

label_0:                                          ; preds = %label_1, %main_entry
  %16 = load i32, i32* %15, align 4
  %17 = icmp slt i32 %16, %14
  %18 = add i32 %16, 1
  store i32 %18, i32* %15, align 4
  br i1 %17, label %label_1, label %label_2

label_1:                                          ; preds = %label_0
  %19 = getelementptr { i32, [0 x { i32, [0 x i32]* }*]* }, { i32, [0 x { i32, [0 x i32]* }*]* }* %12, i32 0, i32 1
  %20 = load [0 x { i32, [0 x i32]* }*]*, [0 x { i32, [0 x i32]* }*]** %19, align 8
  %21 = getelementptr [0 x { i32, [0 x i32]* }*], [0 x { i32, [0 x i32]* }*]* %20, i32 0, i32 %16
  %22 = load { i32, [0 x i32]* }*, { i32, [0 x i32]* }** %21, align 8
  store { i32, [0 x i32]* }* %22, { i32, [0 x i32]* }** %11, align 8
  %23 = load { i32, [0 x i32]* }*, { i32, [0 x i32]* }** %11, align 8
  %24 = bitcast { i32, [0 x i32]* }* %23 to { i32, i32* }*
  %25 = getelementptr { i32, i32* }, { i32, i32* }* %24, i32 0, i32 0
  %26 = load i32, i32* %25, align 4
  call void @printInt(i32 %26)
  br label %label_0

label_2:                                          ; preds = %label_0
  ret i32 0
}