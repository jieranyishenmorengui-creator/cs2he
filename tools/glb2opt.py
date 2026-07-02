#!/usr/bin/env python3
"""Convert GLB (glTF Binary) → .opt (VisCheck format)"""

import struct
import json
import sys
import os

def read_glb(path):
    with open(path, 'rb') as f:
        # Header
        magic, version, length = struct.unpack('<I I I', f.read(12))
        assert magic == 0x46546C67, f"Not a GLB file (magic={magic:08x})"

        chunks = []
        while f.tell() < length:
            chunk_len, chunk_type = struct.unpack('<I I', f.read(8))
            data = f.read(chunk_len)
            chunks.append((chunk_type, data))

    return chunks

def extract_triangles_from_glb(glb_path):
    chunks = read_glb(glb_path)

    # Find JSON and BIN chunks
    json_data = None
    bin_data = None
    for chunk_type, data in chunks:
        if chunk_type == 0x4E4F534A:  # JSON
            json_data = json.loads(data.decode('utf-8'))
        elif chunk_type == 0x004E4942:  # BIN
            bin_data = data

    if not json_data or not bin_data:
        print("No JSON or BIN chunk found")
        return []

    meshes = []

    # Get min/max accessors for position
    for mesh_idx, mesh in enumerate(json_data.get('meshes', [])):
        for prim in mesh.get('primitives', []):
            if 'POSITION' not in prim.get('attributes', {}):
                continue

            pos_accessor_idx = prim['attributes']['POSITION']
            pos_accessor = json_data['accessors'][pos_accessor_idx]

            indices_accessor_idx = prim.get('indices')
            if indices_accessor_idx is None:
                continue
            idx_accessor = json_data['accessors'][indices_accessor_idx]

            buf_view_pos = json_data['bufferViews'][pos_accessor['bufferView']]
            buf_view_idx = json_data['bufferViews'][idx_accessor['bufferView']]

            # Read positions
            pos_offset = (buf_view_pos.get('byteOffset', 0) + pos_accessor.get('byteOffset', 0))
            pos_count = pos_accessor['count']
            pos_stride = buf_view_pos.get('byteStride', 12)  # 3 floats

            positions = []
            for i in range(pos_count):
                off = pos_offset + i * pos_stride
                x, y, z = struct.unpack_from('<fff', bin_data, off)
                positions.append((x, y, z))

            # Read indices
            idx_offset = (buf_view_idx.get('byteOffset', 0) + idx_accessor.get('byteOffset', 0))
            idx_count = idx_accessor['count']

            # Determine index format
            comp_type = idx_accessor.get('componentType')
            if comp_type == 5123:  # UNSIGNED_SHORT
                fmt = '<H'
                stride = 2
            elif comp_type == 5125:  # UNSIGNED_INT
                fmt = '<I'
                stride = 4
            else:
                print(f"  Unknown index type: {comp_type}, skipping")
                continue

            indices = []
            for i in range(idx_count):
                val = struct.unpack_from(fmt, bin_data, idx_offset + i * stride)[0]
                indices.append(val)

            # Build triangles (triplet)
            tris = []
            for i in range(0, len(indices), 3):
                if i + 2 < len(indices):
                    a, b, c = indices[i], indices[i+1], indices[i+2]
                    tris.append((positions[a], positions[b], positions[c]))

            if tris:
                meshes.append(tris)
                print(f"  Mesh {mesh_idx}: {len(tris)} triangles")

    return meshes

def write_opt(meshes, opt_path):
    with open(opt_path, 'wb') as f:
        f.write(struct.pack('<Q', len(meshes)))
        for tris in meshes:
            f.write(struct.pack('<Q', len(tris)))
            for v0, v1, v2 in tris:
                f.write(struct.pack('<fff', *v0))
                f.write(struct.pack('<fff', *v1))
                f.write(struct.pack('<fff', *v2))
    print(f"Wrote {opt_path} ({sum(len(t) for t in meshes)} total triangles)")

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.glb> <output.opt>")
        return

    in_path = sys.argv[1]
    out_path = sys.argv[2]

    print(f"Reading {in_path}...")
    meshes = extract_triangles_from_glb(in_path)

    if not meshes:
        print("No triangles found!")
        return

    write_opt(meshes, out_path)
    print("Done!")

if __name__ == '__main__':
    main()
