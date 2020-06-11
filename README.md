# hellscape

GIMPLE obfuscator for C, C++, Go, ... all supported GCC targets and front-ends that use GIMPLE. 

Inspired by the seminal paper by Pascal Junod and co.: [Obfuscator-LLVM -- Software Protection for the Masses](https://github.com/obfuscator-llvm/obfuscator).

SUPPORT WILL NOT BE PROVIDED ON THE ISSUE TRACKER (bugs are always welcome). PAID SUPPORT IS AVAILABLE.

You can also donate via BTC: `1M2RbhYU98fyi7fDFAKTSuv47ERGEJU5xN`.

##### Table of Contents
* [Installation](#installation)  
* ["Show me how to use it, dammit!"](#show-me-how-to-use-it-dammit)
  * [Substitution](#instruction-substitution)
  * [Bogus Control Flow](#bogus-control-flow)
  * [Flattening](#flattening)
  * [Maximum protections](#all-at-once)
* [Adding a custom pass](#adding-a-custom-pass)
* [License](#license)

|Original|Substitution|Bogus Control Flow|Flattening|
|:-:|:-:|:-:|:-:|
|<img src="https://i.imgur.com/NSOomo3.png" width="150">|<img src="https://i.imgur.com/Eb4jtyl.png" width="150">|<img src="https://imgur.com/b5M6Jcv.png" width="150">|<img src="https://i.imgur.com/wOKn2pd.png" width="300">|

### Installation

Currently building on Linux is supported, you may find some luck on macOS (PRs are welcomed). 

You need CMake, gcc (with plugins enabled) and (optionally) ninja.

GCC >= 9.3.0 is required, GCC 10.1.0 is tested and working.

```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

Then place `hellscape.so` in a known directory, like `~/bin/hellscape.so` and pass the correct path to GCC.

### "Show me how to use it, dammit!"

Throughout this walk-through, we'll use the following function:

```c
uint32_t target(uint32_t n) {
  uint32_t mod = n % 4;
  uint32_t result = 0;

  if (mod == 0) {
    result = (n | 0xBAAAD0BF) * (2 ^ n);
  } else if (mod == 1) {
    result = (n & 0xBAAAD0BF) * (3 + n);
  } else if (mod == 2) {
    result = (n ^ 0xBAAAD0BF) * (4 | n);
  } else {
    result = (n + 0xBAAAD0BF) * (5 & n);
  }

  return result;
}
```
(Adapted from [here](https://blog.quarkslab.com/deobfuscation-recovering-an-ollvm-protected-program.html).)

The CFG produced by this function (including GIMPLE IR) is as follows:

<p align="center"><img src="https://i.imgur.com/NSOomo3.png" height="450"></p>

The compiler plugin is easy to use; let's enable each pass one-by-one and look at the CFG, then at the end run all 3 passes together.

##### Instruction Substitution
For the first magic trick, instruction substitution. The command below,

* Sets the RNG seed to `0xdeadbeef` as to ensure binaries are reproducable. Outside of testing, you probably want to omit that flag to produce diverse binaries,
* Enables the subsitution pass (note you can enable "looping", i.e.: running the pass over itself multiple times with `-fplugin-arg-hellscape-subLoop=X`.)

```
$ gcc -fPIC -fplugin=/path/to/hellscape.so -fplugin-arg-hellscape-seed=deadbeef -fplugin-arg-hellscape-sub target.c
```

Let's view the produced CFG with the `Viz` class:

<p align="center"><img src="https://i.imgur.com/Eb4jtyl.png" height="450"></p>

##### Bogus Control Flow

Now for a smokescreen: bogus control flow. The command below,
* Sets the RNG seed (see above),
* Enables the bogus control flow pass which wraps every basic block in an opaque condition that always evaluates to `true`.

```
$ gcc -fPIC -fplugin=/path/to/hellscape.so -fplugin-arg-hellscape-seed=deadbeef -fplugin-arg-hellscape-bcf target.c
```

Again, viewing the CFG:

<p align="center"><img src="https://imgur.com/b5M6Jcv.png" height="450"></p>

##### Flattening

The last trick (for now) is flattening. The command below,

* Sets the RNG seed (see above),
* Enables the flattening pass.

```
$ gcc -fPIC -fplugin=/path/to/hellscape.so -fplugin-arg-hellscape-seed=deadbeef -fplugin-arg-hellscape-fla target.c
```

Then, viewing the CFG:

<p align="center"><img src="https://i.imgur.com/wOKn2pd.png"></p>

##### All at once

Simply rolling all the above commands together, we get the following CFG (view in a browser):

```
$ gcc -fPIC -fplugin=/path/to/hellscape.so -fplugin-arg-hellscape-seed=deadbeef -fplugin-arg-hellscape-fla -fplugin-arg-hellscape-bcf -fplugin-arg-hellscape-sub target.c
```

<p align="center"><img src="https://i.imgur.com/kastBEo.png"></p>

And of course in IDA it is even worse due to switch lowering:

<p align="center"><img src="https://i.imgur.com/tM1awwR.png" height="500"></p>

### Adding a custom pass

If you ever get stuck, reference one of the existing passes, they're well documented. That being said, the general idea is as follows:

1) Create a new pass class, e.g.: `ExamplePass`, under the file `EX.cpp` and `EX.h`,
2) Copy the contents of `SUB.h` and re-name the pass data, name of the pass as well as the constructor,
3) Create a function `execute` in the corresponding C++ file, and complete your pass,
4) Register it under the `PassManager` by adding a new option, `register_pass_info` and `register_callback`, as necessary,
5) Submit the pass for review.

Note that for as pass to be accepted upstream, it must compile for a `gcc` build that has `--enable-checking=yes,rtl,tree` added. (You will need to re-build `gcc` for your target architecture with `--enable-checking`.)

Additionally: If you experience crashes when developing your plug-in, you can debug them by passing `-wrapper gdb,--args` to `gcc`. (Run `gcc` in `gdb`, effectively.)

### License

Hellscape is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Hellscape is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

---

To be clear: this software is GPLv3 because it uses GCC headers which are licensed as GPLv3, I could have went with a license that is compatible with the GPLv3 but that would still be inconsequential as to the requirements of distributing this software.

This also means that due to the nature of the GCC plugin system, it is close to impossible to build proprietary GCC plugins; please keep this in mind when you re-distribute this software.

As always, speak with your lawyer if you have any questions. This is not legal advice.
