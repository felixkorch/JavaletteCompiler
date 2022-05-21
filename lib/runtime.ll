@dnl = internal constant [4 x i8] c"%d\0A\00"
@fnl = internal constant [6 x i8] c"%.1f\0A\00"
@d   = internal constant [3 x i8] c"%d\00"
@lf  = internal constant [4 x i8] c"%lf\00"

declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i32 @puts(i8*)

define void @printInt(i32 %x) {
entry: %t0 = getelementptr [4 x i8], [4 x i8]* @dnl, i32 0, i32 0
	call i32 (i8*, ...) @printf(i8* %t0, i32 %x)
	ret void
}

define void @printDouble(double %x) {
entry: %t0 = getelementptr [6 x i8], [6 x i8]* @fnl, i32 0, i32 0
	call i32 (i8*, ...) @printf(i8* %t0, double %x)
	ret void
}

define void @printString(i8* %s) {
entry:  call i32 @puts(i8* %s)
	ret void
}

define i32 @readInt() {
entry:	%res = alloca i32
        %t1 = getelementptr [3 x i8], [3 x i8]* @d, i32 0, i32 0
	call i32 (i8*, ...) @scanf(i8* %t1, i32* %res)
	%t2 = load i32, i32* %res
	ret i32 %t2
}

define double @readDouble() {
entry:	%res = alloca double
        %t1 = getelementptr [4 x i8], [4 x i8]* @lf, i32 0, i32 0
	call i32 (i8*, ...) @scanf(i8* %t1, double* %res)
	%t2 = load double, double* %res
	ret double %t2
}

%struct.MultiArray_T = type { i32, i8* }

; Function Attrs: mustprogress nofree nounwind sspstrong uwtable willreturn
define dso_local noalias %struct.MultiArray_T* @newMultiArray(i32 %0, i8* %1) local_unnamed_addr #0 {
  %3 = tail call noalias align 16 dereferenceable_or_null(16) i8* @calloc(i64 1, i64 16) #4
  %4 = bitcast i8* %3 to %struct.MultiArray_T*
  %5 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %4, i64 0, i32 0
  store i32 %0, i32* %5, align 16, !tbaa !5
  %6 = getelementptr inbounds %struct.MultiArray_T, %struct.MultiArray_T* %4, i64 0, i32 1
  store i8* %1, i8** %6, align 8, !tbaa !11
  ret %struct.MultiArray_T* %4
}

; Function Attrs: mustprogress nofree nounwind willreturn
declare noalias noundef align 16 i8* @calloc(i64 noundef, i64 noundef) local_unnamed_addr #1

; Function Attrs: nofree nounwind sspstrong uwtable
define dso_local noalias %struct.MultiArray_T* @multiArray(i32 %0, i32 %1, i32* nocapture %2) local_unnamed_addr #2 {
  %4 = load i32, i32* %2, align 4, !tbaa !12
  %5 = icmp eq i32 %0, 1
  %6 = sext i32 %4 to i64
  br i1 %5, label %7, label %10

7:                                                ; preds = %3
  %8 = sext i32 %1 to i64
  %9 = tail call noalias align 16 i8* @calloc(i64 %6, i64 %8) #4
  br label %26

10:                                               ; preds = %3
  %11 = shl nsw i64 %6, 3
  %12 = tail call noalias align 16 i8* @malloc(i64 %11) #4
  %13 = bitcast i8* %12 to i8**
  %14 = add nsw i32 %0, -1
  %15 = getelementptr inbounds i32, i32* %2, i64 1
  %16 = icmp sgt i32 %4, 0
  br i1 %16, label %17, label %26

17:                                               ; preds = %10
  %18 = zext i32 %4 to i64
  br label %19

19:                                               ; preds = %17, %19
  %20 = phi i64 [ 0, %17 ], [ %24, %19 ]
  %21 = tail call %struct.MultiArray_T* @multiArray(i32 %14, i32 %1, i32* nonnull %15)
  %22 = getelementptr inbounds i8*, i8** %13, i64 %20
  %23 = bitcast i8** %22 to %struct.MultiArray_T**
  store %struct.MultiArray_T* %21, %struct.MultiArray_T** %23, align 8, !tbaa !13
  %24 = add nuw nsw i64 %20, 1
  %25 = icmp eq i64 %24, %18
  br i1 %25, label %26, label %19, !llvm.loop !14

26:                                               ; preds = %19, %10, %7
  %27 = phi i8* [ %9, %7 ], [ %12, %10 ], [ %12, %19 ]
  %28 = tail call noalias align 16 dereferenceable_or_null(16) i8* @calloc(i64 1, i64 16) #4
  %29 = bitcast i8* %28 to %struct.MultiArray_T*
  %30 = bitcast i8* %28 to i32*
  store i32 %4, i32* %30, align 16, !tbaa !5
  %31 = getelementptr inbounds i8, i8* %28, i64 8
  %32 = bitcast i8* %31 to i8**
  store i8* %27, i8** %32, align 8, !tbaa !11
  ret %struct.MultiArray_T* %29
}

; Function Attrs: inaccessiblememonly mustprogress nofree nounwind willreturn
declare noalias noundef align 16 i8* @malloc(i64 noundef) local_unnamed_addr #3

attributes #0 = { mustprogress nofree nounwind sspstrong uwtable willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nofree nounwind willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nofree nounwind sspstrong uwtable "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { inaccessiblememonly mustprogress nofree nounwind willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{!"clang version 13.0.1"}
!5 = !{!6, !7, i64 0}
!6 = !{!"MultiArray_T", !7, i64 0, !10, i64 8}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!"any pointer", !8, i64 0}
!11 = !{!6, !10, i64 8}
!12 = !{!7, !7, i64 0}
!13 = !{!10, !10, i64 0}
!14 = distinct !{!14, !15}
!15 = !{!"llvm.loop.mustprogress"}
