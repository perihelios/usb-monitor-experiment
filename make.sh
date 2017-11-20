#!/bin/bash -e
set -o pipefail

gcc -std=gnu99 -Wall usbmon.c format.c hexdump.c -o usbmon
strip usbmon
