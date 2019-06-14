#!/bin/sh

# Spam all the consoles
for console in tty0 ttyS0 ttyAMA0 ttysclp0 stdout
do
  echo Writing to /dev/${console}
  echo "Kernel is ok" | figlet > /dev/${console}
done

sleep 5s
halt