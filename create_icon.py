from PIL import Image, ImageDraw, ImageFont
import os

# 创建图标
sizes = [16, 32, 48, 64, 128, 256]
images = []

for size in sizes:
    img = Image.new('RGBA', (size, size), (0, 120, 215, 255))  # 蓝色背景
    draw = ImageDraw.Draw(img)

    # 画一个复制图标（两个重叠的方块）
    margin = size // 6
    # 后面的方块（偏移）
    draw.rectangle(
        [margin + size//8, margin, size - margin, size - margin - size//8],
        fill=(255, 255, 255, 200),
        outline=(255, 255, 255, 255),
        width=max(1, size//32)
    )
    # 前面的方块
    draw.rectangle(
        [margin, margin + size//8, size - margin - size//8, size - margin],
        fill=(255, 255, 255, 255),
        outline=(255, 255, 255, 255),
        width=max(1, size//32)
    )

    images.append(img)

# 保存为 ICO
ico_path = os.path.join(os.path.dirname(__file__), 'vibe-copy.ico')
images[0].save(ico_path, format='ICO', sizes=[(s, s) for s in sizes], append_images=images[1:])
print(f"Icon saved to {ico_path}")
