# Dawn bindings for NodeJS

Note: This code is currently WIP. There are a number of [known issues](#known-issues).

## Building

### System requirements

- [CMake 3.10](https://cmake.org/download/) or greater
- [Go 1.13](https://golang.org/dl/) or greater

### Install `depot_tools`

Dawn uses the Chromium build system and dependency management so you need to [install depot_tools] and add it to the PATH.

[install depot_tools]: http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up

### Fetch dependencies

First, the steps are similar to [`docs/building.md`](../../../docs/building.md), but instead of the `Get the code` step, run:

```sh
# Clone the repo as "dawn"
git clone https://dawn.googlesource.com/dawn dawn && cd dawn

# Bootstrap the NodeJS binding gclient configuration
cp scripts/standalone-with-node.gclient .gclient

# Fetch external dependencies and toolchains with gclient
gclient sync
```

Optionally, on Linux install X11-xcb support:

```sh
sudo apt-get install libx11-xcb-dev
```

If you don't have those supporting libraries, then you must use the
`-DDAWN_USE_X11=OFF` flag on CMake (see below).

### Build

Currently, the node bindings can only be built with CMake:

```sh
mkdir <build-output-path>
cd <build-output-path>
cmake <dawn-root-path> -GNinja -DDAWN_BUILD_NODE_BINDINGS=1
ninja dawn.node
```

On Windows, the steps are similar:

```sh
mkdir <build-output-path>
cd <build-output-path>
cmake <dawn-root-path> -DDAWN_BUILD_NODE_BINDINGS=1
cmake --build . --target dawn_node
```

### Running WebGPU CTS

1. [Build](#build) the `dawn.node` NodeJS module.
2. Checkout the [WebGPU CTS repo](https://github.com/gpuweb/cts) or use the one in `third_party/webgpu-cts`.
3. Run `npm install` from inside the CTS directory to install its dependencies.

Now you can run CTS using our `./tools/run` shell script. On Windows, it's recommended to use MSYS2 (e.g. Git Bash):

```sh
./tools/run run-cts --bin=<path-build-dir> [WebGPU CTS query]
```

Where `<path-build-dir>` is the output directory. \
Note: `<path-build-dir>` can be omitted if your build directory sits at `<dawn>/out/active`, which is enforced if you use `<dawn>/tools/setup-build` (recommended).

Or if you checked out your own CTS repo:

```sh
./tools/run run-cts --bin=<path-build-dir> --cts=<path-to-cts> [WebGPU CTS query]
```

If this fails with the error message `TypeError: expander is not a function or its return value is not iterable`, try appending `--build=false` to the start of the `run-cts` command line flags.

To test against SwiftShader (software implementation of Vulkan) instead of the default Vulkan device, prefix `./tools/run run-cts` with `VK_ICD_FILENAMES=<swiftshader-cmake-build>/Linux/vk_swiftshader_icd.json`. For example:

```sh
VK_ICD_FILENAMES=<swiftshader-cmake-build>/Linux/vk_swiftshader_icd.json ./tools/run run-cts --bin=<path-build-dir> [WebGPU CTS query]
```

To test against Lavapipe (mesa's software implementation of Vulkan), similarly to SwiftShader, prefix `./tools/run run-cts` with `VK_ICD_FILENAMES=<lavapipe-install-dir>/share/vulkan/icd.d/lvp_icd.x86_64.json`. For example:

```sh
VK_ICD_FILENAMES=<lavapipe-install-dir>/share/vulkan/icd.d/lvp_icd.x86_64.json ./tools/run run-cts --bin=<path-build-dir> [WebGPU CTS query]
```

The `--flag` parameter must be passed in multiple times, once for each flag begin set. Here are some common arguments:

- `backend=<null|webgpu|d3d11|d3d12|metal|vulkan|opengl|opengles>`
- `adapter=<name-of-adapter>` - specifies the adapter to use. May be a substring of the full adapter name. Pass an invalid adapter name and `--verbose` to see all possible adapters.
- `dlldir=<path>` - used to add an extra DLL search path on Windows, primarily to load the right d3dcompiler_47.dll
- `enable-dawn-features=<features>` - enable [Dawn toggles](https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp), e.g. `dump_shaders`
- `disable-dawn-features=<features>` - disable [Dawn toggles](https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp)

For example, on Windows, to use the d3dcompiler_47.dll from a Chromium checkout, and to dump shader output, we could run the following using Git Bash:

```sh
./tools/run run-cts --verbose --bin=/c/src/dawn/out/active --cts=/c/src/webgpu-cts --flag=dlldir="C:\src\chromium\src\out\Release" --flag=enable-dawn-features=dump_shaders 'webgpu:shader,execution,builtin,abs:integer_builtin_functions,abs_unsigned:storageClass="storage";storageMode="read_write";containerType="vector";isAtomic=false;baseType="u32";type="vec2%3Cu32%3E"'
```

Note that we pass `--verbose` above so that all test output, including the dumped shader, is written to stdout.

### Testing against a `run-cts` expectations file

You can write out an expectations file with the `--output <path>` command line flag, and then compare this snapshot to a later run with `--expect <path>`.

## Viewing Dawn per-test coverage

### Requirements:

Dawn needs to be built with clang and the `DAWN_EMIT_COVERAGE` CMake flag.

LLVM is also required, either [built from source](https://github.com/llvm/llvm-project), or downloaded as part of an [LLVM release](https://releases.llvm.org/download.html). Make sure that the subdirectory `llvm/bin` is in your PATH, and that `llvm-cov` and `llvm-profdata` binaries are present.

Optionally, the `LLVM_SOURCE_DIR` CMake flag can also be specified to point the the `./llvm` directory of [an LLVM checkout](https://github.com/llvm/llvm-project), which will build [`turbo-cov`](../../../tools/src/cmd/turbo-cov/README.md) and dramatically speed up the processing of coverage data. If `turbo-cov` is not built, `llvm-cov` will be used instead.

It may be helpful to write a bash script like `use.sh` that sets up your build environment, for example:

```sh
#!/bin/bash
LLVM_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd )"
export CC=${LLVM_DIR}/bin/clang
export CXX=${LLVM_DIR}/bin/clang++
export MSAN_SYMBOLIZER_PATH=${LLVM_DIR}/bin/llvm-symbolizer
export PATH=${LLVM_DIR}/bin:${PATH}
```

Place this script in the LLVM root directory, then you can setup your build for coverage as follows:

```sh
. ~/bin/llvm-15/use.sh
cmake <dawn-root-path> -GNinja -DDAWN_BUILD_NODE_BINDINGS=1 -DDAWN_EMIT_COVERAGE=1
ninja dawn.node
```

Note that if you already generated the CMake build environment with a different compiler, you will need to delete CMakeCache.txt and generate again.

### Usage

Run `./tools/run run-cts` like before, but include the `--coverage` flag.
After running the tests, your browser will open with a coverage viewer.

Click a source file in the left hand panel, then click a green span in the file source to see the tests that exercised that code.

You can also highlight multiple lines to view all the tests that covered any of that highlighted source.

NOTE: if the left hand panel is empty, ensure that `dawn.node` was built with the same version of Clang that matches the version of LLVM being used to retrieve coverage info.

## Debugging TypeScript with VSCode

Open or create the `.vscode/launch.json` file, and add:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug with node",
      "type": "node",
      "request": "launch",
      "outFiles": ["./**/*.js"],
      "args": [
        "-e",
        "require('./src/common/tools/setup-ts-in-node.js');require('./src/common/runtime/cmdline.ts');",
        "--",
        "placeholder-arg",
        "--gpu-provider",
        "[path-to-cts.js]", // REPLACE: [path-to-cts.js]
        "[test-query]" // REPLACE: [test-query]
      ],
      "cwd": "[cts-root]" // REPLACE: [cts-root]
    }
  ]
}
```

Replacing:

- `[cts-root]` with the path to the CTS root directory. If you are editing the `.vscode/launch.json` from within the CTS workspace, then you may use `${workspaceFolder}`.
- `[cts.js]` this is the path to the `cts.js` file that should be copied to the output directory by the [build step](#build)
- `test-query` with the test query string. Example: `webgpu:shader,execution,builtin,abs:*`

## Debugging dawn-node issues in gdb/lldb

It is possible to run the CTS with dawn-node directly similarly to Debugging TypeScript with VSCode:

```sh
cd <cts-root-dir>
[path-to-node] \ # for example <dawn-root-dir>/third_party/node/<arch>/node
    -e "require('./src/common/tools/setup-ts-in-node.js');require('./src/common/runtime/cmdline.ts');" \
    -- \
    placeholder-arg \
    --gpu-provider [path to cts.js] \
    [test-query]
```

This command is then possible to run in your debugger of choice.

## Recipes for building software GPUs

### Building Lavapipe (LLVM Vulkan)

### System requirements

- Python 3.6 or newer
- [Meson](https://mesonbuild.com/Quick-guide.html)
- llvm-dev

These can be pre-built versions from apt-get, etc.

### Get source code

You can either download a specific version of mesa from [here](https://docs.mesa3d.org/download.html)

or use git to pull from the source tree ([details](https://docs.mesa3d.org/repository.html))

```sh
git clone https://gitlab.freedesktop.org/mesa/mesa.git
```

### Building

In the source directory

```sh
mkdir <build-dir>
meson setup <build-dir>/ -Dprefix=<lavapipe-install-dir> -Dvulkan-drivers=swrast
meson compile -C <build-dir>
meson install -C <build-dir>
```

This should result in Lavapipe being built and the artifacts copied to `<lavapipe-install-dir>`

Further details can be found [here](https://docs.mesa3d.org/install.html)

## Known issues

See https://bugs.chromium.org/p/dawn/issues/list?q=component%3ADawnNode&can=2 for tracked bugs, and `TODO`s in the code.

## Remaining work

- Investigate CTS failures that are not expected to fail.
- Generated includes live in `src/` for `dawn/node`, but outside for Dawn. [discussion](https://dawn-review.googlesource.com/c/dawn/+/64903/9/src/dawn/node/interop/CMakeLists.txt#56)
- Hook up to presubmit bots (CQ / Kokoro)
- `binding::GPU` will require significant rework [once Dawn implements the device / adapter creation path properly](https://dawn-review.googlesource.com/c/dawn/+/64916/4/src/dawn/node/binding/GPU.cpp).

