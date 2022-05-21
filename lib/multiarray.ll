; ModuleID = 'multiarray.c'
source_filename = "multiarray.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.MultiArray_T = type { i32, i8* }

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local %struct.MultiArray_T* @newMultiArray(i32 %0, i8* %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i8*, align 8
  %5 = alloca %struct.MultiArray_T*, align 8
  store i32 %0, i32* %3, align 4
  store i8* %1, i8** %4, align 8
  %6 = call noalias align 16 i8* @calloc(i64 1, i64 16) #2
  %7 = bitcast i8* %6 to %struct.MultiArray_T*
  store %struct.MultiArray_T* %7, %struct.MultiArray_T** %5, align 8
  %8 = load i32, i32* %3, align 4
  %9 = load %struct.MultiArray_T*, %struct.MultiArray_T** %5, align 8
  %10 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %9, i32 0, i32 0
  store i32 %8, i32* %10, align 8
  %11 = load i8*, i8** %4, align 8
  %12 = load %struct.MultiArray_T*, %struct.MultiArray_T** %5, align 8
  %13 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %12, i32 0, i32 1
  store i8* %11, i8** %13, align 8
  %14 = load %struct.MultiArray_T*, %struct.MultiArray_T** %5, align 8
  ret %struct.MultiArray_T* %14
}

; Function Attrs: nounwind
declare noalias align 16 i8* @calloc(i64, i64) #1

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local %struct.MultiArray_T* @multiArray(i32 %0, i32 %1, i32* %2) #0 {
  %4 = alloca %struct.MultiArray_T*, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32*, align 8
  %8 = alloca i32, align 4
  %9 = alloca i8*, align 8
  %10 = alloca i8**, align 8
  %11 = alloca i32, align 4
  %12 = alloca %struct.MultiArray_T, align 8
  store i32 %0, i32* %5, align 4
  store i32 %1, i32* %6, align 4
  store i32* %2, i32** %7, align 8
  %13 = load i32*, i32** %7, align 8
  %14 = getelementptr inbounds i32, i32* %13, i64 0
  %15 = load i32, i32* %14, align 4
  store i32 %15, i32* %8, align 4
  %16 = load i32, i32* %5, align 4
  %17 = icmp eq i32 %16, 1
  br i1 %17, label %18, label %27

18:                                               ; preds = %3
  %19 = load i32, i32* %8, align 4
  %20 = sext i32 %19 to i64
  %21 = load i32, i32* %6, align 4
  %22 = sext i32 %21 to i64
  %23 = call noalias align 16 i8* @calloc(i64 %20, i64 %22) #2
  store i8* %23, i8** %9, align 8
  %24 = load i32, i32* %8, align 4
  %25 = load i8*, i8** %9, align 8
  %26 = call %struct.MultiArray_T* @newMultiArray(i32 %24, i8* %25)
  store %struct.MultiArray_T* %26, %struct.MultiArray_T** %4, align 8
  br label %62

27:                                               ; preds = %3
  %28 = load i32, i32* %8, align 4
  %29 = sext i32 %28 to i64
  %30 = mul i64 %29, 8
  %31 = call noalias align 16 i8* @malloc(i64 %30) #2
  %32 = bitcast i8* %31 to i8**
  store i8** %32, i8*** %10, align 8
  store i32 0, i32* %11, align 4
  br label %33

33:                                               ; preds = %49, %27
  %34 = load i32, i32* %11, align 4
  %35 = load i32, i32* %8, align 4
  %36 = icmp slt i32 %34, %35
  br i1 %36, label %37, label %52

37:                                               ; preds = %33
  %38 = load i32, i32* %5, align 4
  %39 = sub nsw i32 %38, 1
  %40 = load i32, i32* %6, align 4
  %41 = load i32*, i32** %7, align 8
  %42 = getelementptr inbounds i32, i32* %41, i64 1
  %43 = call %struct.MultiArray_T* @multiArray(i32 %39, i32 %40, i32* %42)
  %44 = bitcast %struct.MultiArray_T* %43 to i8*
  %45 = load i8**, i8*** %10, align 8
  %46 = load i32, i32* %11, align 4
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds i8*, i8** %45, i64 %47
  store i8* %44, i8** %48, align 8
  br label %49

49:                                               ; preds = %37
  %50 = load i32, i32* %11, align 4
  %51 = add nsw i32 %50, 1
  store i32 %51, i32* %11, align 4
  br label %33, !llvm.loop !6

52:                                               ; preds = %33
  %53 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %12, i32 0, i32 0
  %54 = load i32, i32* %8, align 4
  store i32 %54, i32* %53, align 8
  %55 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %12, i32 0, i32 1
  %56 = load i8**, i8*** %10, align 8
  %57 = bitcast i8** %56 to i8*
  store i8* %57, i8** %55, align 8
  %58 = load i32, i32* %8, align 4
  %59 = load i8**, i8*** %10, align 8
  %60 = bitcast i8** %59 to i8*
  %61 = call %struct.MultiArray_T* @newMultiArray(i32 %58, i8* %60)
  store %struct.MultiArray_T* %61, %struct.MultiArray_T** %4, align 8
  br label %62

62:                                               ; preds = %52, %18
  %63 = load %struct.MultiArray_T*, %struct.MultiArray_T** %4, align 8
  ret %struct.MultiArray_T* %63
}

; Function Attrs: nounwind
declare noalias align 16 i8* @malloc(i64) #1

attributes #0 = { noinline nounwind optnone sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 13.0.1"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
