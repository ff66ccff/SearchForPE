param(
    [string]$ProcessName = "SportExamSearch_new",
    [string]$Out = "D:\SearchForPE\tools\out\app.png",
    [int]$WaitMs = 2000,
    [switch]$NoLaunch,
    [string]$Exe = "D:\SearchForPE\dist\SportExamSearch_new.exe",
    [string]$Keys = ""
)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public class Win32Shot {
    public struct RECT { public int Left, Top, Right, Bottom; }
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder s, int n);
    public static IntPtr FindByPid(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr h, IntPtr l) {
            uint wpid; GetWindowThreadProcessId(h, out wpid);
            if (wpid == pid && IsWindowVisible(h)) { found = h; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
[Win32Shot]::SetProcessDPIAware() | Out-Null

if (-not $NoLaunch) {
    Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 300
    Start-Process -FilePath $Exe | Out-Null
    Start-Sleep -Milliseconds $WaitMs
}

$proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) { Write-Output "PROCESS NOT FOUND"; exit 1 }
$hwnd = [Win32Shot]::FindByPid([uint32]$proc.Id)
if ($hwnd -eq [IntPtr]::Zero) { Write-Output ("WINDOW NOT FOUND pid=" + $proc.Id); exit 1 }
[Win32Shot]::ShowWindow($hwnd, 5) | Out-Null
[Win32Shot]::SetCursorPos(6, 6) | Out-Null
[Win32Shot]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 450

if ($Keys -ne "") {
    [System.Windows.Forms.SendKeys]::SendWait($Keys)
    Start-Sleep -Milliseconds 700
}

$r = New-Object Win32Shot+RECT
[Win32Shot]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$w = $r.Right - $r.Left
$h = $r.Bottom - $r.Top
Write-Output ("rect: {0},{1} {2}x{3}" -f $r.Left, $r.Top, $w, $h)
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$sz = New-Object System.Drawing.Size $w, $h
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $sz)
$g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output ("saved " + $Out)