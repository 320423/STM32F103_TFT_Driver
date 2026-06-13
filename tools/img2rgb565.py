#!/usr/bin/env python3
"""
图片转 RGB565 C 数组  (零依赖, 仅使用 Python 标准库)

支持格式: BMP 24/32bit, PNG 8bit RGB/RGBA

用法:
  python img2rgb565.py <图片> [输出.h] [变量名] [选项]

选项:
  --resize W H     缩放到指定尺寸
  --fit W H        等比缩放适配到 WxH 以内 (如 --fit 240 320)
  --max-kb N       超过 N KB 时自动缩放到屏幕尺寸 (默认 500)

示例:
  python img2rgb565.py img.png img.h logo --fit 240 320
  python img2rgb565.py img.png img.h logo --resize 64 64
"""

import sys
import struct
import zlib
from pathlib import Path


# ═══════════════════════════ BMP 解码 ═══════════════════════════

def parse_bmp(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    if data[0:2] != b'BM':
        raise ValueError("not a BMP file")

    pixel_offset = struct.unpack_from('<I', data, 10)[0]
    width  = struct.unpack_from('<i', data, 18)[0]
    height = struct.unpack_from('<i', data, 22)[0]
    bits   = struct.unpack_from('<H', data, 28)[0]

    if bits not in (24, 32):
        raise ValueError(f"{bits}-bit BMP not supported, use 24/32 bit")

    row_bytes = (width * (bits // 8) + 3) & ~3
    pixels = []

    for y in range(abs(height) - 1, -1, -1):
        row_start = pixel_offset + y * row_bytes
        step = bits // 8
        for x in range(width):
            px = row_start + x * step
            b, g, r = data[px], data[px + 1], data[px + 2]
            pixels.append((r, g, b))

    return abs(width), abs(height), pixels


# ═══════════════════════════ PNG 解码 ═══════════════════════════

def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc: return a
    if pb <= pc: return b
    return c


def _unfilter_row(filter_type, cur, prev, bpp):
    if filter_type == 0:
        pass
    elif filter_type == 1:  # Sub
        for i in range(bpp, len(cur)):
            cur[i] = (cur[i] + cur[i - bpp]) & 0xFF
    elif filter_type == 2:  # Up
        if prev is not None:
            for i in range(len(cur)):
                cur[i] = (cur[i] + prev[i]) & 0xFF
    elif filter_type == 3:  # Average
        for i in range(len(cur)):
            a = cur[i - bpp] if i >= bpp else 0
            b = prev[i] if prev is not None else 0
            cur[i] = (cur[i] + ((a + b) >> 1)) & 0xFF
    elif filter_type == 4:  # Paeth
        for i in range(len(cur)):
            a = cur[i - bpp] if i >= bpp else 0
            b = prev[i] if prev is not None else 0
            c = prev[i - bpp] if prev is not None and i >= bpp else 0
            cur[i] = (cur[i] + _paeth(a, b, c)) & 0xFF
    return cur


def parse_png(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError("not a PNG file")

    width = height = bit_depth = color_type = 0
    compressed = b''
    pos = 8

    while pos < len(data):
        length = struct.unpack_from('>I', data, pos)[0]
        chunk_type = data[pos + 4: pos + 8].decode('ascii', errors='replace')
        chunk_data = data[pos + 8: pos + 8 + length]
        pos += 12 + length

        if chunk_type == 'IHDR':
            width = struct.unpack_from('>I', chunk_data, 0)[0]
            height = struct.unpack_from('>I', chunk_data, 4)[0]
            bit_depth = chunk_data[8]
            color_type = chunk_data[9]
            if bit_depth != 8 or color_type not in (2, 6):
                raise ValueError(
                    f"PNG must be 8-bit RGB/RGBA, got {bit_depth}-bit type={color_type}")
        elif chunk_type == 'IDAT':
            compressed += chunk_data
        elif chunk_type == 'IEND':
            break

    raw = zlib.decompress(compressed)
    bpp = 4 if color_type == 6 else 3
    stride = width * bpp + 1

    if len(raw) != stride * height:
        raise ValueError("PNG data size mismatch")

    pixels = []
    prev_row = None

    for y in range(height):
        row_start = y * stride
        filter_type = raw[row_start]
        cur_row = bytearray(raw[row_start + 1: row_start + stride])
        cur_row = _unfilter_row(filter_type, cur_row, prev_row, bpp)

        for x in range(width):
            px = x * bpp
            r, g, b = cur_row[px], cur_row[px + 1], cur_row[px + 2]
            pixels.append((r, g, b))

        prev_row = cur_row

    return width, height, pixels


# ═══════════════════════════ 格式检测 ═══════════════════════════

def parse_image(filepath):
    with open(filepath, 'rb') as f:
        header = f.read(8)

    if header[:2] == b'BM':
        return parse_bmp(filepath)
    elif header[:8] == b'\x89PNG\r\n\x1a\n':
        return parse_png(filepath)
    else:
        ext = Path(filepath).suffix.lower()
        raise ValueError(
            f"Unsupported format ({ext}). "
            f"Supported: BMP (24/32bit), PNG (8bit RGB/RGBA).")


# ═══════════════════════════ 缩放 ═══════════════════════════

def resize_pixels(pixels, src_w, src_h, dst_w, dst_h):
    """最近邻缩放"""
    result = []
    for y in range(dst_h):
        src_y = y * src_h // dst_h
        for x in range(dst_w):
            src_x = x * src_w // dst_w
            result.append(pixels[src_y * src_w + src_x])
    return result


# ═══════════════════════════ 主逻辑 ═══════════════════════════

def rgb888_to_rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def parse_args():
    args = {'resize': None, 'max_kb': 500}
    positional = []
    i = 1
    while i < len(sys.argv):
        a = sys.argv[i]
        if a == '--resize' and i + 2 < len(sys.argv):
            args['resize'] = (int(sys.argv[i + 1]), int(sys.argv[i + 2]))
            i += 3
        elif a == '--fit' and i + 2 < len(sys.argv):
            args['fit'] = (int(sys.argv[i + 1]), int(sys.argv[i + 2]))
            i += 3
        elif a == '--max-kb' and i + 1 < len(sys.argv):
            args['max_kb'] = int(sys.argv[i + 1])
            i += 2
        else:
            positional.append(a)
            i += 1
    return args, positional


def main():
    args, pos = parse_args()

    if len(pos) < 1:
        print(__doc__)
        sys.exit(1)

    img_path = pos[0]
    out_path = pos[1] if len(pos) > 1 else None
    var_name = pos[2] if len(pos) > 2 else "image_data"

    w, h, pixels = parse_image(img_path)
    orig_w, orig_h = w, h

    # 处理缩放
    if args.get('resize'):
        w, h = args['resize']
        print(f"Resize: {orig_w}x{orig_h} -> {w}x{h}")
        pixels = resize_pixels(pixels, orig_w, orig_h, w, h)
    elif args.get('fit'):
        fw, fh = args['fit']
        ratio = min(fw / orig_w, fh / orig_h, 1.0)
        w, h = int(orig_w * ratio), int(orig_h * ratio)
        if ratio < 1.0:
            print(f"Fit: {orig_w}x{orig_h} -> {w}x{h} (ratio={ratio:.2f})")
            pixels = resize_pixels(pixels, orig_w, orig_h, w, h)
    elif (orig_w > 240 or orig_h > 320) and (orig_w * orig_h * 2 > args['max_kb'] * 1024):
        # 自动缩放到屏幕
        ratio = min(240 / orig_w, 320 / orig_h)
        w, h = int(orig_w * ratio), int(orig_h * ratio)
        print(f"Auto-fit: {orig_w}x{orig_h} -> {w}x{h} (TFT screen)")
        pixels = resize_pixels(pixels, orig_w, orig_h, w, h)

    data_bytes = w * h * 2
    if data_bytes > 510 * 1024:
        print(f"[WARN] {data_bytes/1024:.0f} KB may exceed F103 512KB Flash!")

    # RGB888 → RGB565
    rgb565 = [rgb888_to_rgb565(r, g, b) for r, g, b in pixels]

    # 生成 C 数组
    name = Path(img_path).name
    lines = [
        "#include <stdint.h>",
        "",
        "/**",
        f" * Image: {name}",
        f" * Size: {w} x {h}  RGB565",
        f" * Data: {len(rgb565)} pixels, {len(rgb565) * 2} bytes",
        " */",
        f"const uint16_t {var_name}[{len(rgb565)}] = {{",
    ]

    per_line = 16
    for i in range(0, len(rgb565), per_line):
        row = rgb565[i: i + per_line]
        hex_str = ", ".join(f"0x{p:04X}" for p in row)
        lines.append(f"    {hex_str},")

    lines.append("};")
    result = "\n".join(lines)

    if out_path:
        Path(out_path).write_text(result, encoding="utf-8")
        kb = len(rgb565) * 2 / 1024
        print(f"[OK] {out_path}")
        print(f"     Size: {w}x{h}  {len(rgb565)*2} bytes ({kb:.1f} KB)")
        print(f"     Usage: #include \"{Path(out_path).name}\"")
        print(f"            TFT_DrawImage(x, y, {w}, {h}, {var_name});")
    else:
        print(result)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
