# cubql-openmp-dist-stack-issue-reproducer

A small reproducer for the OpenMP traversal/intersection issue observed when
`CUBQL_DIST_STACK` is enabled in
[`rayQueries.h`](https://github.com/NVIDIA/cuBQL/blob/20f9db19cdb160e2b83d8f05dfdb0437fe11fe24/cuBQL/traversal/rayQueries.h#L11),
as reported in [NVIDIA/cuBQL#43](https://github.com/NVIDIA/cuBQL/issues/43).

The repository contains a minimal executable which creates a hard-coded cube
mesh made up of two triangles per face, builds a BVH over that mesh, and then
traverses that BVH using the `ShrinkingRayTraversal` template.

## Get the source

cuBQL is included as a Git submodule so ensure that this repo is cloned with submodules initialised:

```bash
git submodule update --init --recursive
```

## GPU architecture

The OpenMP target architecture is selected at configuration time with
`CUBQL_OMP_GPU_ARCH`. Supply the NVIDIA compute capability without a decimal
point: for example, use `80` for compute capability 8.0 or `90` for compute
capability 9.0.

The default is `89` (since my laptop has an RTX 2000Ada).

The selected value is translated into the appropriate compiler option:

- LLVM/Clang: `-march=sm_<architecture>`
- NVHPC: `-gpu=cc<architecture>`

## Configure and build

Four CMake configure and build presets are provided:

| Preset | Compiler | Build type |
| --- | --- | --- |
| `llvm_release` | LLVM/Clang | Release |
| `llvm_debug` | LLVM/Clang | Debug |
| `nvhpc_release` | NVHPC | Release |
| `nvhpc_debug` | NVHPC | Debug |

For example, configure and build an LLVM Release executable for compute
capability 8.0:

```bash
cmake --preset llvm_release -DCUBQL_OMP_GPU_ARCH=80
cmake --build --preset llvm_release
```

Configure and build an NVHPC Debug executable for compute capability 9.0:

```bash
cmake --preset nvhpc_debug -DCUBQL_OMP_GPU_ARCH=90
cmake --build --preset nvhpc_debug
```

## Run

Run the executable with mandatory target offloading so that OpenMP reports an
error instead of falling back to host execution if the GPU image cannot be
used:

```bash
OMP_TARGET_OFFLOAD=MANDATORY \
  ./build/llvm_release/cubql-dist-stack-reproducer
```

Use the corresponding build directory when testing another preset, for
example:

```bash
OMP_TARGET_OFFLOAD=MANDATORY \
  ./build/nvhpc_release/cubql-dist-stack-reproducer
```

## Expected output

The ray starts at the centre of the cube and travels towards its positive-X
face. It should intersect primitive 10 at a distance of 1. So a successful hit
would be:

```text
--------------------------------------
Hit primitive     : 10
Hit distance      : 1
Expected t        : 1
Expected hit prim : 10
Traversal         : PASS
--------------------------------------
```

The executable returns exit status `0` for this result.

The issue is reproduced when the traversal incorrectly reports no
intersection:

```text
--------------------------------------
Hit primitive     : -1
Hit distance      : inf
Expected t        : 1
Expected hit prim : 10
Traversal         : FAIL
--------------------------------------
```

The executable returns exit status `1` for this result. 