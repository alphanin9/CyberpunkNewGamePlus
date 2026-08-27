<#
.SYNOPSIS
    Build-system smoketest: checks dependency hygiene, then that the project builds.

.DESCRIPTION
    Guards the invariants that nested git submodules quietly break:
      1. No dependency is vendored more than once across the submodule tree.
      2. Every .gitmodules entry actually has a gitlink (no stale entries).
      3. No translation unit sees two include dirs that shadow each other.
      4. The project still builds.

.EXAMPLE
    pwsh tools/smoketest.ps1
    pwsh tools/smoketest.ps1 -NoBuild
#>
[CmdletBinding()]
param(
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
Push-Location $repo

$script:failures = 0

function Start-Check($name) { Write-Host "`n=== $name ===" -ForegroundColor Cyan }
function Add-Pass($msg)     { Write-Host "  PASS  $msg" -ForegroundColor Green }
function Add-Fail($msg)     { Write-Host "  FAIL  $msg" -ForegroundColor Red; $script:failures++ }
function Add-Info($msg)     { Write-Host "        $msg" -ForegroundColor DarkGray }

function Get-NormalizedUrl($url) {
    ($url -replace '\.git$', '').TrimEnd('/').ToLowerInvariant()
}

# Enumerate submodules of the repo at $root as objects: Owner, Path, Url, Commit.
# Commit is $null when .gitmodules names a submodule that has no gitlink in HEAD.
function Get-SubmoduleEntries($root, $owner) {
    $modules = Join-Path $root '.gitmodules'
    if (-not (Test-Path $modules)) { return @() }

    $lines = & git config -f $modules --get-regexp '^submodule\..*\.(path|url)$' 2>$null
    if (-not $lines) { return @() }

    $byName = @{}
    foreach ($line in $lines) {
        if ($line -notmatch '^submodule\.(.+)\.(path|url)\s+(.+)$') { continue }
        $name, $key, $value = $Matches[1], $Matches[2], $Matches[3].Trim()
        if (-not $byName.ContainsKey($name)) { $byName[$name] = @{} }
        $byName[$name][$key] = $value
    }

    $entries = @()
    foreach ($name in $byName.Keys) {
        $path = $byName[$name]['path']
        $url = $byName[$name]['url']
        if (-not $path -or -not $url) { continue }

        $commit = $null
        $tree = & git -C $root ls-tree HEAD $path 2>$null
        if ($tree -and $tree -match '^160000\s+commit\s+(\w{40})') { $commit = $Matches[1] }

        $entries += [pscustomobject]@{
            Owner  = $owner
            Path   = $path
            Url    = $url
            Commit = $commit
        }
    }
    $entries
}

# Root submodules, plus one level down into the repos we control. Upstream deps
# (archivexl, tweakxl) vendor their own copies of nameof/wil/etc, but we only
# consume their support/ headers, so their internal duplication is not ours.
$ownedSubmodules = @('deps/sharedpunk')

$all = @(Get-SubmoduleEntries $repo '.')
foreach ($e in @($all)) {
    if ($ownedSubmodules -notcontains $e.Path) { continue }
    $nested = Join-Path $repo $e.Path
    if (Test-Path (Join-Path $nested '.git')) {
        $all += Get-SubmoduleEntries $nested $e.Path
    }
}

# --- Check 1: each dependency vendored exactly once -------------------------
Start-Check 'Dependencies are not vendored twice'
$dupes = $all | Group-Object { Get-NormalizedUrl $_.Url } | Where-Object { $_.Count -gt 1 }
if ($dupes) {
    foreach ($d in $dupes) {
        $commits = $d.Group | ForEach-Object { $_.Commit } | Select-Object -Unique
        $verdict = 'same commit'
        if ($commits.Count -gt 1) { $verdict = 'DIVERGENT commits' }
        Add-Fail "$($d.Name) vendored $($d.Count)x ($verdict)"
        foreach ($g in $d.Group) {
            $short = '<no gitlink>'
            if ($g.Commit) { $short = $g.Commit.Substring(0, 7) }
            Add-Info "$($g.Owner)/$($g.Path) @ $short"
        }
    }
} else {
    Add-Pass "$($all.Count) submodule entries, no duplicate upstreams"
}

# --- Check 2: no stale .gitmodules entries ----------------------------------
Start-Check '.gitmodules entries all have gitlinks'
$stale = $all | Where-Object { -not $_.Commit }
if ($stale) {
    foreach ($s in $stale) {
        Add-Fail "$($s.Owner)/.gitmodules declares '$($s.Path)' but HEAD has no gitlink for it"
    }
} else {
    Add-Pass 'every declared submodule is present in HEAD'
}

# --- Check 3: no shadowed include dirs --------------------------------------
# Two in-repo include dirs on the same compile line that share a top-level entry
# name mean header resolution is decided by dep-traversal order, not by intent.
Start-Check 'No shadowed include directories'
$ccPath = Join-Path $repo 'build\compile_commands.json'
& xmake project -k compile_commands build *>$null
if (-not (Test-Path $ccPath)) {
    Add-Fail "could not generate $ccPath"
} else {
    $cc = Get-Content $ccPath -Raw | ConvertFrom-Json
    $seen = @{}
    $shadowed = 0

    foreach ($unit in $cc) {
        $dirs = @()
        foreach ($arg in $unit.arguments) {
            if ($arg -notmatch '^[-/]I(.+)$') { continue }
            $inc = $Matches[1].Trim('"')
            # xmake emits repo-relative dirs for our targets and absolute ones for
            # xrepo packages; only the former can be joined against $repo.
            if ([System.IO.Path]::IsPathRooted($inc)) { continue }
            try { $full = [System.IO.Path]::GetFullPath((Join-Path $repo $inc)) }
            catch { continue }
            # Only in-repo dirs; xrepo package dirs are the package manager's problem.
            if ($full.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase) -and (Test-Path $full)) {
                $dirs += $full
            }
        }

        for ($i = 0; $i -lt $dirs.Count; $i++) {
            for ($j = $i + 1; $j -lt $dirs.Count; $j++) {
                $a, $b = $dirs[$i], $dirs[$j]
                $an = Get-ChildItem $a -Name -ErrorAction SilentlyContinue
                $bn = Get-ChildItem $b -Name -ErrorAction SilentlyContinue
                $common = $an | Where-Object { $bn -contains $_ }
                if (-not $common) { continue }

                $key = "$a|$b"
                if ($seen.ContainsKey($key)) { continue }
                $seen[$key] = $true
                $shadowed++

                $ra = $a.Substring($repo.Length).TrimStart('\')
                $rb = $b.Substring($repo.Length).TrimStart('\')
                Add-Fail "'$ra' shadows '$rb' (both provide: $($common -join ', '))"
                Add-Info "first on the command line wins; e.g. in $($unit.file)"
            }
        }
    }

    if ($shadowed -eq 0) { Add-Pass "$($cc.Count) translation units, no overlapping include dirs" }
}

# --- Check 4: it builds -----------------------------------------------------
if ($NoBuild) {
    Write-Host "`n=== Build (skipped) ===" -ForegroundColor DarkGray
} else {
    Start-Check 'Project builds'
    & xmake build *>$null
    if ($LASTEXITCODE -eq 0) { Add-Pass 'xmake build succeeded' }
    else { Add-Fail "xmake build failed (exit $LASTEXITCODE)" }
}

Pop-Location
Write-Host ''
if ($script:failures -gt 0) {
    Write-Host "$script:failures check(s) failed." -ForegroundColor Red
    exit 1
}
Write-Host 'All checks passed.' -ForegroundColor Green
exit 0
