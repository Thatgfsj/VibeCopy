Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$form = New-Object System.Windows.Forms.Form
$form.Text = "VibeCopy 鼠标速复"
# 窗口改回 800 x 300
$form.ClientSize = New-Object System.Drawing.Size(800, 300)
$form.TopMost = $true
$form.StartPosition = "CenterScreen"
# 允许窗口自由缩放
$form.FormBorderStyle = "Sizable"
$form.MinimumSize = New-Object System.Drawing.Size(400, 200)
$form.Font = New-Object System.Drawing.Font("Segoe UI", 12)

# 列表区域：跟随窗口全方位缩放
$table = New-Object System.Windows.Forms.TableLayoutPanel
$table.Location = New-Object System.Drawing.Point(10, 10)
$table.Size = New-Object System.Drawing.Size(780, 175)
$table.ColumnCount = 2
$table.AutoScroll = $true
$table.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle([System.Windows.Forms.SizeType]::Percent, 50))) | Out-Null
$table.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle([System.Windows.Forms.SizeType]::Percent, 50))) | Out-Null
$table.Anchor = [System.Windows.Forms.AnchorStyles]::Top -bor [System.Windows.Forms.AnchorStyles]::Bottom -bor [System.Windows.Forms.AnchorStyles]::Left -bor [System.Windows.Forms.AnchorStyles]::Right

# 底部按钮：缩小尺寸并设置锚点
$btnEdit = New-Object System.Windows.Forms.Button
$btnEdit.Location = New-Object System.Drawing.Point(10, 195)
$btnEdit.Size = New-Object System.Drawing.Size(250, 50)
$btnEdit.Text = "编辑配置"
$btnEdit.Font = New-Object System.Drawing.Font("Segoe UI", 12)
$btnEdit.Anchor = [System.Windows.Forms.AnchorStyles]::Bottom -bor [System.Windows.Forms.AnchorStyles]::Left

$btnReload = New-Object System.Windows.Forms.Button
$btnReload.Location = New-Object System.Drawing.Point(275, 195)
$btnReload.Size = New-Object System.Drawing.Size(250, 50)
$btnReload.Text = "刷新列表"
$btnReload.Font = New-Object System.Drawing.Font("Segoe UI", 12)
$btnReload.Anchor = [System.Windows.Forms.AnchorStyles]::Bottom -bor [System.Windows.Forms.AnchorStyles]::Left -bor [System.Windows.Forms.AnchorStyles]::Right

$btnTop = New-Object System.Windows.Forms.Button
$btnTop.Location = New-Object System.Drawing.Point(540, 195)
$btnTop.Size = New-Object System.Drawing.Size(250, 50)
$btnTop.Text = "取消置顶"
$btnTop.Font = New-Object System.Drawing.Font("Segoe UI", 12)
$btnTop.Anchor = [System.Windows.Forms.AnchorStyles]::Bottom -bor [System.Windows.Forms.AnchorStyles]::Right

# 状态栏：跟随底部左右拉伸
$status = New-Object System.Windows.Forms.Label
$status.Location = New-Object System.Drawing.Point(10, 255)
$status.Size = New-Object System.Drawing.Size(780, 35)
$status.Text = "准备就绪... 单击大色块即可复制"
$status.Anchor = [System.Windows.Forms.AnchorStyles]::Bottom -bor [System.Windows.Forms.AnchorStyles]::Left -bor [System.Windows.Forms.AnchorStyles]::Right

$global:fileName = "复制配置文件.txt"

function Load-Snippets {
    $table.Controls.Clear()
    
    if (-not (Test-Path $global:fileName)) {
        "" | Out-File -FilePath $global:fileName -Encoding UTF8
        $status.Text = "已创建 复制配置文件.txt，请点击'编辑配置'添加内容。"
        return
    }

    $lines = Get-Content $global:fileName -Encoding UTF8
    $index = 0
    $row = 0
    
    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        
        $displayName = ""
        $actualText = ""
        
        if ($line -match "\=\=") {
            $parts = $line -split "\=\=", 2
            $displayName = $parts[0].Trim()
            $actualText = $parts[1]
        } 
        elseif ($line -match '^\s*[\(（](.*?)[\)）]\s*(.*)$') {
            $displayName = $matches[1].Trim()
            $actualText = $matches[2].Trim()
        } 
        else {
            $displayName = $line
            $actualText = $line
        }
        
        # 色块按钮高度设为 55，适应更紧凑的窗口
        $btn = New-Object System.Windows.Forms.Button
        $btn.Text = $displayName
        $btn.Height = 55
        $btn.Font = New-Object System.Drawing.Font("Segoe UI", 12)
        $btn.Tag = $actualText
        
        $btn.Add_Click({
            $textToCopy = $this.Tag
            [System.Windows.Forms.Clipboard]::SetText($textToCopy)
            $status.Text = "已复制: " + $textToCopy.Substring(0, [Math]::Min($textToCopy.Length, 60))
        })
        
        $col = $index % 2
        if ($index -gt 0 -and $col -eq 0) { $row++ }
        
        $table.Controls.Add($btn, $col, $row)
        $index++
    }
    $status.Text = "加载完毕。鼠标单击大色块即可复制！"
}

$btnEdit.Add_Click({
    Start-Process notepad.exe -ArgumentList $global:fileName
})

$btnReload.Add_Click({
    Load-Snippets
})

$btnTop.Add_Click({
    $form.TopMost = -not $form.TopMost
    if ($form.TopMost) { $btnTop.Text = "取消置顶" } else { $btnTop.Text = "窗口置顶" }
})

$form.Controls.Add($table)
$form.Controls.Add($btnEdit)
$form.Controls.Add($btnReload)
$form.Controls.Add($btnTop)
$form.Controls.Add($status)

Load-Snippets
$form.ShowDialog() | Out-Null
