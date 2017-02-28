# Assembles the input file and formats the output in the format used by ml64Output in CodeGenUnitTest.cpp.
Param([string]$InputFile = "TestAsm.asm",
      [string]$OutputFile = "TestAsm.lst",
      [string]$MlPath = "c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\x86_amd64\ml64.exe",
      [int]$IndentLength = 16,
      [int]$QuotedLineLength = 99)

function Expand-Tabs
{
    [CmdletBinding()]
    Param([Parameter(Position = 0,
           ValueFromPipeline = $true)]
          [string]$Text,
          [UInt16]$TabSize = 8)

    process
    {
        $expandedLine = ""
        $numExpansions = 0

        foreach ($portion in ($Text -split "`t"))
        {
            if ($numExpansions -gt 0)
            {
                $expandedLine += " " * ($TabSize - $expandedLine.Length % $TabSize)
            }

            $expandedLine += $portion
            $numExpansions++
        }

        $expandedLine
    }
}

& cmd /c "$MlPath" /nologo /c /Sn /Fl"$OutputFile" "$InputFile" | Out-Null

# Expand tabs and format as [indent]"line   \n".
#
# Note: Masm has what seems to be a bug where it sometimes concatenates the cvtsi2ss
# instruction with its byte code, which confuses ML64Verifier. The replace mitigates that.
$lines = Get-Content "$OutputFile" `
    | Expand-Tabs -TabSize 8 `
    | %{ "{0,-$IndentLength}`"{1,-$QuotedLineLength}\n`"" -f " ", $_ } `
    | %{ $_ -creplace "([^ ])cvtsi2ss", '$1     cvtsi2ss' }

# Add [indent]""; to end the string.
$lines += "{0,-$IndentLength}`"`";" -f " "

# Print the header.
@"
            std::string ml64Output;

            ml64Output +=
"@

$currentLength = 0

foreach ($line in $lines)
{
    $currentLength += $line.Length

    # If next line is likely to overflow, split into a new string. MSVC can't handle strings larger than 64 kB.
    # This is overestimating the length of the line as it also includes the spaces before/after the string.
    if ($currentLength + $line.Length -ge 65535)
    {
@"
$line;

            ml64Output +=
"@

        $currentLength = 0
    }
    else
    {
        $line
    }
}

if ($currentLength -eq 0)
{
    '            "";'
}

@"

            ML64Verifier v(ml64Output.c_str(), start);
"@
