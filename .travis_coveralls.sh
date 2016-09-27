#!/bin/bash

make check-coverage-libicu
pip install --user cpp-coveralls
coveralls --include libwget/ --include src/ -e "src/psl2c.c"
