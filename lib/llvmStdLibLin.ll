target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; importing these from another file and wrapping them
declare dso_local void @_Z5printc(i8 noundef) local_unnamed_addr #0; char
declare dso_local void @_Z5printPKc(ptr noundef) local_unnamed_addr #0; str
declare dso_local void @_Z5printj(i32 noundef) local_unnamed_addr #0; uint32
declare dso_local void @_Z5printy(i64 noundef) local_unnamed_addr #0; uint64
declare dso_local void @_Z5printi(i32 noundef) local_unnamed_addr #0; int32
declare dso_local void @_Z5printx(i64 noundef) local_unnamed_addr #0; int64
declare dso_local void @_Z5printf(float noundef) local_unnamed_addr #0; float
declare dso_local void @_Z5printd(double noundef) local_unnamed_addr #0; double
;declare dso_local void @_Z5print(t noundef) local_unnamed_addr #0; fp128
declare dso_local void @_Z7printlnc(i8 noundef) local_unnamed_addr #0; char nl
declare dso_local void @_Z7printlnPKc(ptr noundef) local_unnamed_addr #0; str nl
declare dso_local void @_Z7printlnj(i32 noundef) local_unnamed_addr #0; uint32 nl
declare dso_local void @_Z7printlny(i64 noundef) local_unnamed_addr #0; uint64 nl
declare dso_local void @_Z7printlni(i32 noundef) local_unnamed_addr #0; int32 nl
declare dso_local void @_Z7printlnx(i64 noundef) local_unnamed_addr #0; int64 nl
declare dso_local void @_Z7printlnf(float noundef) local_unnamed_addr #0; float nl
declare dso_local void @_Z7printlnd(double noundef) local_unnamed_addr #0; double nl
;declare dso_local void @_Z7println(t noundef) local_unnamed_addr #0; fp128 nl
declare dso_local void @_Z7printlnv() local_unnamed_addr #0; nl

declare dso_local i32 @_Z8strToIntPKc(ptr noundef) local_unnamed_addr #0
declare dso_local i32 @_Z9strToUIntPKc(ptr noundef) local_unnamed_addr #0

define dso_local void @printChar(i8 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printc(i8 noundef %in0)
    ret void
}
define dso_local void @printStr(ptr noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printPKc(ptr noundef %in0)
    ret void
}
define dso_local void @printUInt(i32 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printj(i32 noundef %in0)
    ret void
}
define dso_local void @printUInt64(i64 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printy(i64 noundef %in0)
    ret void
}
define dso_local void @printInt(i32 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printi(i32 noundef %in0)
    ret void
}
define dso_local void @printInt64(i64 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printx(i64 noundef %in0)
    ret void
}
define dso_local void @printFloat(float noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printf(float noundef %in0)
    ret void
}
define dso_local void @printDouble(double noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z5printd(double noundef %in0)
    ret void
}
define dso_local void @printUInt128(i128 noundef %in) local_unnamed_addr #0 {
    ; char chars[39]
    ; char* charsRunningP = chars;
    %chars = alloca [39 x i8]
    ; int128_t val = in
    %valP = alloca i128
    store i128 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 79// 2^128 is a little over 10^39
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 39
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i128, ptr %valP
    %c.0 = urem i128 %val.0, 10
    %c.1 = trunc i128 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [39 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i128 %val.0, 10
    store i128 %val.1, ptr %valP
    %cond2 = icmp eq i128 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [39 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; return
    ret void
}
define dso_local void @printUInt256(i256 noundef %in) local_unnamed_addr #0 {
    ; char chars[78]
    ; char* charsRunningP = chars;
    %chars = alloca [78 x i8]
    ; int256_t val = in
    %valP = alloca i256
    store i256 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 79// 2^256 is a little over 10^77
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 79
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i256, ptr %valP
    %c.0 = urem i256 %val.0, 10
    %c.1 = trunc i256 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i256 %val.0, 10
    store i256 %val.1, ptr %valP
    %cond2 = icmp eq i256 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; return
    ret void
}
define dso_local void @printUInt512(i512 noundef %in) local_unnamed_addr #0 {
    ; char chars[155]
    ; char* charsRunningP = chars;
    %chars = alloca [155 x i8]
    ; int512_t val = in
    %valP = alloca i512
    store i512 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 155// 2^512 is a little over 10^154
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 155
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i512, ptr %valP
    %c.0 = urem i512 %val.0, 10
    %c.1 = trunc i512 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i512 %val.0, 10
    store i512 %val.1, ptr %valP
    %cond2 = icmp eq i512 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [155 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; return
    ret void
}
define dso_local void @printlnChar(i8 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnc(i8 noundef %in0)
    ret void
}
define dso_local void @printlnStr(ptr noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnPKc(ptr noundef %in0)
    ret void
}
define dso_local void @printlnUInt(i32 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnj(i32 noundef %in0)
    ret void
}
define dso_local void @printlnUInt64(i64 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlny(i64 noundef %in0)
    ret void
}
define dso_local void @printlnInt(i32 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlni(i32 noundef %in0)
    ret void
}
define dso_local void @printlnInt64(i64 noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnx(i64 noundef %in0)
    ret void
}
define dso_local void @printlnFloat(float noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnf(float noundef %in0)
    ret void
}
define dso_local void @printlnDouble(double noundef %in0) local_unnamed_addr #0 {
    tail call void @_Z7printlnd(double noundef %in0)
    ret void
}
define dso_local void @println() local_unnamed_addr #0 {
    tail call void @_Z7printlnv()
    ret void
}
define dso_local void @printlnUInt128(i128 noundef %in) local_unnamed_addr #0 {
    ; char chars[39]
    ; char* charsRunningP = chars;
    %chars = alloca [39 x i8]
    ; int128_t val = in
    %valP = alloca i128
    store i128 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 79// 2^128 is a little over 10^39
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 39
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i128, ptr %valP
    %c.0 = urem i128 %val.0, 10
    %c.1 = trunc i128 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [39 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i128 %val.0, 10
    store i128 %val.1, ptr %valP
    %cond2 = icmp eq i128 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [39 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; std::cout << '\n'
    tail call void @println()
    ; return
    ret void
}
define dso_local void @printlnUInt256(i256 noundef %in) local_unnamed_addr #0 {
    ; char chars[78]
    ; char* charsRunningP = chars;
    %chars = alloca [78 x i8]
    ; int256_t val = in
    %valP = alloca i256
    store i256 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 79// 2^256 is a little over 10^77
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 79
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i256, ptr %valP
    %c.0 = urem i256 %val.0, 10
    %c.1 = trunc i256 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i256 %val.0, 10
    store i256 %val.1, ptr %valP
    %cond2 = icmp eq i256 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; std::cout << '\n'
    tail call void @println()
    ; return
    ret void
}
define dso_local void @printlnUInt512(i512 noundef %in) local_unnamed_addr #0 {
    ; char chars[155]
    ; char* charsRunningP = chars;
    %chars = alloca [155 x i8]
    ; int512_t val = in
    %valP = alloca i512
    store i512 %in, ptr %valP
    ; int i = 0
    %i = alloca i64
    store i64 0, ptr %i
    br label %loop1Top
loop1Top:
    ; if i < 155// 2^512 is a little over 10^154
    %i.0 = load i64, ptr %i
    %cond1 = icmp ult i64 %i.0, 155
    br i1 %cond1, label %loop1In, label %loop1Bottom
loop1In:
    ; c = val % 10
    %val.0 = load i512, ptr %valP
    %c.0 = urem i512 %val.0, 10
    %c.1 = trunc i512 %c.0 to i8
    %c.2 = add i8 %c.1, 48; ascii code of '0'
    ; chars[i] = c
    %charP1 = getelementptr [78 x i8], ptr %chars, i64 0, i64 %i.0
    store i8 %c.2, ptr %charP1
    ; val /= 10
    %val.1 = udiv i512 %val.0, 10
    store i512 %val.1, ptr %valP
    %cond2 = icmp eq i512 %val.1, 0
    br i1 %cond2, label %loop1Bottom, label %loop1Increment
loop1Increment:
    ; i++
    %i.1 = add i64 %i.0, 1
    store i64 %i.1, ptr %i
    br label %loop1Top
loop1Bottom:
    
    %i.2 = load i64, ptr %i
    ; int j = 0
    %j = alloca i64
    store i64 0, ptr %j
    br label %loop2Top
loop2Top:
    ; if j < i
    %j.0 = load i64, ptr %j
    %cond3 = icmp ule i64 %j.0, %i.2
    br i1 %cond3, label %loop2In, label %loop2Bottom
loop2In:
    ; c = chars[%i.2-j]
    %tmp = sub i64 %i.2, %j.0
    %charP2 = getelementptr [155 x i8], ptr %chars, i64 0, i64 %tmp
    %c.3 = load i8, ptr %charP2
    tail call void @printChar(i8 noundef %c.3)
    br label %loop2Increment
loop2Increment:
    ; j++
    %j.1 = add i64 %j.0, 1
    store i64 %j.1, ptr %j
    br label %loop2Top
loop2Bottom:
    ; std::cout << '\n'
    tail call void @println()
    ; return
    ret void
}

define dso_local i32 @strToInt(ptr noundef %in0) local_unnamed_addr #0 {
    %out = tail call i32 @_Z8strToIntPKc(ptr noundef %in0)
    ret i32 %out
}
define dso_local i32 @strToUInt(ptr noundef %in0) local_unnamed_addr #0 {
    %out = tail call i32 @_Z9strToUIntPKc(ptr noundef %in0)
    ret i32 %out
}

attributes #0 = { norecurse nosync nounwind mustprogress "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
