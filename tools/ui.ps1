param(
    [string]$OpsFile = "",
    [string]$ProcessName = "SportExamSearch_new",
    [string]$Exe = "D:\SearchForPE\dist\SportExamSearch_new.exe",
    [switch]$NoLaunch
)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class UI {
    public struct RECT { public int Left, Top, Right, Bottom; }
    public struct POINT { public int X, Y; }
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr ctx);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, IntPtr e);
    [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hWnd);
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
[UI]::SetProcessDpiAwarenessContext([IntPtr]-4) | Out-Null

if (-not $NoLaunch) {
    Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 350
    Start-Process -FilePath $Exe | Out-Null
    Start-Sleep -Milliseconds 2200
}

$proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) { Write-Output "PROCESS NOT FOUND"; exit 1 }
$hwnd = [UI]::FindByPid([uint32]$proc.Id)
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "WINDOW NOT FOUND"; exit 1 }
[UI]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 300
$dpi = [UI]::GetDpiForWindow($hwnd)
if ($dpi -eq 0) { $dpi = 96 }
$scale = $dpi / 96.0
$cr = New-Object UI+RECT
[UI]::GetClientRect($hwnd, [ref]$cr) | Out-Null
$orig = New-Object UI+POINT
[UI]::ClientToScreen($hwnd, [ref]$orig) | Out-Null
Write-Output ("dpi=" + $dpi + " client=" + $cr.Right + "x" + $cr.Bottom + " origin=" + $orig.X + "," + $orig.Y)

function Capture([string]$path) {
    $r = New-Object UI+RECT
    [UI]::GetWindowRect($hwnd, [ref]$r) | Out-Null
    $w = $r.Right - $r.Left
    $h = $r.Bottom - $r.Top
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $sz = New-Object System.Drawing.Size $w, $h
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, $sz)
    $g.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output ("shot " + $path)
}

function MoveDip([double]$dx, [double]$dy) {
    $px = [int][math]::Round($orig.X + $dx * $scale)
    $py = [int][math]::Round($orig.Y + $dy * $scale)
    [UI]::SetCursorPos($px, $py) | Out-Null
    Start-Sleep -Milliseconds 120
}

function ClickDip([double]$dx, [double]$dy) {
    MoveDip $dx $dy
    [UI]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 60
    [UI]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 160
}

if ($OpsFile -ne "") {
    $ops = Get-Content -LiteralPath $OpsFile -Encoding UTF8
    foreach ($line in $ops) {
        if ($line.Trim() -eq "") { continue }
        Write-Output ("op: " + $line)
        $parts = $line -split "`t"
        $verb = $parts[0].Trim()
        $arg = if ($parts.Count -gt 1) { $parts[1] } else { "" }
        switch ($verb) {
            "wait" { Start-Sleep -Milliseconds ([int]$arg) }
            "paste" {
                [System.Windows.Forms.Clipboard]::SetText($arg)
                Start-Sleep -Milliseconds 120
                [System.Windows.Forms.SendKeys]::SendWait("^v")
                Start-Sleep -Milliseconds 500
            }
            "keys" { [System.Windows.Forms.SendKeys]::SendWait($arg); Start-Sleep -Milliseconds 400 }
            "hover" {
                $xy = $arg -split ","
                MoveDip ([double]$xy[0]) ([double]$xy[1])
            }
            "click" {
                $xy = $arg -split ","
                ClickDip ([double]$xy[0]) ([double]$xy[1])
            }
            "scroll" {
                [UI]::mouse_event(0x0800, 0, 0, [uint32]([int]$arg), [IntPtr]::Zero)
                Start-Sleep -Milliseconds 180
            }
            "shot" { Capture $arg.Trim() }
            "smsg" {
                # smsg <verb>,<dipX>,<dipY>  -> posts synthetic mouse messages using client pixel coords
                $bits = $arg -split ","
                $verb2 = $bits[0]
                $lpx = [int][math]::Round(([double]$bits[1]) * $scale)
                $lpy = [int][math]::Round(([double]$bits[2]) * $scale)
                $lp = [IntPtr](($lpy -shl 16) -bor ($lpx -band 0xFFFF))
                if ($verb2 -eq "down") {
                    [UI]::SendMessage($hwnd, 0x0200, [IntPtr]::Zero, $lp) | Out-Null
                    [UI]::SendMessage($hwnd, 0x0201, [IntPtr]1, $lp) | Out-Null
                } elseif ($verb2 -eq "up") {
                    [UI]::SendMessage($hwnd, 0x0202, [IntPtr]::Zero, $lp) | Out-Null
                } elseif ($verb2 -eq "move") {
                    [UI]::SendMessage($hwnd, 0x0200, [IntPtr]::Zero, $lp) | Out-Null
                } elseif ($verb2 -eq "click") {
                    [UI]::SendMessage($hwnd, 0x0200, [IntPtr]::Zero, $lp) | Out-Null
                    [UI]::SendMessage($hwnd, 0x0201, [IntPtr]1, $lp) | Out-Null
                    Start-Sleep -Milliseconds 60
                    [UI]::SendMessage($hwnd, 0x0202, [IntPtr]::Zero, $lp) | Out-Null
                } elseif ($verb2 -eq "key") {
                    [UI]::SendMessage($hwnd, 0x0100, [IntPtr]([int]$bits[2]), [IntPtr]::Zero) | Out-Null
                } elseif ($verb2 -eq "wheel") {
                    $wp = [IntPtr]((([int]$bits[3]) -shl 16) -bor ($lpx -band 0xFFFF))
                    [UI]::SendMessage($hwnd, 0x020A, $wp, ([IntPtr](($lpy -shl 16) -bor ($lpx -band 0xFFFF)))) | Out-Null
                }
                Start-Sleep -Milliseconds 150
            }
            "focus" { [UI]::SetForegroundWindow($hwnd) | Out-Null; Start-Sleep -Milliseconds 200 }
            default { Write-Output ("unknown op: " + $verb) }
        }
    }
}