# slackware-module
This is a university assignment where I created a SlackWare kernel module in C and a matching userspace program in Go.

The kernel module acts as a driver for a simulated air sensor device. The device will output a numeric value when read, and can be calibrated with a gain and offset.

Using regression, the userspace program can simplify or automate the process of calibrating the device driver by parsing optional arguments.
