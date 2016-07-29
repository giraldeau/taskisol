#!/bin/sh -x

time ./taskisol default syscall
time ./taskisol default pagefault
time ./taskisol signal syscall
time ./taskisol signal pagefault

