param(
	[Parameter(Mandatory = $true)]
	[string]$X86Dll,

	[Parameter(Mandatory = $true)]
	[string]$X64Dll,

	[Parameter(Mandatory = $true)]
	[string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$x86Path = (Resolve-Path -LiteralPath $X86Dll).Path
$x64Path = (Resolve-Path -LiteralPath $X64Dll).Path
$outputFullPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
$outputDirectory = Split-Path -Parent $outputFullPath

if (-not (Test-Path -LiteralPath $outputDirectory)) {
	New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}

$tempRoot = Join-Path $env:TEMP ("foo_playlist_search_package_" + [Guid]::NewGuid().ToString("N"))
$x64Directory = Join-Path $tempRoot "x64"

try {
	New-Item -ItemType Directory -Path $x64Directory | Out-Null
	Copy-Item -LiteralPath $x86Path -Destination (Join-Path $tempRoot "foo_playlist_search.dll")
	Copy-Item -LiteralPath $x64Path -Destination (Join-Path $x64Directory "foo_playlist_search.dll")

	if (Test-Path -LiteralPath $outputFullPath) {
		Remove-Item -LiteralPath $outputFullPath -Force
	}

	Add-Type -AssemblyName System.IO.Compression.FileSystem
	[System.IO.Compression.ZipFile]::CreateFromDirectory($tempRoot, $outputFullPath)
	Write-Host "Created $outputFullPath"
} finally {
	if (Test-Path -LiteralPath $tempRoot) {
		Remove-Item -LiteralPath $tempRoot -Recurse -Force
	}
}
