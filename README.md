Target audience
======================

This repository is built for internal training use, demonstrating the design and implementation of a simple but feasible state automata for encoding and decoding "application layer messages" based on "transport layer TCP packet/bytestream".

The terms "transport layer" and "TCP" conform to the ["Internet Protocol Suite" defined in RFC1122](https://tools.ietf.org/html/rfc1122).


# Paths

You're reading `<proj-root>/README.md`.


# Version specification

Tested on Ubuntu14.04 with "GNU gcc version 5.4.0". This repo is `Linux specific`.


# Single-threaded polling

***Multitasking paradigm choice for multiplexing on a singlecore machine***

It's of active notice that "single-threaded polling" instead of "multi-threading" is used for multiplexing because

- upon a single sockfd, sending routines of 2 messages SHOULDN'T be interleaved at message partition scope 
- upon a single sockfd, receiving routines of 2 messages SHOULDN'T be interleaved at message partition scope  
- the time consumptions of sending/receiving routines are (almost) even
- the cost of thread-switching, e.g. "state operations" by pushing/popping registers to/from stack-memory, is comparatively high 

However, on a multicore machine running Linux, a single "OS process" using a single "thread/L(ight)W(eight)P(rocess)" COULDN'T use more than a single CPU core, thus multithreading would be necessary in some cases. 


# Multi-threading concerns for target audience

If your design allows multiple threads to access I/O of a same "sockfd", remember to guard it by proper locking. 

The best assumption about thread-safety of "send/recv" or "sendto/recvfrom" is at the scope of no larger than "transport layer packet", which is insufficient to automatically handle fragmentation of "application layer messages" in a multi-threaded context, especially for TCP.  

Before designing locks for a multi-threaded context, [a tutorial from University of Wisconsin–Madison](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) is strongly recommended for reading. 

It gives a good explanation to a feasible design of `spin lock` and `mutex lock`, by 
- introducing granular building blocks such as "hardware & OS primitives", e.g. `test-and-set`, `compare-and-swap` and `park/unpark`, as well as 
- checking against several malicious scheduling cases. 

The tutorial is attached to this repository under `<proj-root>/docs/`.  

Though designed for Java language, [JSR203](https://jcp.org/en/jsr/detail?id=203) is another good reference to read too.

## An example for TCP

One of the possible multi-threading improvements to make is about "receiving". 
- Change the currently single-threaded access to `epoll_wait` in [\<proj-root\>/tcp-epoll-server.cpp](https://github.com/genxium/CSocketChatroom/blob/master/tcp-epoll-server.cpp) to multi-threaded.
- After each `new client_fd` is accepted, create a dedicated thread for it to proceed with the `RecvPerClientFd: "RecvBytes -> ParseToApplictionLayerMessageBySpecificStateautomata"` routines.
  - Due to the use of stateautomata, it's inappropriate or at least non-trivial to make `RecvPerClientFd` multi-threaded.
- Buffer the results of each `RecvPerClientFd` into a corresponding `ApplicationLayerMessageQueuePerClientFd`, which in turn can be accessed in a multi-threaded manner.

However profiling the performance difference and actually making an improvement is non-trivial. Refer to [this blog from Cloudflare](https://blog.cloudflare.com/how-to-receive-a-million-packets/) for more information.  

Moreover, using some `sub-thread-scope-task`s, e.g. [coroutine](https://en.wikipedia.org/wiki/Coroutine)/[goroutine in Golang](https://tour.golang.org/concurrency/1) helps equivalently, e.g. the approach used by [evio](https://github.com/tidwall/evio). 
