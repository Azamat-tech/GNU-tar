#!/bin/bash

export GNUTAR="$(which tar)"
export MYTAR_C="$(readlink -e mytar.c)"
export STEF="$(readlink -e stef/stef.sh)"
cd tests/
./run-tests.sh $(cat phase-1.tests)
rm -rf dir.*
