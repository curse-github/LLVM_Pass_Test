target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i64:64-f80:64-n8:16:32:64-S64"

@Str1 = linkonce_odr constant [38 x i8] c"must input argument for strLen input\0A\00"

define dso_local i32 @main(i32 %argc, ptr %argv) local_unnamed_addr #0 {
    ; if (argc < 2) throw std::runtime_error(Str1)
    %errCond = icmp ult i32 %argc, 2
    br i1 %errCond, label %argError1, label %getStr
argError1:
    tail call void @printStr(ptr @Str1)
    br label %mainRet
getStr:
    ; char* str = argv[1]
    %strPP = getelementptr [2 x ptr], ptr %argv, i64 0, i64 1
    %strP = load ptr, ptr %strPP
    %strLen = tail call i32 @strLen(ptr noundef %strP)
    tail call void @printlnUInt(i32 noundef %strLen)
    br label %mainRet
mainRet:
    ret i32 0
}

define dso_local i32 @strLen(ptr %strP) {
    %lenP = alloca i32
    store i32 0, ptr %lenP
    br label %loop
loop:
    %len.0 = load i32, ptr %lenP
    %chrP = getelementptr [0 x i8], ptr %strP, i64 0, i32 %len.0
    %chr = load i8, ptr %chrP
    %loopCond = icmp eq i8 %chr, 0
    br i1 %loopCond, label %ret, label %loopInner
loopInner:
    %len.1 = add i32 %len.0, 1
    store i32 %len.1, ptr %lenP
    br label %loop
ret:
    ret i32 %len.0
}

declare dso_local void @printChar(i8 noundef %in0) local_unnamed_addr #0
declare dso_local void @printStr(ptr noundef %in0) local_unnamed_addr #0
declare dso_local void @printUInt(i32 noundef %in0) local_unnamed_addr #0
declare dso_local void @printUInt64(i64 noundef %in0) local_unnamed_addr #0
declare dso_local void @printInt(i32 noundef %in0) local_unnamed_addr #0
declare dso_local void @printFloat(float noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnChar(i8 noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnStr(ptr noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnUInt(i32 noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnUInt64(i64 noundef %in) local_unnamed_addr #0
declare dso_local void @printlnInt(i32 noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnFloat(float noundef %in0) local_unnamed_addr #0
declare dso_local void @printlnUInt128(i64 noundef %in) local_unnamed_addr #0
declare dso_local void @printlnUInt256(i64 noundef %in) local_unnamed_addr #0
declare dso_local void @printlnUInt512(i64 noundef %in) local_unnamed_addr #0
declare dso_local void @println() local_unnamed_addr #0
declare dso_local i32 @strToInt(ptr noundef %in0) local_unnamed_addr #0
declare dso_local i32 @strToUInt(ptr noundef %in0) local_unnamed_addr #0

attributes #0 = { norecurse nosync nounwind mustprogress "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nosync nounwind mustprogress "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }