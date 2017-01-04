#!/bin/bash

make check-coverage-libicu
pip install --user cpp-coveralls
coveralls -t d9uGTP4NSD092kh2b85aDSsEDxatcYC6F --include src/
