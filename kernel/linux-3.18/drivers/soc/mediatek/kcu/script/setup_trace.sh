#!/bin/bash

mount -t debugfs nodev /sys/kernel/debug
cd /sys/kernel/debug/tracing
echo 0 >tracing_on
echo sig==35 >events/signal/filter
echo 1 >events/signal/enable
echo "(id==173) || (id==177)" >events/raw_syscalls/sys_enter/filter
echo "(id==177) || (id==179) " >events/raw_syscalls/sys_exit/filter
echo 1 >events/raw_syscalls/enable
echo 1 >tracing_on
