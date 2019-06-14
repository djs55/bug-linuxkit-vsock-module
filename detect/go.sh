#!/bin/sh

if /build/sbin/detect; then
  RESULT="Kernel is ok"
else
  RESULT="Kernel is broken :("
fi

# Spam all the consoles
for console in tty0 ttyS0 ttyAMA0 ttysclp0 stdout
do
  echo Writing to /dev/${console}
  echo "${RESULT}" | figlet > /dev/${console}
done

sleep 5s
halt
