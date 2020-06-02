package main

import "fmt"

func target(n uint32) uint32 {
	var mod uint32 = n % 4
	var result uint32 = 0

	if mod == 0 {
		result = (n | 0xbaaad0bf) * (2 ^ n)
	} else if mod == 1 {
		result = (n & 0xbaaad0bf) * (3 + n)
	} else if mod == 2 {
		result = (n ^ 0xbaaad0bf) * (4 | n)
	} else {
		result = (n + 0xbaaad0bf) * (5 & n)
	}

	return result
}

func main() {
	fmt.Printf("%d\n", target(10))
}
