$ErrorActionPreference = "Continue"
Remove-Item "$env:TEMP\dsh_gh_token.txt" -Force -ErrorAction SilentlyContinue
Remove-Item "D:\SearchForPE\tools\_gh*.ps1","D:\SearchForPE\tools\_credprobe.ps1","D:\SearchForPE\tools\_release_notes.md","D:\SearchForPE\tools\_assetname.txt" -Force -ErrorAction SilentlyContinue
git -C D:\SearchForPE add -A
git -C D:\SearchForPE -c user.name=dsh -c user.email=dsh@local commit -m "docs: 添加 Release 徽章与下载指引" 2>&1 | Select-Object -First 3
$token = [Cred]::Read("git:https://github.com")
