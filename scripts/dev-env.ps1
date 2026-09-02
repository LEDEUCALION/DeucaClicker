<#
.SYNOPSIS
    Prépare la session courante pour compiler DeucaClicker.

.DESCRIPTION
    CMake, Ninja et vcpkg sont livrés à l'intérieur de Visual Studio et ne sont
    pas sur le PATH système. Ce script localise l'installation par vswhere,
    entre dans l'environnement de développement, puis ajoute les outils
    manquants.

    Rien de tout cela ne survit à la fermeture du terminal : il faut recharger
    le script à chaque nouvelle session.

.EXAMPLE
    . .\scripts\dev-env.ps1

    Le point initial est obligatoire. Sans lui, PowerShell exécute le script
    dans une portée fille et tout ce qu'il pose disparaît en sortant.
#>

if ($MyInvocation.InvocationName -ne '.') {
    Write-Warning "Ce script doit etre charge avec un point : . .\scripts\dev-env.ps1"
    Write-Warning "Sans cela, l'environnement disparait des la fin du script."
    return
}

$installerRoot = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
$vswhere = Join-Path $installerRoot 'vswhere.exe'

if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere introuvable : $vswhere. Visual Studio est-il installe ?"
    return
}

# -products * couvre Community, Professional et Enterprise sans les nommer.
$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

if ([string]::IsNullOrWhiteSpace($vsPath)) {
    Write-Error "Aucune installation Visual Studio avec les outils C++ n'a ete trouvee."
    return
}

$devShell = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path $devShell)) {
    Write-Error "Module DevShell introuvable : $devShell"
    return
}

# vcvars, appelé en interne par Enter-VsDevShell, résout vswhere par un chemin
# relatif à sa propre position — lequel n'existe plus depuis Visual Studio 18,
# où l'installateur est resté sous Program Files (x86). Sans cet ajout, la
# console affiche une erreur qui n'empêche rien mais inquiète pour rien.
$env:PATH = "$installerRoot;$env:PATH"

Import-Module $devShell
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation `
    -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null

# Les outils embarqués, ajoutés seulement s'ils existent : leur emplacement a
# déjà changé entre versions de Visual Studio, et un PATH pollué de chemins
# morts masque les vrais problèmes.
$extensions = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake'
foreach ($tool in @('CMake\bin', 'Ninja')) {
    $dir = Join-Path $extensions $tool
    if (Test-Path $dir) {
        $env:PATH = "$dir;$env:PATH"
    }
}

$vcpkg = Join-Path $vsPath 'VC\vcpkg'
if (Test-Path $vcpkg) {
    $env:VCPKG_ROOT = $vcpkg
}

# La racine du dépôt se déduit de la position du script, jamais d'un chemin
# codé en dur : le dépôt n'est pas cloné au même endroit chez tout le monde.
Set-Location (Split-Path -Parent $PSScriptRoot)

function Show-Tool([string]$name) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    if ($command) {
        Write-Host ("  {0,-6}: {1}" -f $name, $command.Source)
    }
    else {
        Write-Host ("  {0,-6}: INTROUVABLE" -f $name) -ForegroundColor Yellow
    }
}

Write-Host "Environnement pret." -ForegroundColor Green
Show-Tool 'cmake'
Show-Tool 'ninja'
Show-Tool 'cl'
Write-Host ("  vcpkg : {0}" -f $env:VCPKG_ROOT)
Write-Host ""
Write-Host "  cmake --preset x64-release"
Write-Host "  cmake --build --preset x64-release"
Write-Host "  ctest --preset x64-release"
