Target audience
======================

This repository is built for internal training use, demonstrating the design and implementation of a simple but feasible state automata for encoding and decoding "application layer messages" based on "transport layer TCP packet/bytestream".

The terms "transport layer" and "TCP" conform to the ["Internet Protocol Suite" defined in RFC1122](https://tools.ietf.org/html/rfc1122).


# Paths

You're reading `<proj-root>/README.md`.


# Version specification

Tested on Ubuntu14.04 with "GNU gcc version 5.4.0". This repo is `Linux specific`.


# Multi-threading concerns for target audience

If your design allows multiple threads to access I/O of a same "sockfd", remember to guard it by proper locking. 

The best assumption about the "intrinsic thread-safety of functions `send/recv` or `sendto/recvfrom`" is at the scope of no larger than "transport layer packet", which is

- sufficient to guarantee correct fragmentation of "application layer messages" for UDP, but
- insufficient to guarantee correct fragmentation of "application layer messages" for TCP

, in a multi-threaded context. Therefore all threads reading from a same "TCP sockfd" should also respect a same ["associated stateautomata of that TCP sockfd"](https://app.yinxiang.com/fx/27213209-98dc-4670-9f14-4ffa9d8e6202).   

Before designing locks for a multi-threaded context, [a tutorial from University of Wisconsin–Madison](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) is strongly recommended for reading. It gives a good explanation to a feasible design of `spin lock` and `mutex lock`, by 
- introducing granular building blocks such as "hardware & OS primitives", e.g. `test-and-set`, `compare-and-swap` and `park/unpark`, as well as 
- checking against several malicious scheduling cases. The tutorial is attached to this repository under [\<proj-root\>/docs/](https://github.com/genxium/CSocketChatroom/tree/master/docs).  


## An example for TCP

One of the possible multi-threading improvements to make is about "receiving", [see this note for some proposals](https://app.yinxiang.com/fx/b718999d-cc8e-45e1-b4ae-b519ee25850c). However profiling the performance difference and actually making an improvement is non-trivial. Refer to [this blog from Cloudflare](https://blog.cloudflare.com/how-to-receive-a-million-packets/) for more information.  

Moreover, using some `sub-thread-scope-task`s, e.g. [coroutine](https://en.wikipedia.org/wiki/Coroutine)/[goroutine in Golang](https://tour.golang.org/concurrency/1) helps equivalently, e.g. the approach used by [evio](https://github.com/tidwall/evio). Though designed for Java language, [JSR203](https://jcp.org/en/jsr/detail?id=203) is another good reference to read too.
