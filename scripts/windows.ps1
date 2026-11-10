[CmdletBinding()]
param(
    [ValidateSet("Setup", "Configure", "Build", "Test", "Run")]
    [string]$Action = "Build",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("Client", "Core")]
    [string]$Target = "Client"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildRoot = Join-Path $RepoRoot "build\Windows-$Configuration"
$VcpkgRoot = Join-Path $RepoRoot "build\vcpkg"
$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
$VcpkgInstalled = Join-Path $RepoRoot "build\vcpkg_installed"
$LuaJitSource = Join-Path $RepoRoot "external\LuaJIT\src"

function Invoke-Native {
    param(
        [Parameter(Mandatory)] [string]$FilePath,
        [Parameter()] [string[]]$ArgumentList = @()
    )

    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($ArgumentList -join ' ')"
    }
}

function Find-VcVars64 {
    $fromEnvironment = if ($env:VSINSTALLDIR) {
        Join-Path $env:VSINSTALLDIR "VC\Auxiliary\Build\vcvars64.bat"
    }
    else {
        $null
    }

    if ($fromEnvironment -and (Test-Path -LiteralPath $fromEnvironment)) {
        return $fromEnvironment
    }

    $visualStudioRoot = Join-Path $env:ProgramFiles "Microsoft Visual Studio"
    if (Test-Path -LiteralPath $visualStudioRoot) {
        $candidate = Get-ChildItem -LiteralPath $visualStudioRoot -Directory |
            Sort-Object Name -Descending |
            ForEach-Object {
                Get-ChildItem -LiteralPath $_.FullName -Directory -ErrorAction SilentlyContinue |
                    ForEach-Object { Join-Path $_.FullName "VC\Auxiliary\Build\vcvars64.bat" }
            } |
            Where-Object { Test-Path -LiteralPath $_ } |
            Select-Object -First 1

        if ($candidate) {
            return $candidate
        }
    }

    throw "Visual Studio with the Desktop development with C++ workload is required."
}

function Initialize-Submodules {
    Invoke-Native -FilePath "git" -ArgumentList @(
        "-C", $RepoRoot, "submodule", "update", "--init", "--recursive"
    )
}

function Initialize-Vcpkg {
    $manifest = Get-Content -Raw (Join-Path $RepoRoot "vcpkg.json") | ConvertFrom-Json
    $baseline = $manifest.'builtin-baseline'
    if (-not $baseline) {
        throw "vcpkg.json must contain a builtin-baseline."
    }

    if (-not (Test-Path -LiteralPath (Join-Path $VcpkgRoot ".git"))) {
        New-Item -ItemType Directory -Force -Path (Split-Path $VcpkgRoot) | Out-Null
        Invoke-Native -FilePath "git" -ArgumentList @(
            "clone", "--filter=blob:none", "--no-checkout",
            "https://github.com/microsoft/vcpkg.git", $VcpkgRoot
        )
    }

    $currentRevision = (& git -C $VcpkgRoot rev-parse HEAD 2>$null)
    if ($LASTEXITCODE -ne 0 -or $currentRevision -ne $baseline) {
        Invoke-Native -FilePath "git" -ArgumentList @("-C", $VcpkgRoot, "checkout", $baseline)
    }

    if (-not (Test-Path -LiteralPath $VcpkgExe)) {
        Invoke-Native -FilePath (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -ArgumentList @("-disableMetrics")
    }
}

function Build-LuaJit {
    $library = Join-Path $LuaJitSource "lua51.lib"
    $runtime = Join-Path $LuaJitSource "lua51.dll"
    if ((Test-Path -LiteralPath $library) -and (Test-Path -LiteralPath $runtime)) {
        Write-Host "LuaJIT is already built."
        return
    }

    $vcVars64 = Find-VcVars64
    $command = "call `"$vcVars64`" && cd /d `"$LuaJitSource`" && call msvcbuild.bat"
    Invoke-Native -FilePath "cmd.exe" -ArgumentList @("/d", "/s", "/c", $command)

    if (-not (Test-Path -LiteralPath $library) -or -not (Test-Path -LiteralPath $runtime)) {
        throw "LuaJIT reported success but lua51.lib or lua51.dll is missing."
    }
}

function Install-Dependencies {
    Invoke-Native -FilePath $VcpkgExe -ArgumentList @(
        "install",
        "--triplet", "x64-windows",
        "--x-manifest-root=$RepoRoot",
        "--x-install-root=$VcpkgInstalled"
    )
}

function Invoke-Setup {
    Initialize-Submodules
    Build-LuaJit
    Initialize-Vcpkg
    Install-Dependencies
}

function Invoke-Configure {
    Invoke-Setup
    $toolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
    Invoke-Native -FilePath "cmake" -ArgumentList @(
        "-S", $RepoRoot,
        "-B", $BuildRoot,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=$Configuration",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
        "-DVCPKG_INSTALLED_DIR=$VcpkgInstalled",
        "-DVCPKG_TARGET_TRIPLET=x64-windows"
    )
}

function Compile-Shaders {
    param([Parameter(Mandatory)] [string]$BuildTarget)

    $shaderExtensions = @(".vert", ".frag", ".comp", ".geom", ".tesc", ".tese")
    $shaderRoots = @((Join-Path $RepoRoot "res\shaders"))
    $targetShaderRoot = Join-Path $RepoRoot "tests\$BuildTarget\res\shaders"
    if (Test-Path -LiteralPath $targetShaderRoot) {
        $shaderRoots += $targetShaderRoot
    }

    $shaders = $shaderRoots |
        ForEach-Object { Get-ChildItem -LiteralPath $_ -Recurse -File } |
        Where-Object { $_.Extension -in $shaderExtensions }

    foreach ($shader in $shaders) {
        $output = "$($shader.FullName).spv"
        if (-not (Test-Path -LiteralPath $output) -or $shader.LastWriteTimeUtc -gt (Get-Item $output).LastWriteTimeUtc) {
            Invoke-Native -FilePath "glslangValidator" -ArgumentList @("-V", $shader.FullName, "-o", $output)
        }
    }
}

function Invoke-Build {
    param([Parameter(Mandatory)] [string]$BuildTarget)

    Invoke-Configure
    Compile-Shaders -BuildTarget $BuildTarget
    Invoke-Native -FilePath "cmake" -ArgumentList @(
        "--build", $BuildRoot, "--target", $BuildTarget, "--parallel"
    )
}

function Stage-Resources {
    param([Parameter(Mandatory)] [string]$ExecutableTarget)

    $targetRoot = Join-Path $BuildRoot "tests\$ExecutableTarget"
    $resourceRoot = Join-Path $targetRoot "res"
    if (Test-Path -LiteralPath $resourceRoot) {
        Remove-Item -LiteralPath $resourceRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $resourceRoot | Out-Null

    foreach ($category in @("lua", "textures", "shaders", "fonts", "sounds", "models")) {
        $categoryRoot = Join-Path $resourceRoot $category
        $engineTarget = Join-Path $categoryRoot "mle"
        New-Item -ItemType Directory -Force -Path $engineTarget | Out-Null

        $engineSource = Join-Path $RepoRoot "res\$category"
        if (Test-Path -LiteralPath $engineSource) {
            Copy-Item -Path (Join-Path $engineSource "*") -Destination $engineTarget -Recurse -Force
        }

        $clientSource = Join-Path $RepoRoot "tests\$ExecutableTarget\res\$category"
        if (Test-Path -LiteralPath $clientSource) {
            $clientTarget = Join-Path $categoryRoot "i"
            New-Item -ItemType Directory -Force -Path $clientTarget | Out-Null
            Copy-Item -Path (Join-Path $clientSource "*") -Destination $clientTarget -Recurse -Force
        }
    }
}

function Stage-Runtime {
    param([Parameter(Mandatory)] [string]$ExecutableTarget)

    $targetRoot = Join-Path $BuildRoot "tests\$ExecutableTarget"
    Copy-Item -LiteralPath (Join-Path $LuaJitSource "lua51.dll") -Destination $targetRoot -Force

    $vcpkgBin = if ($Configuration -eq "Debug") {
        Join-Path $VcpkgInstalled "x64-windows\debug\bin"
    }
    else {
        Join-Path $VcpkgInstalled "x64-windows\bin"
    }
    if (Test-Path -LiteralPath $vcpkgBin) {
        Get-ChildItem -LiteralPath $vcpkgBin -Filter "*.dll" -File |
            Copy-Item -Destination $targetRoot -Force
    }

    $sdlRuntime = Get-ChildItem $BuildRoot -Recurse -Filter "SDL3.dll" -File |
        Select-Object -First 1
    if (-not $sdlRuntime) {
        throw "SDL3.dll was not produced by the build."
    }
    Copy-Item -LiteralPath $sdlRuntime.FullName -Destination $targetRoot -Force
}

function Prepare-Executable {
    param([Parameter(Mandatory)] [string]$ExecutableTarget)

    Stage-Resources -ExecutableTarget $ExecutableTarget
    Stage-Runtime -ExecutableTarget $ExecutableTarget
}

switch ($Action) {
    "Setup" {
        Invoke-Setup
    }
    "Configure" {
        Invoke-Configure
    }
    "Build" {
        Invoke-Build -BuildTarget $Target
    }
    "Test" {
        Invoke-Build -BuildTarget "Core"
        Prepare-Executable -ExecutableTarget "Core"
        $executable = Join-Path $BuildRoot "tests\Core\Core.exe"
        Push-Location (Split-Path $executable)
        try {
            Invoke-Native -FilePath $executable
        }
        finally {
            Pop-Location
        }
    }
    "Run" {
        Invoke-Build -BuildTarget "Client"
        Prepare-Executable -ExecutableTarget "Client"
        $executable = Join-Path $BuildRoot "tests\Client\Client.exe"
        Push-Location (Split-Path $executable)
        try {
            Invoke-Native -FilePath $executable
        }
        finally {
            Pop-Location
        }
    }
}
