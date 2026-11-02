<#
.SYNOPSIS
    One-click build script for the 128x64 mono LCD GUI project (SDCC / mcs51).

.DESCRIPTION
    Usage (PowerShell):
        .\build.ps1                # build -> output/gui.ihx + gui.hex
        .\build.ps1 rebuild        # clean everything, then full build
        .\build.ps1 clean          # remove intermediate files, keep firmware
        .\build.ps1 distclean      # remove intermediate files and firmware
        .\build.ps1 font           # regenerate the Chinese font tables
        .\build.ps1 size           # show memory usage
        .\build.ps1 -NoMake        # force manual sdcc invocation (skip make)

    Double-click build.bat to run this script without touching ExecutionPolicy.

    Requires: sdcc (+ packihx) in PATH. Optional: make, python (font only).
#>

param(
    [ValidateSet("all", "rebuild", "clean", "distclean", "font", "size")]
    [string]$Target = "all",
    [switch]$NoMake
)

$ErrorActionPreference = "Stop"

#---------------------------------------------------------------- paths
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Src  = Join-Path $Root "source"
$Out  = Join-Path $Root "output"
$Ihx  = Join-Path $Out "gui.ihx"
$Hex  = Join-Path $Out "gui.hex"
$Mem  = Join-Path $Out "gui.mem"
$FontTool = Join-Path $Src "Tools/gen_font.py"

#---------------------------------------------------------------- sources & flags
$Srcs = @(
    "main.c",
    "Bsp/Timer.c",
    "Bsp/Key.c",
    "Bsp/LcdHal.c",
    "GUI/GUI.c",
    "GUI/Font.c",
    "GUI/FontAscii.c",
    "GUI/FontCN.c",
    "App/Menu.c",
    "App/MenuRes.c",
    "App/App.c"
)
$CFlags = @("-mmcs51", "--model-large", "--opt-code-speed",
            "--iram-size", "256", "--xram-size", "8192", "--code-size", "65536",
            "-I.", "-I..")
$LdFlags = @("-mmcs51", "--model-large",
             "--iram-size", "256", "--xram-size", "8192", "--code-size", "65536")

#---------------------------------------------------------------- helpers
function Write-Step($msg) { Write-Host "[build] $msg" -ForegroundColor Cyan }
function Write-Err($msg)  { Write-Host "[build] $msg" -ForegroundColor Red }
function Test-Tool($name) { return [bool](Get-Command $name -ErrorAction SilentlyContinue) }

# Locate sdcc.exe.
# 1) already in PATH
# 2) $env:SDCC_HOME\bin
# 3) common install dirs (official installer / chocolatey)
# 4) the toolchain bundled by the "Embedded IDE" VSCode extension
function Find-Sdcc {
    $inPath = Get-Command sdcc -ErrorAction SilentlyContinue
    if ($inPath) { return $inPath.Source }

    $dirs = @()
    if ($env:SDCC_HOME) { $dirs += (Join-Path $env:SDCC_HOME "bin") }
    if ($env:ProgramFiles) { $dirs += (Join-Path $env:ProgramFiles "SDCC/bin") }
    if (${env:ProgramFiles(x86)}) { $dirs += (Join-Path ${env:ProgramFiles(x86)} "SDCC/bin") }

    if ($env:USERPROFILE) {
        $dirs += (Join-Path $env:USERPROFILE ".eide/tools/sdcc/bin")
        $eide = Join-Path $env:USERPROFILE ".eide/tools/sdcc_mcs51"
        if (Test-Path -LiteralPath $eide) {
            $hit = Get-ChildItem -LiteralPath $eide -Recurse -Filter "sdcc.exe" -ErrorAction SilentlyContinue |
                   Select-Object -First 1
            if ($hit) { $dirs += $hit.Directory.FullName }
        }
    }
    foreach ($d in @("C:/SDCC/bin", "D:/SDCC/bin")) {
        if (Test-Path -LiteralPath $d) { $dirs += $d }
    }

    foreach ($d in $dirs) {
        if ($d -and (Test-Path -LiteralPath (Join-Path $d "sdcc.exe"))) {
            return (Join-Path $d "sdcc.exe")
        }
    }
    return $null
}

# Add the folder holding sdcc.exe (and packihx.exe) to PATH for this session
function Use-Sdcc {
    $exe = Find-Sdcc
    if (-not $exe) { return $false }
    $dir = Split-Path -Parent $exe
    if ($env:PATH -notlike ("*" + $dir + "*")) {
        $env:PATH = $dir + ";" + $env:PATH
    }
    Write-Host ("[build] sdcc: " + $exe)
    return $true
}

# Delete through .NET so host-level delete wrappers cannot break the script
function Remove-FileSafe($path) {
    if (Test-Path -LiteralPath $path) {
        try { [System.IO.File]::Delete($path) } catch { }
    }
}

function Remove-Artifacts {
    # Whitelist by extension only.
    # NOTE: never use -Include together with -LiteralPath here: the filter is
    # silently ignored and every file gets deleted (this once wiped the tree).
    $ext = @(".rel", ".sym", ".asm", ".lst", ".rst", ".lk", ".map", ".mem")
    $rootFull = [System.IO.Path]::GetFullPath($Src)
    $n = 0
    $all = @(Get-ChildItem -LiteralPath $Src -Recurse -File)
    $all | ForEach-Object {
        $e = $_.Extension.ToLowerInvariant()
        if ($ext -contains $e) {
            $full = [System.IO.Path]::GetFullPath($_.FullName)
            if ($full.StartsWith($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
                Remove-FileSafe $full
                $n++
            }
        }
    }
    Write-Host ("[build] removed " + $n + " intermediate file(s)")
}

function Remove-Firmware {
    foreach ($f in @($Ihx, $Hex, $Mem, (Join-Path $Out "gui.map"), (Join-Path $Out "gui.lk"))) {
        Remove-FileSafe $f
    }
}

function Show-Size {
    if (Test-Path -LiteralPath $Mem) {
        Write-Host ""
        Write-Host "---- memory usage ----" -ForegroundColor Yellow
        Get-Content -LiteralPath $Mem | Select-Object -Last 8
    }
    if (Test-Path -LiteralPath $Hex) {
        $h = Get-Item -LiteralPath $Hex
        Write-Host ("firmware: {0} ({1} bytes)" -f $h.FullName, $h.Length) -ForegroundColor Green
    }
}

function Invoke-Native {
    # Run a native tool and re-print its stderr as normal output.
    # Redirecting stderr to a file keeps PowerShell from turning tool chatter
    # (e.g. "packihx: read ... OK.") into red error records or, with
    # $ErrorActionPreference = "Stop", into a terminating error.
    param([string]$Exe, [string[]]$Arguments)

    $errFile = [System.IO.Path]::GetTempFileName()
    $proc = Start-Process -FilePath $Exe -ArgumentList $Arguments `
                          -NoNewWindow -Wait -PassThru -RedirectStandardError $errFile
    $code = $proc.ExitCode
    if (Test-Path -LiteralPath $errFile) {
        $lines = @(Get-Content -LiteralPath $errFile)
        foreach ($l in $lines) {
            if ($l.Trim().Length -gt 0) { Write-Host $l }
        }
        Remove-FileSafe $errFile
    }
    return $code
}

function Invoke-Build {
    if (-not (Test-Path -LiteralPath $Out)) {
        New-Item -ItemType Directory -Path $Out | Out-Null
    }

    if ((Test-Tool "make") -and (-not $NoMake)) {
        Write-Step "building with make..."
        $code = Invoke-Native "make" @("all")
        if ($code -ne 0) { throw "make failed" }
        return
    }

    Write-Step "make not found, invoking sdcc directly..."
    $objs = @()
    foreach ($s in $Srcs) {
        $rel = $s -replace '\.c$', '.rel'
        Write-Host "  CC  $s"
        $code = Invoke-Native "sdcc" ($CFlags + @("-c", $s, "-o", $rel))
        if ($code -ne 0) { throw "compile failed: $s" }
        $objs += $rel
    }
    Write-Host "  LD  gui.ihx"
    $code = Invoke-Native "sdcc" ($LdFlags + @("-o", $Ihx) + $objs)
    if ($code -ne 0) { throw "link failed" }

    if (Test-Tool "packihx") {
        $pk = (Get-Command packihx).Source
        $out = & cmd /c "`"$pk`" `"$Ihx`" > `"$Hex`" 2>&1"
        foreach ($l in $out) { Write-Host $l }
        Write-Step "gui.hex generated"
    } else {
        Write-Host "[build] packihx not found, skipped .hex (use .ihx with STC-ISP)" -ForegroundColor Yellow
    }
}

function Invoke-Font {
    Write-Step "generating font tables (CN + ASCII)..."
    $code = Invoke-Native "python" @($FontTool)
    if ($code -ne 0) { throw "font generation failed" }
    $code = Invoke-Native "python" @($FontTool, "--ascii")
    if ($code -ne 0) { throw "ascii font generation failed" }
}

#---------------------------------------------------------------- main
if ((($Target -eq "all") -or ($Target -eq "rebuild")) -and (-not (Use-Sdcc))) {
    Write-Err "sdcc not found."
    Write-Err "  - install SDCC (https://sdcc.sourceforge.net/), or"
    Write-Err "  - set SDCC_HOME to its root folder, e.g."
    Write-Err "      setx SDCC_HOME \"C:\Program Files\SDCC\""
    Write-Err "  - or add its bin folder to PATH (sdcc.exe + packihx.exe)."
    exit 1
}
if (($Target -eq "font") -and (-not (Test-Tool "python"))) {
    Write-Err "python not found in PATH (needed by Tools/gen_font.py)."
    exit 1
}

Push-Location $Src
try {
    switch ($Target) {
        "clean"     { Write-Step "removing intermediate files"; Remove-Artifacts }
        "distclean" { Write-Step "removing intermediate files and firmware"; Remove-Artifacts; Remove-Firmware }
        "font"      { Invoke-Font }
        "size"      { Show-Size }
        "rebuild"   { Remove-Artifacts; Remove-Firmware; Invoke-Build; Show-Size }
        default     { Invoke-Build; Show-Size }
    }
} catch {
    Write-Err ("FAILED: " + $_.Exception.Message)
    Pop-Location
    exit 1
}
Pop-Location

Write-Host "[build] done: $Target" -ForegroundColor Green
