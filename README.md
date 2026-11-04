# img2tiles

**img2tiles** is a lightweight command-line utility written in C that converts image colors into rectangular tiles and exports their coordinates and colors to a JSON file.

It is designed to simplify the process of extracting colored regions from images, making them easier to use in 2D game development and other graphics-related projects.

## Features

- Extract pixels matching specified RGBA colors.
- Support for multiple target colors in a single run.
- Strict and tolerance-based color matching.
- Merge adjacent matching pixels into rectangular regions.
- Export extracted tiles to JSON.
- Lightweight implementation in C.
- Simple command-line interface.

## Requirements

- Clang (default compiler).
- GNU Make.
- GCC (optional alternative compiler).

### Dependencies

The project uses the following third-party libraries:

- [stb_image](https://github.com/nothings/stb) — image loading.
- [yyjson](https://github.com/ibireme/yyjson) — JSON generation and serialization.
- [ya_getopt](https://github.com/kubo/ya_getopt) — parsing arguments.

Both libraries are included in the project under `third_party/`.

## Installation

Clone the repository:

```bash
git clone https://github.com/cacao17168/img2tiles.git
cd img2tiles
```

Build the project:

```bash
make
```

By default, the project uses Clang.

If you prefer GCC, change the compiler specified in the Makefile from `clang` to `gcc`.

## Usage

```bash
./img2tiles --file <image> [OPTIONS] <color1> [color2 ...]
```

Colors must be specified in hexadecimal RGBA format (`RRGGBBAA`). The `#` prefix is optional.

### Examples

Extract pure black pixels:

```bash
./img2tiles -f default.png 000000FF
```

Extract multiple colors:

```bash
./img2tiles -f default.png 000000FF FF0000FF 00FF00FF
```

Enable tolerance-based color matching:

```bash
./img2tiles -f default.png --tolerance 2 000000FF
```

Enable verbose output:

```bash
./img2tiles -f default.png --verbose 000000FF
```

Run silently:

```bash
./img2tiles -f default.png --quiet 000000FF
```

## Command-line Options

| Option | Short | Description |
|---|---|---|
| `--file` | `-f` | Specify the input image. |
| `--help` | `-h` | Display the help message. |
| `--verbose` | `-v` | Enable debug information. |
| `--quiet` | `-q` | Suppress standard output and error output. |
| `--tolerance` | `-t` | Set the color matching tolerance. |

### Color Matching

img2tiles supports two color matching approaches:

**Strict matching**

When tolerance is set to `0`, pixels must match the target RGBA color exactly.

**Tolerance-based matching**

A positive tolerance allows small differences between pixel colors and the target color.

For example, with a target color of `000000FF` and tolerance of `2`:

| Pixel RGBA | Match |
|---|---|
| `(0, 0, 0, 255)` | Yes |
| `(1, 0, 0, 255)` | Yes |
| `(2, 0, 0, 253)` | Yes |
| `(3, 0, 0, 255)` | No |

Tolerance is applied independently to each RGBA channel.

## Output Format

The generated JSON file is saved alongside the input image, using the same filename with a `.json` extension.

Each extracted rectangle is represented as an object containing its position, dimensions, and color.

Example:

```json
[
    {
        "x": 0,
        "y": 0,
        "w": 4,
        "h": 2,
        "color": {
            "r": 0,
            "g": 0,
            "b": 0,
            "a": 255
        }
    }
]
```

### JSON Fields

| Field | Description |
|---|---|
| `x` | Horizontal position of the rectangle. |
| `y` | Vertical position of the rectangle. |
| `w` | Rectangle width in pixels. |
| `h` | Rectangle height in pixels. |
| `color.r` | Red channel (0–255). |
| `color.g` | Green channel (0–255). |
| `color.b` | Blue channel (0–255). |
| `color.a` | Alpha channel (0–255). |

Coordinates use the top-left corner of the image as the origin `(0, 0)`.

## License

This project is licensed under the [MIT License](LICENSE).
