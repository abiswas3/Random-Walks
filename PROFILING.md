# compiling so that you can find the pieces of code you are interested in

```
$ cat ~/.cargo/config
[target.x86_64-unknown-linux-gnu]
rustflags = [ "-C", "link-args=-Wl,-export-dynamic" ]
```

this will make it so that the rustc linker includes all the symbols in the binary

```
$ cat Cargo.toml
...
[profile.release-with-debug]
inherits = "release"
debug = true
```

this makes it so that we can build with debug information, but still in release-mode: 

```
$ cargo build --release-with-debug
```

Now we want to find in our built binary the symbols for the functions we care about: 

{{< remark>}}
The nm command in Linux is used to inspect symbol tables of compiled binaries (e.g., .o, .a, or ELF binaries like executables and shared libraries).
Symbols are things like:

Function names

Global variables

Static variables

Debug info

They help during linking and debugging.
{{</ remark >}}

```
00000000 T main
00000010 T my_function
00000000 B global_variable

```

{{< remark>}}
T = the symbol is in the text (code) section (i.e., a function)

B = in BSS section (uninitialized global)

D = in data section (initialized global)

U = undefined (i.e., needs to be resolved during linking)

{{</ remark >}}
```
$ nm --defined-only target/release-with-debug/minimal_mult|grep scalar
000000000004b7d0 t _ZN12minimal_mult10yuval_mult10scalar_mul17ha435832ac9fe806eE
000000000004a790 t _ZN12minimal_mult12vanilla_cios10scalar_mul17h8731c86ebf619ce7E
000000000004b3a0 t _ZN12minimal_mult14optimised_cios20scalar_mul_unwrapped17h08a3ba9b5ffe6264E
```

Now let's run the binary, dumping all the instructions that are being executed along the way:

```
$ ../t/pin-external-3.31-98869-gfa6f126a8-gcc-linux/pin -t ../t/pin-external-3.31-98869-gfa6f126a8-gcc-linux/source/tools/ManualExamples/obj-intel64/itrace.so -- ./target/release-with-debug/minimal_mult
```

This will generate an itrace.out file that looks like:

```

Loading /root/Montgomery-Benchmarks/target/release-with-debug/minimal_mult 5e4052170000
Loading /lib64/ld-linux-x86-64.so.2 793baee95000
Loading [vdso] 793baee93000
0x793baeeb5940 mov rdi, rsp
0x793baeeb5943 call 0x793baeeb6620
0x793baeeb6620 nop edx, edi
0x793baeeb6624 push rbp
0x793baeeb6625 lea rcx, ptr [rip-0x2162c]
0x793baeeb662c lea rax, ptr [rip+0x19cad]
0x793baeeb6633 movq xmm2, rcx
0x793baeeb6638 movq xmm3, rax
0x793baeeb663d punpcklqdq xmm2, xmm3
0x793baeeb6641 mov rbp, rsp

...
```

Thus the yuval::scalar_mul routine is at 0x5e4052170000+0x4b7d0 (the address where minimal_mult was loaded, as per itrace.out, plus the offset where yuval_mult::scalar_mul is contained within minimal_mult, as per nm)

Likewise, the opt::scalar_mul routine is contained at 0x5e4052170000+0x4b3a0

So if we want to dump a copy of a run of the yuval scalar_mul, we simply print from 0x5e4052170000+0x4b7d0=0x5e40521bb7d0 to the corresponding ret instruction: 

```
cat itrace.out | sed -n '/^0x5e40521bb7d0/,/ret/{p;/ret/q}' > yuval.disas
```

and similarly 

```
cat itrace.out | sed -n '/^0x5e40521bb3a0/,/ret/{p;/ret/q}' > opt.disas
```

(N.B. this sed script is a little bit facile, and works here because the functions of interest do not call anything else. The real version would be to do

```
$ objdump -d -Mintel target/release-with-debug/minimal_mult|sed -n '/^0.*scalar_mul/,/ret/{/scalar_mul\|ret/p}'
000000000004a790 <_ZN12minimal_mult12vanilla_cios10scalar_mul17h8731c86ebf619ce7E>:
   4ac47:       c3                      ret
000000000004b3a0 <_ZN12minimal_mult14optimised_cios20scalar_mul_unwrapped17h08a3ba9b5ffe6264E>:
   4b7c6:       c3                      ret
000000000004b7d0 <_ZN12minimal_mult10yuval_mult10scalar_mul17ha435832ac9fe806eE>:
   4bce6:       c3                      ret
```

and then e.g. for yuval, 

```
cat itrace.out | sed -n '/^0x5e40521bb7d0/,/0x5e40521bbce6/{p;/0x5e40521bbce6/q}'
```

that is, using the address of the ret to figure out when to stop printing, and not just searching for the first ret, as this would be the return from the first callee of the function of interest and not from that functino itself)


Now these files tell the story of what happened. For example, see what memory operations are involved:

```
$ cat yuval.disas|grep -o '[a-z0-9]\+ ptr' | sort|uniq -c
      2 byte ptr
     74 qword ptr
      6 xmmword ptr
$ cat opt.disas|grep -o '[a-z0-9]\+ ptr' | sort|uniq -c
     36 qword ptr
```

So we see that yuval does over twice the memory accesses. 

As far as opcodes:

```
$ cat yuval.disas |awk '{print $2}'|sort|uniq -c |sort -n
      1 imul
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
     32 mul
     37 add
     56 adc
    152 mov
$ cat opt.disas |awk '{print $2}'|sort|uniq -c |sort -n
      1 ret
      1 setb
      3 lea
      4 imul
      6 pop
      6 push
     32 mul
     40 add
     52 adc
    135 mov
```

So opt does 36 multiplications (mul+imul) and 92 add/subs, while yuval does 33 and 101 respectively.

So indeed we have accomplished fewer multiplications at the cost of more additive operations, but we pay the price, by virtue of data organisation, in expensive memory accesses
