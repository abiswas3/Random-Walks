+++
date = '2025-05-15T17:52:40+01:00'
draft = false
title = "Montgomery Multiplication: Theory Vs Realtity" 
tags = ["cryptography", "benchmarking"]
categories = ["Software"]
authors = ["Ari"]
toc = false
type= "posts"  
+++

Recently, in [a blog post](https://hackmd.io/@Ingonyama/Barret-Montgomery) Yuval Domb describes a subtle, but remarkably clever optimisation that allows us to compute the Montgomery product of two $n$ word[^z] integers using $2n^2 + 1$ multiplication operations (as opposed to the previously best known $2n^2 + n$).
Although an improvement in theory, empirically the new algorithm appears to be slower than its suboptimal predecessor. 
This post is a deep dive into why this could be, and potentially what can be done about it.
Before diving into the code and our analysis, we briefly review Montgomery Multiplication.

[^z]: Word, digit, limbs are all interchangeable. For everything we discuss here, a word is 64-bits. So a 2-limb integer is just an integer that takes 2$\times$ 64 bits to describe.


## Montgomery Multiplication - Review

Given $n$ limb modulus $p$, and integers $a$ and $b$, the modular multiplication problem is to compute the remainder of the product

$$ ab \mod p$$

Reducing an integer with respect to a modulus, is computationally as expensive as performing the [division operation](https://en.algorithmica.org/hpc/arithmetic/division/), which is known to be much slower than other operations such as multiplication or addition[^e].
Furthermore, modern cryptographic applications often require chaining many modular multiplications together.
If we used long division for every individual multiplication, this would be prohibitively slow.
Instead, we use [Montgomery multiplication](https://en.wikipedia.org/wiki/Montgomery_modular_multiplication).

In Montgomery multiplication, instead of computing $ab \mod p$, we compute $$abR^{-1} \mod p$$where $R$ is set to the smallest power of two exceeding $p$ that falls on a computer word boundary.
For example, if $p$ is 381 bit integer, then $R= 2^{n \times w}=2^{6\times 64} = R^{384}$ on a 64-bit architecture.
Abstracting over some details, the reason for doing so is that the above computation does not use the expensive division algorithm.
As $R$ is a power of two, we can divide by $R$ by using the bit shift operation which is native to most Instruction Set Architectures.
This leads to more efficient code than implementing long division.
For the interested reader, [Montgomery Arithmetic from a Software Perspective
](https://eprint.iacr.org/2017/1057) is a good resource for understanding the theory of Montgomery Multiplication.


[^e]: This is usually because most CPU's do not directly offer a modulo instruction natively.

[^a]: The terms words and digits are sometimes used interchangeably. A word in this post will always be 64 bits of memory.

At a high level every Montgomery multiplication uses 3 kinds of operations[^aa] (see [Page 34 Table 2.1](/papers/Acar.pdf#page=34) for an overview of the exact counts):

+ Addition of two 64 bit integers with and without carry.
+ Multiplication of two 64 bit integers with and without carry.
+ Read and Writes of 64 and 128 bit values to memory.

[^aa]: These are the operations that the algorithms need. When running code on an actual CPU, there are other instructional overheads -- OS code, memory freeing etc.

More complex operations just use the above instructions as building blocks.

{{< remark >}}
A critical factor for understanding performance is that these operations not equal.
For example, on Skylake (mid-level x86 processor)

+ add has a latency of .25 (i.e. on average, with 1 core, you can retire an add and three other equivalent-cost instructions in one cycle)

+ moving a 64-bit register into memory is .5, and reading 64 bits from memory is latency of 1

+ 64-bit multiplication is latency of 3 (so 8 times as long as the add!)

[See Page 278](https://www.agner.org/optimize/instruction_tables.pdf) for more details on what instructions take how long.
{{< /remark >}}


Now given the above constraints, one would imagine that minimising the number of multiplication operations would the answer to faster algorithms.
Previously, the best known algorithm for performing a single Montgomery multiplication operation required $2n^2 + n$ multiplication operations.

{{< remark >}}
Surprisingly, this was not what we saw in actual experiments. 
The remainder of this document tries to understand why?
{{</ remark>}}



## Benchmarking Details 

We first analyse the performance, and subsequently in a later section, we provide the instructions for re-creating the workflow.

### Empirical Performance

We compare 2 algorithms to the new algorithm. 
The SOS algorithm in [Acar's theis](papers/Acar.pdf) which we refer to as C-mult.
We refr by G-mult to an optimisation of the CIOS algorithm by the [Gnark](https://hackmd.io/@gnark/modular_multiplication) team. 
Both these algorithms are already implemented in [Arkworks](https://github.com/a16z/arkworks-algebra).

[^y]: The only change we made was we [unrolled](https://github.com/abiswas3/Montgomery-Benchmarks/blob/0263c89bce53ea62ff7d9cba143020443b763659/src/yuval_mult.rs#L34) for ```addv``` helper method for $n=4$.
This only sped things up.

We found an implementation[^y] of the new multiplication algorithm [here](https://github.com/worldfnd/ProveKit/blob/dd8134ec3f2ad4991caa87653254ee64daf2d441/block-multiplier/src/lib.rs#L121) by the folks at [Ingoyama](https://www.ingonyama.com/), specifically targetting the [BN-254](https://neuromancer.sk/std/bn/bn254) curve (i.e $n=4$), which we refer to as Y-mult.
As stated earlier, Arkworks already had an implementation of the CIOS algorithm written [here](https://github.com/a16z/arkworks-algebra/blob/7ad88c46e859a94ab8e0b19fd8a217c3dc472f1c/ff-macros/src/montgomery/mul.rs#L11) and the SOS[^g] algorithm [here](https://github.com/a16z/arkworks-algebra/blob/7ad88c46e859a94ab8e0b19fd8a217c3dc472f1c/ff-macros/src/montgomery/mul.rs#L77).
To simplify the investigation process, we stripped the code down to the essential helper functions and the ```scalar_mul``` function. 
Then we wrote a [simple script](https://github.com/abiswas3/Montgomery-Benchmarks/blob/barebones_profiling/src/main.rs) to compare the performance of the two algorithms to see the speedup promised in theory manifested itself in practice.
The stripped down code needed to compute the montgomery product with Yuval's algorithm can be found [here](https://github.com/abiswas3/Montgomery-Benchmarks/blob/barebones_profiling/src/yuval_mult.rs).
Similarly, the stripped down code needed to compute the montgomery product with CIOS can be found [here](https://github.com/abiswas3/Montgomery-Benchmarks/blob/barebones_profiling/src/optimised_cios.rs).

[^g]: The comments claim that this is CIOS, but if you refer to the algorithms in Acar's thesis, this is defined as SOS.

{{< remark >}}
This entire writeup focuses on $n=4$. 
It is entirely possible that for larger values of $N$, the miscellaneous overheads we are about to discuss below will be outdone by the gains from fewer multiplications. 
{{< /remark >}}

```
git clone https://github.com/abiswas3/Montgomery-Benchmarks.git
git checkout -b barebones_profiling
git pull origin barebones_profiling
cargo run --release
```
Cargo run release should produce a `benchmark_data.csv` file for which each line is a trial run of how long a single multiplication took in nano seconds.
Feel free to use whatever plotting software you use.

**Plot new one**
![Benchmark result](/images/mont_mult/bench.jpeg)

### Computer Information

All computations were run on Intel CPUs.
Admittedly, this computer would not rank amongst the top of the line fully speced out servers availale today.
However, for multiplying numbers, this was sufficient. 
We did also run the code on a M2 macbook air with 8GB of ram. 
The relative trend between the algorithms were found to be the same.

```
lscpu
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          40 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   1
  On-line CPU(s) list:    0
Vendor ID:                AuthenticAMD
  BIOS Vendor ID:         QEMU
  Model name:             DO-Premium-AMD
    BIOS Model name:      pc-i440fx-6.1  CPU @ 2.0GHz
    BIOS CPU family:      1
    CPU family:           23
    Model:                49
    Thread(s) per core:   1
    Core(s) per socket:   1
    Socket(s):            1
    Stepping:             0
    BogoMIPS:             3992.49
    Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mt
                          rr pge mca cmov pat pse36 clflush mmx fxsr sse
                           sse2 syscall nx mmxext fxsr_opt pdpe1gb rdtsc
                          p lm rep_good nopl xtopology cpuid extd_apicid
                           tsc_known_freq pni pclmulqdq ssse3 fma cx16 s
                          se4_1 sse4_2 x2apic movbe popcnt aes xsave avx
                           f16c rdrand hypervisor lahf_lm svm cr8_legacy
                           abm sse4a misalignsse 3dnowprefetch osvw topo
                          ext perfctr_core ibpb stibp vmmcall fsgsbase b
                          mi1 avx2 smep bmi2 rdseed adx smap clflushopt
                          clwb sha_ni xsaveopt xsavec xgetbv1 clzero xsa
                          veerptr wbnoinvd arat npt nrip_save umip rdpid
Virtualization features:
  Virtualization:         AMD-V
  Hypervisor vendor:      KVM
  Virtualization type:    full
Caches (sum of all):
  L1d:                    32 KiB (1 instance)
  L1i:                    32 KiB (1 instance)
```

```
free -h
               total        used        free      shared  buff/cache   available
Mem:           1.9Gi       421Mi       359Mi       5.5Mi       1.3Gi       1.5Gi
Swap:             0B          0B          0B
```

## Analysis

To understand why these algorithms perform the way they do, we look at the x-86 assembly instruction that were executed for each of these algorithms.
Shown below is the exact x86 assembly instructions that were exectured when running the two new multiplication algorihtms (we ignore SOS as CIOS was known to be faster).

+ [y-mult](/code/yuval) (Short for Yuval's multiplication algorithm.)
+ [g-mult](/code/opt) (This is CIOS from Acar's thesis with the GNARK optimisation)

These files **only** contain the assembly instructions that were executed from when the function ```scalar_mul``` was called, to when the functions returned i.e. these are the exact instructions for Montgomery Multiplications.
Instructions that were overhead of our benchmarking code, or the operating system have been filtered out.

[Here]() is a dump of entire (3 million odd) set of x86 instruction that are executed, when we type ```cargo run``` into our terminal.
We describe later how to extract the methods we care about from this giant mess of code.
For now you can safely trust that they in the files described above.
The format in which the code is expressed is 

```
memory address, instruction
```

Where memory address refers to the virutal memory address at which the instruction is stored for the process executing the program.
Summarising our findings. The arkworks code base executes fewer x86 instructions; 280 vs 326 to specific.

```
wc -l static/code/yuval                                                                     ─╯
     326 static/code/yuval

wc -l static/code/opt                                                                       ─╯
     280 static/code/opt

```


For `g-mult`, as expected we see $2n^2 + n = 36$ multiplication operations.

```
1 ret
1 setb
3 lea
4 imul <--
6 pop
6 push
32 mul <--
40 add
52 adc
135 mov
```

Further more, there are 36 64-bit read/write operations to memory.

```
root@ubuntu-s-1vcpu-2gb-amd-nyc3-01:~/Montgomery-Benchmarks# cat opt|grep -o '[a-z0-9]\+ ptr' | sort|uniq -c
     36 qword ptr

```

For the new algorithm we expect to see fewer multiplications i.e $2n^2 + 1 = 33$ multiplication opearations, and that's exactly what we see.
This is what we hoped would give us our speedup. 
```
1 imul <---
1 ret
1 shl
1 shr
1 xorps
2 movups
2 sar
3 lea
3 movzx
3 sbb
4 movaps
4 xor
5 sub
6 pop
6 push
6 setb
32 mul <---
37 add
56 adc
152 mov
```
But the overhead from other instructions, especially read/writes to memory seems far worse. 
We make 74 64-bit read/write operations and 6 128-bit/read write operations.

```
root@ubuntu-s-1vcpu-2gb-amd-nyc3-01:~/Montgomery-Benchmarks# cat yuval|grep -o '[a-z0-9]\+ ptr' | sort|uniq -c
      2 byte ptr
     74 qword ptr
      6 xmmword ptr
```

{{< remark >}}

So it seems that the gains from having fewer multiplications is being drowned out by the heavier read/write operations, and the greater number of assembly instructions.
Note that this does not imply that the new algorithm could not be faster for even $n=4$.
It just means that the way the Rust compiler is building the current binary is not as efficient.

{{</ remark >}}

At this point we have established why we think we do not see an improvement in runtime.
The remainder of the post is written as tutorial on how we were able to perform the above analysis.

## How To Replicate The Process

