#!/bin/bash

make check-coverage-libicu
pip install --user urllib3[secure] cpp-coveralls

# Work around https://github.com/eddyxu/cpp-coveralls/issues/108 by manually
# installing the pyOpenSSL module and injecting it into urllib3 as per
# https://urllib3.readthedocs.io/en/latest/user-guide.html#ssl-py2
sed -i -e '/^import sys$/a import urllib3.contrib.pyopenssl\nurllib3.contrib.pyopenssl.inject_into_urllib3()' `which coveralls`

coveralls -t d9uGTP4NSD092kh2b85aDSsEDxatcYC6F --include src/
