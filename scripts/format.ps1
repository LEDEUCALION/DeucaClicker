<#
.SYNOPSIS
    Applique clang-format à l'ensemble des sources, dans la version qu'utilise
    l'intégration continue.

.DESCRIPTION
    Le résultat de clang-format varie d'une version à l'autre. Formater avec
    celle livrée par Visual Studio, puis voir la CI refuser le résultat, est une
    perte de temps qui n'apprend rien à personne : ce script installe et utilise
    exactement la version épinglée.

.PARAMETER Check
    Vérifie sans modifier, et sort en erreur si un fichier n'est pas conforme.
    C'est ce que fait la CI.

.EXAMPLE
    .\scripts\format.ps1

.EXAMPLE
    .\scripts\format.ps1 -Check
#>
[CmdletBinding()]
param(
    [switch]$Check
)

# Volontairement pas 'Stop' : ce script pilote des exécutables natifs, et
# PowerShell 5.1 emballe leur sortie d'erreur dans des ErrorRecord qui feraient
# échouer le script alors que le programme appelé a très bien pu réussir. Les
# échecs sont donc contrôlés à la main, par code de retour.
$ErrorActionPreference = 'Continue'

# Doit rester identique à la version épinglée dans .github/workflows/ci.yml.
$version = '23.1.0'

function Get-ClangFormatPath {
    # Aucun guillemet dans le code Python : PowerShell 5.1 mange les guillemets
    # internes d'un argument passé à un exécutable natif, et l'expression
    # arriverait tronquée de l'autre côté.
    $output = & python -c "import clang_format,os;print(os.path.dirname(clang_format.__file__))" 2>&1
    if ($LASTEXITCODE -ne 0) {
        return $null
    }

    $package = ($output | Select-Object -Last 1).ToString().Trim()
    if ([string]::IsNullOrWhiteSpace($package)) {
        return $null
    }

    $path = Join-Path $package 'data\bin\clang-format.exe'
    if (Test-Path $path) {
        return $path
    }

    return $null
}

$clangFormat = Get-ClangFormatPath
if (-not $clangFormat) {
    Write-Host "Installation de clang-format $version..." -ForegroundColor Cyan
    & python -m pip install --quiet --user "clang-format==$version"
    $clangFormat = Get-ClangFormatPath
}

if (-not $clangFormat) {
    Write-Host "clang-format $version n'a pas pu etre installe. Python est-il disponible ?" -ForegroundColor Red
    exit 1
}

$actual = (& $clangFormat --version) -replace '.*?(\d+\.\d+\.\d+).*', '$1'
if ($actual -ne $version) {
    Write-Warning "clang-format $actual trouve alors que $version est attendu."
    Write-Warning "Le formatage risque de differer de celui de l'integration continue."
}

$root = Split-Path -Parent $PSScriptRoot
$directories = @('src', 'tests', 'bench') |
    ForEach-Object { Join-Path $root $_ } |
    Where-Object { Test-Path $_ }

$files = Get-ChildItem -Path $directories -Recurse -Include *.cpp, *.hpp

if ($Check) {
    $offenders = @()
    foreach ($file in $files) {
        & $clangFormat --dry-run --Werror --style=file $file.FullName | Out-Null
        if ($LASTEXITCODE -ne 0) {
            $offenders += $file.FullName.Substring($root.Length + 1)
        }
    }

    if ($offenders.Count -gt 0) {
        Write-Host "Fichiers non conformes :" -ForegroundColor Red
        $offenders | ForEach-Object { Write-Host "  $_" }
        Write-Host ""
        Write-Host "Lancez .\scripts\format.ps1 pour les corriger."
        exit 1
    }

    Write-Host "$($files.Count) fichiers conformes." -ForegroundColor Green
    exit 0
}

foreach ($file in $files) {
    & $clangFormat -i --style=file $file.FullName
}

Write-Host "$($files.Count) fichiers formates avec clang-format $version." -ForegroundColor Green
