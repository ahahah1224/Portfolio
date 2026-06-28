# OBJ Parser — Fast Wavefront .obj loader in C

A lightweight, dependency-free Wavefront `.obj` and `.mtl` parser written in pure C.
Built from scratch with a custom memory allocator, no standard library parsing functions used.

## Features

- Parses geometry: vertices (`v`), normals (`vn`), UV coordinates (`vt`)
- Loads material libraries (`.mtl`): colors, PBR values, texture maps with all flags
- Polygon triangulation (fan method) — handles quads and n-gons
- Custom memory allocator (`meta0`) — stores element count and byte size alongside each allocation
- No `pow()` calls in hot path — uses a lookup table for fast float parsing
- Single-header design — just `#include "parse_OBJ.h"`

## Performance

Parsed a model with **~2 million triangles** on a standard desktop CPU.
No heap allocations per face — temporary buffers are reused across the entire file.

## Usage

```c
#include "parse_OBJ.h"

int main() {
    // Read file into memory
    char *data = file_read("model.obj", NULL);

    // Parse
    Parse_OBJ model;
    Read_OBJ(data, &model);
    free_meta0(data);

    // Access geometry
    printf("Vertices:  %d\n", model.size_v);
    printf("UVs:       %d\n", model.size_uv);
    printf("Normals:   %d\n", model.size_n);
    printf("Materials: %d\n", model.size_mtl);

    // Access triangles per material
    for (int i = 0; i < model.size_mtl; i++) {
        if (!model.tringles[i]) continue;
        int count = sizeof_meta0(model.tringles[i])->count / 3;
        printf("Material '%s': %d triangles\n", model.material[i].name, count);
    }

    // Free
    free_Parse_OBJ(&model);
    return 0;
}
```

## Data Structures

```c
// A single vertex index triple (1-based, as in the .obj file)
typedef struct { uint32_t v, uv, n; } Vertex;

// The parsed model
typedef struct {
    int     size_v;   Vec4   *vectors;   // geometry vertices (x, y, z, w)
    int     size_uv;  Uv3    *uvs;       // texture coordinates
    int     size_n;   Normal *normals;   // vertex normals
    int     size_mtl; Mtl    *material;  // materials
    Vertex **tringles;                   // triangle index buffers, one per material
    char   *mtllib, *obj_name, *coments;
} Parse_OBJ;
```

## Files

| File | Description |
|------|-------------|
| `parse_OBJ.h` | Main parser — single header |
| `memory.h` | Custom allocator (`meta0`) with size metadata |

## Build

No build system required. Just compile your project with the headers in the include path:

```bash
gcc main.c -o main -I.
```

Tested with GCC on Windows.

## Notes

- Vertex indices are **1-based** (standard OBJ format). Subtract 1 before passing to a renderer.
- The parser does not normalize normals — use them as-is or normalize in your shader.
- Models with multiple objects (`o` keyword) are merged into one `Parse_OBJ`.

## Author

Low-level C developer. Interested in graphics, parsers, and systems programming.
Open to junior positions and freelance work.
