# cuBQL OpenMP issue reproducers

This repository contains small, focused reproducers for issues encountered when
using cuBQL through its OpenMP GPU-offload pathway. Each reproducer is built as
a separate executable so that the issues can be compiled, run, and reported
independently.

## Reproducers

| Executable | Description |
| --- | --- |
| `dist_stack_fail` | Reproduces the traversal/intersection issue observed when `CUBQL_DIST_STACK` is enabled in [`rayQueries.h`](https://github.com/NVIDIA/cuBQL/blob/20f9db19cdb160e2b83d8f05dfdb0437fe11fe24/cuBQL/traversal/rayQueries.h#L11), as reported in [NVIDIA/cuBQL#43](https://github.com/NVIDIA/cuBQL/issues/43). |
| `bounds_extend_fail` | Reproduces an NVHPC OpenMP target failure while constructing a `cuBQL::box3d` and extending it with three `cuBQL::vec3d` vertices. On the affected setup, the optimized NVHPC build terminates with `CUDA_ERROR_LAUNCH_FAILED`, while the NVHPC Debug and LLVM builds complete successfully. |

The executable sources are in `reproducers/`, and headers shared between
reproducers are in `include/`.

## Configuring and compiling the repo

cuBQL is included as a Git submodule. After cloning the repository, initialise
the submodule with:

```bash
git submodule update --init --recursive
```

Once cloned, select the OpenMP target architecture at configuration time with
`CUBQL_OMP_GPU_ARCH`. Supply the NVIDIA compute capability without a decimal
point: for example, use `80` for compute capability 8.0 or `90` for compute
capability 9.0.

The default is `89`.

The value is translated into the corresponding compiler option:

- LLVM/Clang: `-march=sm_<architecture>`
- NVHPC: `-gpu=cc<architecture>`

Four CMake configure and build presets are provided:

| Preset | Compiler | Build type |
| --- | --- | --- |
| `llvm_release` | LLVM/Clang | Release |
| `llvm_debug` | LLVM/Clang | Debug |
| `nvhpc_release` | NVHPC | Release |
| `nvhpc_debug` | NVHPC | Debug |

For example, configure and build all reproducers with LLVM in Release mode:

```bash
cmake --preset llvm_release
cmake --build --preset llvm_release
```

Configure and build all reproducers with NVHPC in Release mode:

```bash
cmake --preset nvhpc_release
cmake --build --preset nvhpc_release
```

To select a different GPU architecture, pass it during configuration:

```bash
cmake --preset nvhpc_release -DCUBQL_OMP_GPU_ARCH=80
```

A single reproducer can be built by naming its target:

```bash
cmake --build --preset nvhpc_release --target bounds_extend_fail
```

## Run

Use mandatory target offloading so that a missing or unusable GPU image is
reported instead of silently falling back to the host:

```bash
OMP_TARGET_OFFLOAD=MANDATORY \
  ./build/llvm_release/dist_stack_fail

OMP_TARGET_OFFLOAD=MANDATORY \
  ./build/llvm_release/bounds_extend_fail
```

Replace `llvm_release` with the preset being tested, such as `nvhpc_release` or
`nvhpc_debug`.

## Expected behavior

### `dist_stack_fail`

The reproducer creates a hard-coded cube mesh, builds a BVH over it, and traces
a ray from inside the cube towards its positive-X face. The expected result is
an intersection with primitive 10 at a distance of 1:

```text
--------------------------------------
Hit primitive     : 10
Hit distance      : 1
Expected t        : 1
Expected hit prim : 10
Traversal         : PASS
--------------------------------------
```

The issue is reproduced when traversal incorrectly reports no intersection:

```text
--------------------------------------
Hit primitive     : -1
Hit distance      : inf
Expected t        : 1
Expected hit prim : 10
Traversal         : FAIL
--------------------------------------
```

### `bounds_extend_fail`

The reproducer constructs an empty `cuBQL::box3d` inside an OpenMP target
region and extends it with the three vertices of a triangle. A successful run
produces bounds from `(-1,-1,0)` to `(1,1,0)`:

```text
--------------------------------------
Computed lower   : (-1,-1,0)
Computed upper   : (1,1,0)
Expected lower   : (-1,-1,0)
Expected upper   : (1,1,0)
Bounds extension : PASS
--------------------------------------
```

On the affected setup, an NVHPC Release build instead terminates in the target
region with an error similar to:

```text
Accelerator Fatal Error: call to cuMemcpyDtoHAsync returned error 719
(CUDA_ERROR_LAUNCH_FAILED)
```

The NVHPC runtime terminates the process before the executable can print its
own `FAIL` summary. The runtime error and nonzero exit status are therefore the
failure signal for this reproducer. The corresponding NVHPC Debug build
(`-g -O0`) completes successfully, making this an optimization-sensitive
failure on the affected setup.

Both executables return exit status `0` when their computed result is correct
and `1` when they can report an incorrect result themselves.
