#!/bin/bash
# Ensure stty sane runs even if the script is interrupted or crashes
trap "stty sane" EXIT

stty -icanon -echo
./forth_vm
