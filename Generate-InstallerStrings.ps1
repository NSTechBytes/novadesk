<#
  Generates installer/InstallerStrings.nsh from installer.* keys in locales/*.json.
  Run this after changing an installer translation:
    .\Generate-InstallerStrings.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$localeDirectory = Join-Path $root 'locales'
$outputPath = Join-Path $root 'installer\InstallerStrings.nsh'

# Keep this mapping aligned with the MUI_LANGUAGE entries in installer/setup.nsi.
$languages = [ordered]@{
  'en' = 'English';       'ar' = 'Arabic';        'bg' = 'Bulgarian'
  'zh-Hans' = 'SimpChinese'; 'zh-Hant' = 'TradChinese'; 'hr' = 'Croatian'
  'cs' = 'Czech';         'da' = 'Danish';        'nl' = 'Dutch'
  'et' = 'Estonian';      'fi' = 'Finnish';       'fr' = 'French'
  'de' = 'German';        'el' = 'Greek';         'he' = 'Hebrew'
  'hu' = 'Hungarian';     'id' = 'Indonesian';    'it' = 'Italian'
  'ja' = 'Japanese';      'ko' = 'Korean';        'lv' = 'Latvian'
  'lt' = 'Lithuanian';    'ms' = 'Malay';         'nb' = 'Norwegian'
  'pl' = 'Polish';        'pt-BR' = 'PortugueseBR'; 'pt-PT' = 'Portuguese'
  'ro' = 'Romanian';      'ru' = 'Russian';       'sr-Cyrl' = 'Serbian'
  'sr-Latn' = 'SerbianLatin'; 'sk' = 'Slovak';     'sl' = 'Slovenian'
  'es' = 'Spanish';       'sv' = 'Swedish';       'th' = 'Thai'
  'tr' = 'Turkish';       'uk' = 'Ukrainian';     'vi' = 'Vietnamese'
}

$strings = [ordered]@{
  'installer.run' = 'InstallerRunNovadesk'
  'installer.chooseInstallationType' = 'InstallerChooseInstallationType'
  'installer.standardRecommended' = 'InstallerStandardRecommended'
  'installer.portable' = 'InstallerPortable'
  'installer.uninstallOptions' = 'InstallerUninstallOptions'
  'installer.completeRemove' = 'InstallerCompleteRemove'
}

if (-not (Test-Path -LiteralPath $localeDirectory)) {
  throw "Locale directory is missing: $localeDirectory"
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('; Generated from locales/*.json by Generate-InstallerStrings.ps1. Do not edit by hand.')

foreach ($language in $languages.GetEnumerator()) {
  $catalogPath = Join-Path $localeDirectory "$($language.Key).json"
  if (-not (Test-Path -LiteralPath $catalogPath)) {
    throw "Missing locale catalog: $catalogPath"
  }

  try {
    $catalog = Get-Content -Raw -Encoding utf8 -LiteralPath $catalogPath |
      ConvertFrom-Json
  } catch {
    throw "Invalid JSON in ${catalogPath}: $($_.Exception.Message)"
  }

  foreach ($string in $strings.GetEnumerator()) {
    $property = $catalog.PSObject.Properties[$string.Key]
    if ($null -eq $property -or $property.Value -isnot [string] -or
        [string]::IsNullOrWhiteSpace($property.Value)) {
      throw "$($language.Key).json: missing non-empty '$($string.Key)'"
    }

    # Escape the NSIS characters that have syntax meaning inside a LangString.
    $value = $property.Value.Replace('$', '$$').Replace('"', '$\"').
      Replace("`r", '').Replace("`n", '$\r$\n')
    $lines.Add("LangString $($string.Value) `${LANG_$($language.Value.ToUpperInvariant())} `"$value`"")
  }
}

[System.IO.File]::WriteAllLines(
  $outputPath, $lines, [System.Text.UTF8Encoding]::new($true))

Write-Host "Generated $outputPath with $($lines.Count - 1) language strings." -ForegroundColor Green
