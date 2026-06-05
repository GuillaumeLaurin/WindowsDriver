$ErrorActionPreference = "Stop"
$PropertiesFile = "$PSScriptRoot/c_cpp_properties.json"

$WdkRoot = "C:/Program Files (x86)/Windows Kits/10"
$RegKey  = "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots"
if (Test-Path $RegKey) {
    $val = (Get-ItemProperty $RegKey -ErrorAction SilentlyContinue).KitsRoot10
    if ($val) { $WdkRoot = $val.TrimEnd('\') }
}
Write-Host "WDK root   : $WdkRoot"

$WdkVersion = Get-ChildItem "$WdkRoot/Include" -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    Sort-Object { [Version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name
if (-not $WdkVersion) { Write-Error "No WDK version found"; exit 1 }
Write-Host "WDK version: $WdkVersion"

$KmdfMajor = $null; $KmdfMinor = $null
$KmdfVersion = Get-ChildItem "$WdkRoot/Include/wdf/kmdf" -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^\d+\.\d+$' } |
    Sort-Object { [Version]$_.Name } -Descending |
    Select-Object -First 1 -ExpandProperty Name
if ($KmdfVersion) {
    $KmdfMajor, $KmdfMinor = $KmdfVersion -split '\.'
    Write-Host "KMDF       : $KmdfVersion"
} else { Write-Warning "KMDF not found" }

$ClPath = $null
$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $VsWhere) {
    $InstallPath = ([string](& $VsWhere -latest -products * -property installationPath 2>$null)).Trim()
    if ($InstallPath) {
        $MsvcDir = Join-Path $InstallPath "VC\Tools\MSVC"
        if (Test-Path $MsvcDir) {
            $cl = [string](Get-ChildItem $MsvcDir -Directory |
                Sort-Object Name -Descending | Select-Object -First 1 |
                ForEach-Object { Join-Path $_.FullName "bin\Hostx64\x64\cl.exe" })
            if (Test-Path $cl) { $ClPath = $cl }
        }
    }
}
if ($ClPath) { Write-Host "cl.exe     : $ClPath" }
else { Write-Warning "cl.exe not found"; $ClPath = "" }

$Includes = @(
    "$WdkRoot/Include/$WdkVersion/km",
    "$WdkRoot/Include/$WdkVersion/shared",
    "$WdkRoot/Include/$WdkVersion/um",
    "`${workspaceFolder}/include"
)
if ($KmdfVersion) { $Includes = @("$WdkRoot/Include/wdf/kmdf/$KmdfVersion") + $Includes }
$Includes = $Includes | ForEach-Object { $_ -replace '\\', '/' }

$Defines = [System.Collections.Generic.List[string]]@(
    "_AMD64_", "_KERNEL_MODE","KERNEL_MODE","NTDDI_VERSION=0x0A000010",
    "_WIN32_WINNT=0x0A00","WINVER=0x0A00","WIN32_LEAN_AND_MEAN",
    "POOL_TAG=0x4B445257","_NO_CRT_STDIO_INLINE","NO_STDLIB_INIT","DBG=1","DRIVER_DBG"
)
if ($KmdfVersion) {
    $Defines.Add("USE_KMDF")
    $Defines.Add("KMDF_VERSION_MAJOR=$KmdfMajor")
    $Defines.Add("KMDF_VERSION_MINOR=$KmdfMinor")
}

$Json = [ordered]@{
    configurations = @([ordered]@{
        name             = "WDK Kernel Driver (x64)"
        intelliSenseMode = "windows-msvc-x64"
        compilerPath     = ($ClPath -replace '\\','/')
        includePath      = $Includes
        defines          = $Defines.ToArray()
        cStandard        = "c17"
        cppStandard      = "c++17"
        compileCommands  = "`${workspaceFolder}/build/debug/compile_commands.json"
    })
    version = 4
}
$Json | ConvertTo-Json -Depth 10 | Set-Content $PropertiesFile -Encoding UTF8
Write-Host "`nc_cpp_properties.json updated." -ForegroundColor Green
Write-Host "Reload VS Code: Ctrl+Shift+P > 'Developer: Reload Window'"