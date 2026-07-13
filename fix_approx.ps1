$files = Get-ChildItem -Path tests -Filter *.cpp
foreach ($file in $files) {
    $content = Get-Content $file.FullName
    $content = $content -replace "Catch::Approx", "Approx"
    [System.IO.File]::WriteAllLines($file.FullName, $content, [System.Text.Encoding]::UTF8)
}
