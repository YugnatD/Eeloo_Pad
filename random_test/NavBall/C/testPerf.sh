#!/bin/sh
perf record ./main 0 0 0
perf report --no-children --demangle
