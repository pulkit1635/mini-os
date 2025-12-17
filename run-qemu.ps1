param(
    [string]$Iso = "",
    [int]$Memory = 256,
    [switch]$X64,
    [switch]$Whpx
)

$ErrorActionPreference = 'Stop'

# Prefer minios.iso in repo, else first *.iso under repo
if (-not $Iso) {
    $defaultIso = Join-Path $PSScriptRoot "minios.iso"
    if (Test-Path $defaultIso) {
        $Iso = (Resolve-Path $defaultIso).Path
    } else {
        $found = Get-ChildItem -Path $PSScriptRoot -Filter *.iso -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { $Iso = $found.FullName }
    }
}

$exe = if ($X64) { 'qemu-system-x86_64' } else { 'qemu-system-i386' }

# Build argument list
$argsList = @('-M','pc','-m',"$Memory")

if ($Iso) {
    $argsList += @('-cdrom', $Iso, '-boot', 'd')
} else {
    $kernel = Join-Path $PSScriptRoot 'build\kernel.bin'
    if (-not (Test-Path $kernel)) {
        Write-Error 'No ISO found and build\\kernel.bin not present. Provide -Iso or build the kernel first.'
        exit 1
    }
    $argsList += @('-kernel', $kernel)
}

$argsList += @('-serial','stdio')
if ($Whpx) { $argsList += @('-accel','whpx') }

Write-Host "[RUN] $exe $($argsList -join ' ')"
& $exe @argsList
