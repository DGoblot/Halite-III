$bots = @("Colinatole3", "Nelson", "labete")
$seeds = @(1,2,3)

$processes = @()

$i = 1

foreach ($bot in $bots) {
    foreach ($seed in $seeds) {

        $output = "benchmark\score$i.json"

        $p = Start-Process ".\halite.exe" `
            -ArgumentList @(
                "--replay-directory", "replays\",
                "--width", "32",
                "--height", "32",
                "--seed", "$seed",
                "benchmark\$bot.exe",
                "MyBot.exe",
                "--results-as-json"
            ) `
            -NoNewWindow `
            -RedirectStandardOutput $output `
            -PassThru

        $processes += [PSCustomObject]@{
            Process = $p
            Bot = $bot
            Seed = $seed
            Output = $output
        }

        $i++
    }
}

$spinner = @("|","/","-","\")
$i = 0

while ($true) {
    $done = ($processes | Where-Object { $_.Process.HasExited }).Count
    $total = $processes.Count

    $spin = $spinner[$i % $spinner.Length]
    Write-Host "`r$spin Running games... $done/$total" -NoNewline

    if ($done -eq $total) { break }

    $i++
    Start-Sleep -Milliseconds 200
}

Write-Host "`nDone!"

$totalDiff = 0

foreach ($entry in $processes) {
    $json = Get-Content $entry.Output | ConvertFrom-Json

    $botScore = $json.stats.'0'.score
    $myScore  = $json.stats.'1'.score

    $diff = $myScore - $botScore
    $totalDiff += $diff

    Write-Host "--- $($entry.Bot) | Seed $($entry.Seed) ---"
    Write-Host "Bot: $botScore | MyBot: $myScore | Diff: $diff"
}

$runScore = $totalDiff / $processes.Count

$grouped = $processes | Group-Object Bot

foreach ($g in $grouped) {
    $sum = 0

    foreach ($entry in $g.Group) {
        $json = Get-Content $entry.Output | ConvertFrom-Json
        $sum += ($json.stats.'1'.score - $json.stats.'0'.score)
    }

    $avg = $sum / $g.Count
    Write-Host "`n>>> $($g.Name) moyenne: $avg"
}

Write-Host "`nScore du bot:" $runScore

$bestFile = "benchmark\best_score.json"

if (Test-Path $bestFile) {
    $bestData = Get-Content $bestFile | ConvertFrom-Json
    $bestScore = $bestData.bestScore
} else {
    $bestScore = -999999
}

if ($runScore -gt $bestScore) {
    Write-Host "Nouveau record !"

    $bestData = [PSCustomObject]@{
        bestScore = $runScore
        date = (Get-Date)
    }

    $bestData | ConvertTo-Json | Set-Content $bestFile
} else {
    Write-Host "Moins bon que le record actuel ($bestScore)"
}

Pause