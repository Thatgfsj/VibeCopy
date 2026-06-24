#!/bin/bash

# 配置文件名改为 copy.txt
FILE_NAME="copy.txt"

# 如果文件不存在，自动创建
if [ ! -f "$FILE_NAME" ]; then
    touch "$FILE_NAME"
fi

# 调用 Python 绘制原生 GUI 界面
python3 - "$FILE_NAME" << 'PYEOF'
import sys
import tkinter as tk
from tkinter import ttk
import subprocess
import os
import re

file_name = sys.argv[1]

root = tk.Tk()
root.title("VibeCopy 鼠标速复")
root.geometry("800x300")
root.attributes('-topmost', True)

# 主网格配置，允许缩放
root.grid_rowconfigure(0, weight=1)
root.grid_columnconfigure(0, weight=1)

# 创建带滚动的容器
main_frame = ttk.Frame(root)
main_frame.grid(row=0, column=0, sticky="nsew")

canvas = tk.Canvas(main_frame)
scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
scrollable_frame = ttk.Frame(canvas)

scrollable_frame.bind(
    "<Configure>",
    lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
)

canvas_window = canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
canvas.configure(yscrollcommand=scrollbar.set)

main_frame.pack(fill="both", expand=True)
canvas.pack(side="left", fill="both", expand=True)
scrollbar.pack(side="right", fill="y")

# 窗口拉伸时，让内部 Frame 跟着拉伸
def on_canvas_configure(event):
    canvas.itemconfig(canvas_window, width=event.width)
canvas.bind("<Configure>", on_canvas_configure)

# Linux 鼠标滚轮支持
canvas.bind_all("<Button-4>", lambda e: canvas.yview_scroll(-1, "units"))
canvas.bind_all("<Button-5>", lambda e: canvas.yview_scroll(1, "units"))

# 底部控件区
bottom_frame = ttk.Frame(root)
bottom_frame.grid(row=1, column=0, sticky="ew", padx=10, pady=10)
bottom_frame.grid_columnconfigure(0, weight=1)
bottom_frame.grid_columnconfigure(1, weight=1)
bottom_frame.grid_columnconfigure(2, weight=1)

btn_edit = ttk.Button(bottom_frame, text="编辑配置", command=lambda: subprocess.Popen(['xdg-open', file_name]))
btn_edit.grid(row=0, column=0, sticky="ew", padx=5)

btn_reload = ttk.Button(bottom_frame, text="刷新列表", command=lambda: load_snippets())
btn_reload.grid(row=0, column=1, sticky="ew", padx=5)

is_topmost = [True] # 用 list 包装以便内部函数修改
def toggle_topmost():
    is_topmost[0] = not is_topmost[0]
    root.attributes('-topmost', is_topmost[0])
    btn_top.config(text="取消置顶" if is_topmost[0] else "窗口置顶")

btn_top = ttk.Button(bottom_frame, text="取消置顶", command=toggle_topmost)
btn_top.grid(row=0, column=2, sticky="ew", padx=5)

status_label = tk.Label(root, text="准备就绪... 单击大色块即可复制", anchor="w", bd=1, relief="sunken")
status_label.grid(row=2, column=0, sticky="ew")

# 解析并生成大色块按钮
def load_snippets():
    for widget in scrollable_frame.winfo_children():
        widget.destroy()
        
    scrollable_frame.grid_columnconfigure(0, weight=1)
    scrollable_frame.grid_columnconfigure(1, weight=1)

    try:
        with open(file_name, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        status_label.config(text=f"读取错误: {e}")
        return

    index = 0
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        display = ""
        actual = ""
        
        if '==' in line:
            parts = line.split('==', 1)
            display = parts[0].strip()
            actual = parts[1].strip()
        else:
            m = re.match(r'^\s*[（(](.*?)[)）]\s*(.*)$', line)
            if m:
                display = m.group(1).strip()
                actual = m.group(2).strip()
            else:
                display = line
                actual = line
                
        # 闭包处理复制逻辑
        def make_copy_cmd(text):
            def cmd():
                root.clipboard_clear()
                root.clipboard_append(text)
                status_label.config(text=f"已复制: {text[:60]}")
            return cmd

        # 大色块按钮样式
        btn = tk.Button(scrollable_frame, text=display, font=("Sans", 14), height=3, 
                        relief="ridge", bg="#f0f0f0", activebackground="#d0d0d0",
                        command=make_copy_cmd(actual))
        
        col = index % 2
        r = index // 2
        btn.grid(row=r, column=col, sticky="nsew", padx=5, pady=5)
        index += 1
        
    status_label.config(text="加载完毕。鼠标单击大色块即可复制！")

load_snippets()
root.mainloop()
PYEOF
