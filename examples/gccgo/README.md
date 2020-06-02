gccgo
=====

This example explains building a Go executable obfuscated with Hellscape.

For this guide, you will need `gccgo` installed. The standard Go executable will
NOT work with this example.

In the `CMakeLists.txt`, edit the `execute_process` command to use the `gccgo`
executable instead of the standard `gcc` executable. Then, build Hellscape as
normal.

It can then be used with the `main.go` provided here with the example command (to enable all passes):

```sh
$ gccgo -fplugin=/path/to/hellscape.so -fplugin-arg-hellscape-seed=deadbeef -fplugin-arg-hellscape-fla -fplugin-arg-hellscape-bcf -fplugin-arg-hellscape-sub main.go
```
