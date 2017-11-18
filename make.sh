#!/bin/bash -e
set -o pipefail

gcc -std=gnu99 -Wall usbmon.c -o usbmon
strip usbmon
