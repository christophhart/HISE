$registersRxx = ("al", "cl", "dl", "bl", "spl", "bpl", "dil", "sil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"),
                ("ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"),
                ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"),
                ("rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15")
$registersXmm = ("xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15")

@"
.code
instructions PROC


; Int <-> Int

"@

for ($srcRegId = 0; $srcRegId -lt $registersRxx[0].Count; $srcRegId++)
{
    for ($destRegId = 0; $destRegId -lt $registersRxx[0].Count; $destRegId++)
    {
        $src8 = $registersRxx[0][$srcRegId]
        $src16 = $registersRxx[1][$srcRegId]
        $src32 = $registersRxx[2][$srcRegId]
        $src64 = $registersRxx[3][$srcRegId]

        $dest8 = $registersRxx[0][$destRegId]
        $dest16 = $registersRxx[1][$destRegId]
        $dest32 = $registersRxx[2][$destRegId]
        $dest64 = $registersRxx[3][$destRegId]

@"
; Source: $src64, target: $dest64
mov $dest8, 1
mov $dest16, 1
mov $dest32, 1
mov $dest64, 1

mov $dest8, $src8
mov $dest16, $src16
mov $dest32, $src32
mov $dest64, $src64

mov $dest8, byte ptr [$src64 - 4]
mov $dest8, byte ptr [$src64 + 4]
mov $dest16, word ptr [$src64 + 4]
mov $dest32, dword ptr [$src64 + 4]
mov $dest64, qword ptr [$src64 + 4]

mov byte ptr [$dest64 - 4], $src8
mov byte ptr [$dest64 + 4], $src8
mov word ptr [$dest64 + 4], $src16
mov dword ptr [$dest64 + 4], $src32
mov qword ptr [$dest64 + 4], $src64

movzx $dest16, $src8
movzx $dest16, byte ptr [$src64 + 4]

movzx $dest32, $src8
movzx $dest32, $src16
movzx $dest32, byte ptr [$src64 + 4]
movzx $dest32, word ptr [$src64 + 4]

movzx $dest64, $src8
movzx $dest64, $src16
movzx $dest64, byte ptr [$src64 + 4]
movzx $dest64, word ptr [$src64 + 4]


"@
    }
}


@"
; Float <-> float

"@

foreach ($src in $registersXmm)
{
    foreach ($dest in $registersXmm)
    {
@"
; Source: $src, target: $dest
movss $dest, $src
movsd $dest, $src

cvtss2sd $dest, $src
cvtsd2ss $dest, $src


"@
    }
}


@"
; Float <-> Int

"@

foreach ($xmm in $registersXmm)
{
    for ($rxxRegId = 0; $rxxRegId -lt $registersRxx[0].Count; $rxxRegId++)
    {
        $rxx32 = $registersRxx[2][$rxxRegId]
        $rxx64 = $registersRxx[3][$rxxRegId]

@"
; Between: $xmm and $rxx64
movss dword ptr [$rxx64 - 4], $xmm
movsd qword ptr [$rxx64 + 4], $xmm

movss $xmm, dword ptr [$rxx64 - 4]
movsd $xmm, qword ptr [$rxx64 + 4]
cvtsi2ss $xmm, qword ptr [$rxx64 - 4]
cvtsi2sd $xmm, dword ptr [$rxx64 + 4]

cvttss2si $rxx32, $xmm
cvttsd2si $rxx32, $xmm

cvtsi2ss $xmm, $rxx32
cvtsi2sd $xmm, $rxx32

cvttss2si $rxx64, $xmm
cvttsd2si $rxx64, $xmm

cvtsi2ss $xmm, $rxx64
cvtsi2sd $xmm, $rxx64


"@
    }
}

@"
instructions ENDP

END
"@


@"
            //
            // Int <-> int
            //

"@

for ($srcRegId = 0; $srcRegId -lt $registersRxx[0].Count; $srcRegId++)
{
    for ($destRegId = 0; $destRegId -lt $registersRxx[0].Count; $destRegId++)
    {
        $src8 = $registersRxx[0][$srcRegId]
        $src16 = $registersRxx[1][$srcRegId]
        $src32 = $registersRxx[2][$srcRegId]
        $src64 = $registersRxx[3][$srcRegId]

        $dest8 = $registersRxx[0][$destRegId]
        $dest16 = $registersRxx[1][$destRegId]
        $dest32 = $registersRxx[2][$destRegId]
        $dest64 = $registersRxx[3][$destRegId]
@"
            // Source: $src64, target: $dest64

            buffer.EmitImmediate<OpCode::Mov>($dest8, static_cast<uint8_t>(1));
            buffer.EmitImmediate<OpCode::Mov>($dest16, static_cast<uint16_t>(1));
            buffer.EmitImmediate<OpCode::Mov>($dest32, 1);
            buffer.EmitImmediate<OpCode::Mov>($dest64, 1);

            buffer.Emit<OpCode::Mov>($dest8, $src8);
            buffer.Emit<OpCode::Mov>($dest16, $src16);
            buffer.Emit<OpCode::Mov>($dest32, $src32);
            buffer.Emit<OpCode::Mov>($dest64, $src64);

            buffer.Emit<OpCode::Mov>($dest8, $src64, -4);
            buffer.Emit<OpCode::Mov>($dest8, $src64, 4);
            buffer.Emit<OpCode::Mov>($dest16, $src64, 4);
            buffer.Emit<OpCode::Mov>($dest32, $src64, 4);
            buffer.Emit<OpCode::Mov>($dest64, $src64, 4);

            buffer.Emit<OpCode::Mov>($dest64, -4, $src8);
            buffer.Emit<OpCode::Mov>($dest64, 4, $src8);
            buffer.Emit<OpCode::Mov>($dest64, 4, $src16);
            buffer.Emit<OpCode::Mov>($dest64, 4, $src32);
            buffer.Emit<OpCode::Mov>($dest64, 4, $src64);

            buffer.Emit<OpCode::MovZX>($dest16, $src8);
            buffer.Emit<OpCode::MovZX, 2, false, 1, false>($dest16, $src64, 4);

            buffer.Emit<OpCode::MovZX>($dest32, $src8);
            buffer.Emit<OpCode::MovZX>($dest32, $src16);
            buffer.Emit<OpCode::MovZX, 4, false, 1, false>($dest32, $src64, 4);
            buffer.Emit<OpCode::MovZX, 4, false, 2, false>($dest32, $src64, 4);

            buffer.Emit<OpCode::MovZX>($dest64, $src8);
            buffer.Emit<OpCode::MovZX>($dest64, $src16);
            buffer.Emit<OpCode::MovZX, 8, false, 1, false>($dest64, $src64, 4);
            buffer.Emit<OpCode::MovZX, 8, false, 2, false>($dest64, $src64, 4);


"@
    }
}

@"
            //
            // Float <-> float
            //

"@

foreach ($src in $registersXmm)
{
    foreach ($dest in $registersXmm)
    {
        $src32 = "${src}s"
        $src64 = $src

        $dest32 = "${dest}s"
        $dest64 = $dest
@"
            // Source: $src, target: $dest

            buffer.Emit<OpCode::Mov>($dest32, $src32);
            buffer.Emit<OpCode::Mov>($dest64, $src64);

            buffer.Emit<OpCode::CvtFP2FP>($dest64, $src32);
            buffer.Emit<OpCode::CvtFP2FP>($dest32, $src64);


"@
    }
}


@"
            //
            // Float <-> int
            //

"@

foreach ($xmm in $registersXmm)
{
    for ($rxxRegId = 0; $rxxRegId -lt $registersRxx[0].Count; $rxxRegId++)
    {
        $xmm32 = "${xmm}s"
        $xmm64 = $xmm

        $rxx32 = $registersRxx[2][$rxxRegId]
        $rxx64 = $registersRxx[3][$rxxRegId]

@"
            // Between: $xmm and $rxx64

            buffer.Emit<OpCode::Mov>($rxx64, -4, $xmm32);
            buffer.Emit<OpCode::Mov>($rxx64, 4, $xmm64);

            buffer.Emit<OpCode::Mov>($xmm32, $rxx64, -4);
            buffer.Emit<OpCode::Mov>($xmm64, $rxx64, 4);
            buffer.Emit<OpCode::CvtSI2FP, 4, true, 8, false>($xmm32, $rxx64, -4);
            buffer.Emit<OpCode::CvtSI2FP, 8, true, 4, false>($xmm64, $rxx64, 4);

            buffer.Emit<OpCode::CvtFP2SI>($rxx32, $xmm32);
            buffer.Emit<OpCode::CvtFP2SI>($rxx32, $xmm64);

            buffer.Emit<OpCode::CvtSI2FP>($xmm32, $rxx32);
            buffer.Emit<OpCode::CvtSI2FP>($xmm64, $rxx32);

            buffer.Emit<OpCode::CvtFP2SI>($rxx64, $xmm32);
            buffer.Emit<OpCode::CvtFP2SI>($rxx64, $xmm64);

            buffer.Emit<OpCode::CvtSI2FP>($xmm32, $rxx64);
            buffer.Emit<OpCode::CvtSI2FP>($xmm64, $rxx64);


"@
    }
}
