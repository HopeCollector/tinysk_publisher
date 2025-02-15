#!/bin/bash

# The script is pretty simple. It gets the CPU usage, CPU temperature, memory usage, and the current IP address. The script prints the values separated by a semicolon. 
# The script can be run using the following command: 
# $ bash status.sh

# The output will be something like this: 
# 27.1;44.2;39.7;5.1;0.113;192.168.1.2

# Get the CPU usage
cpu_usage=$(top -bn1 | grep "Cpu(s)" | \
           sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | \
           awk '{print 100 - $1}')

# Get the CPU Temperature
cpu_temp=$(awk '{printf "%.1f", $1/1000}' /sys/class/thermal/thermal_zone0/temp)

# Get memory usage
mem_usage=$(free | grep Mem | awk '{printf "%.1f", $3/$2 * 100.0}')

# Get battery voltage
bat_vol=5.1

# Get battery current
bat_cur=0.113

# Get current ip
ip=$(hostname -I)

# print the status
echo "$cpu_usage;$cpu_temp;$mem_usage;$bat_vol;$bat_cur;$ip"
