declare void @printString(i8*)

declare void @printInt(i32)

declare void @printDouble(double)

declare double @readDouble()

declare i32 @readInt()

declare i32* @multiArray(i32, i32, i32*)

define i32 @main() {
main_entry:
  %0 = alloca [0 x [0 x i32]*]*, align 8
  store [0 x [0 x i32]*]* null, [0 x [0 x i32]*]** %0, align 8
  %1 = alloca [2 x i32], align 4
  %2 = getelementptr [2 x i32], [2 x i32]* %1, i32 0, i32 0
  store i32 10, i32* %2, align 4
  %3 = getelementptr [2 x i32], [2 x i32]* %1, i32 0, i32 1
  store i32 10, i32* %3, align 4
  %4 = bitcast [2 x i32]* %1 to i32*
  %5 = call i32* @multiArray(i32 2, i32 4, i32* %4)
  %6 = bitcast i32* %5 to [0 x [0 x i32]*]*
  store [0 x [0 x i32]*]* %6, [0 x [0 x i32]*]** %0, align 8
  %7 = load [0 x [0 x i32]*]*, [0 x [0 x i32]*]** %0, align 8
  %8 = getelementptr [0 x [0 x i32]*], [0 x [0 x i32]*]* %7, i32 0, i32 0
  %9 = load [0 x i32]*, [0 x i32]** %8, align 8
  %10 = getelementptr [0 x i32], [0 x i32]* %9, i32 0, i32 0
  store i32 33, i32* %10, align 4
  %11 = load [0 x [0 x i32]*]*, [0 x [0 x i32]*]** %0, align 8
  %12 = getelementptr [0 x [0 x i32]*], [0 x [0 x i32]*]* %11, i32 0, i32 0
  %13 = load [0 x i32]*, [0 x i32]** %12, align 8
  %14 = getelementptr [0 x i32], [0 x i32]* %13, i32 0, i32 0
  %15 = load i32, i32* %14, align 4
  call void @printInt(i32 %15)
  ret i32 0
}