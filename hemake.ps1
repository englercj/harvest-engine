# Copyright Chad Engler

$ErrorActionPreference = 'Stop'
$ForwardedArgs = $args
$callerDir = (Get-Location).Path
$projectPath = $null
$hasProjectArg = $false

function Resolve-FullPath([string] $path)
{
    if ([System.IO.Path]::IsPathRooted($path))
    {
        return [System.IO.Path]::GetFullPath($path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $callerDir $path))
}

for ($i = 0; $i -lt $ForwardedArgs.Count; ++$i)
{
    $arg = $ForwardedArgs[$i]

    if ($arg -eq '--project')
    {
        if (($i + 1) -ge $ForwardedArgs.Count)
        {
            Write-Host 'Missing value for --project'
            exit 1
        }

        $projectPath = Resolve-FullPath $ForwardedArgs[$i + 1]
        $hasProjectArg = $true
        break
    }

    if ($arg.StartsWith('--project='))
    {
        $projectValue = $arg.Substring('--project='.Length).Trim('"')
        $projectPath = Resolve-FullPath $projectValue
        $hasProjectArg = $true
        break
    }
}

if (-not $hasProjectArg)
{
    $projectPath = Resolve-FullPath 'he_project.kdl'
}

if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf))
{
    Write-Host "Project file not found: $projectPath"
    exit 1
}

$scriptDir = [System.IO.Path]::GetDirectoryName($PSCommandPath)
Push-Location -LiteralPath $scriptDir
try
{
    $projectDir = Split-Path -Path $projectPath -Parent
    $buildDir = Join-Path $projectDir '.build'
    $dotnetCliHome = Join-Path $buildDir '.dotnet'

    $dotnetChannel = '10.0'
    $dotnetDir = Join-Path $buildDir 'dotnet'
    $dotnetExe = Join-Path $dotnetDir 'dotnet.exe'

    $hemakeBuildCfg = 'Release'
    $hemakeBuildDir = Join-Path $buildDir 'hemake'
    $hemakeBuildAssembly = Join-Path $hemakeBuildDir "bin/$hemakeBuildCfg/net$dotnetChannel/Harvest.Make.CLI.dll"

    $hemakeSrcDir = Join-Path $scriptDir 'hemake'
    $hemakeSrcProj = Join-Path $hemakeSrcDir 'Harvest.Make.CLI/Harvest.Make.CLI.csproj'

    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    New-Item -ItemType Directory -Path $dotnetCliHome -Force | Out-Null

    $env:DOTNET_CLI_HOME = $dotnetCliHome
    $env:DOTNET_NOLOGO = '1'
    $env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'

    if (-not (Test-Path -LiteralPath $dotnetExe -PathType Leaf))
    {
        Write-Host "Downloading .NET SDK $dotnetChannel..."

        $dotnetInstallScript = Join-Path $buildDir 'dotnet-install.ps1'
        Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -OutFile $dotnetInstallScript
        & powershell -NoProfile -ExecutionPolicy Bypass -File $dotnetInstallScript -Channel $dotnetChannel -InstallDir $dotnetDir
        if ($LASTEXITCODE -ne 0)
        {
            Write-Host 'Failed to install .NET SDK.'
            exit $LASTEXITCODE
        }
    }

    $needsRebuild = $false
    if (-not (Test-Path -LiteralPath $hemakeBuildAssembly -PathType Leaf))
    {
        $needsRebuild = $true
        Write-Host 'Building hemake (assembly not found)...'
    }
    else
    {
        $assemblyTime = (Get-Item -LiteralPath $hemakeBuildAssembly).LastWriteTimeUtc
        $projectTime = (Get-Item -LiteralPath $hemakeSrcProj).LastWriteTimeUtc
        $sourceChanged = Get-ChildItem -LiteralPath $hemakeSrcDir -Recurse -Filter *.cs |
            Where-Object { $_.LastWriteTimeUtc -gt $assemblyTime } |
            Select-Object -First 1

        if (($projectTime -gt $assemblyTime) -or $sourceChanged)
        {
            $needsRebuild = $true
            Write-Host 'Building hemake (source files updated)...'
        }
    }

    if ($needsRebuild)
    {
        if (-not $env:MSBUILD_VERBOSITY)
        {
            $env:MSBUILD_VERBOSITY = 'quiet'
        }

        & $dotnetExe build $hemakeSrcProj -c $hemakeBuildCfg -v $env:MSBUILD_VERBOSITY
        if ($LASTEXITCODE -ne 0)
        {
            Write-Host 'Build failed.'
            exit $LASTEXITCODE
        }
    }

    $cliArgs = if ($hasProjectArg) { $ForwardedArgs } else { @('--project', $projectPath) + $ForwardedArgs }
    & $dotnetExe $hemakeBuildAssembly @cliArgs
    exit $LASTEXITCODE
}
finally
{
    Pop-Location
}
