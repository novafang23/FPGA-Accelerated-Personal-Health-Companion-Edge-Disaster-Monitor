param(
    [string]$PortName = "COM4",
    [int]$BaudRate = 115200,
    [int]$DurationSec = 15,
    [string]$OutputFile = "docs/presentation/serial_capture_2026-10-29.log"
)

$ErrorActionPreference = "Stop"

Write-Host "Connecting to $PortName at $BaudRate baud for $DurationSec seconds..."
$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, "None", 8, "One"
$port.ReadTimeout = 2000
$port.DtrEnable = $true
$port.RtsEnable = $true

$lines = [System.Collections.Generic.List[string]]::new()

try {
    $port.Open()
    Write-Host "Port opened successfully. Capturing incoming serial data..."
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    
    while ($stopwatch.Elapsed.TotalSeconds -lt $DurationSec) {
        try {
            $line = $port.ReadLine()
            if ($line) {
                Write-Host $line
                $lines.Add($line)
            }
        } catch [System.TimeoutException] {
            # Expected if quiet
        }
    }
    Write-Host "Capture finished. Total lines received: $($lines.Count)"
} catch {
    Write-Warning "Serial port error: $_"
} finally {
    if ($port.IsOpen) {
        $port.Close()
        Write-Host "Port $PortName closed."
    }
}

if ($lines.Count -gt 0) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllLines($OutputFile, $lines, $utf8NoBom)
    Write-Host "Saved capture to $OutputFile"
} else {
    Write-Host "No lines captured. If device was reset or idle, log file was not updated."
}
