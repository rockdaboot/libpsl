[![Build Status](https://travis-ci.org/rockdaboot/libpsl.png?branch=master)](https://travis-ci.org/rockdaboot/libpsl)

libpsl - C library to handle the Public Suffix List
===================================================

A "public suffix" is a domain name under which Internet users can directly register own names.

Browsers and other web clients can use it to

- avoid privacy-leaking "supercookies"
- avoid privacy-leaking "super domain" certificates ([see post from Jeffry Walton](http://lists.gnu.org/archive/html/bug-wget/2014-03/msg00093.html))
- domain highlighting parts of the domain in a user interface
- sorting domain lists by site

Libpsl...

- has built-in PSL data for fast access
- allows to load PSL data from files
- checks if a given domain is a "public suffix"
- provides immediate cookie domain verification
- finds the longest public part of a given domain
- finds the shortest private part of a given domain
- works with international domains (UTF-8 and IDNA2008 Punycode)
- is thread-safe

Find more information about the Publix Suffix List [here](http://publicsuffix.org/).

Download the Public Suffix List [here](https://hg.mozilla.org/mozilla-central/raw-file/tip/netwerk/dns/effective_tld_names.dat).


API Documentation
-----------------

You find the current API documentation [here](https://rockdaboot.github.io/libpsl).


Quick API example
-----------------

	#include <stdio.h>
	#include <libpsl.h>

	int main(int argc, char **argv)
	{
		const char *domain = "www.example.com";
		const char *cookie_domain = ".com";
		const psl_ctx_t *psl = psl_builtin();
		int is_public, is_acceptable;

		is_public = psl_is_public_suffix(psl, domain);
		printf("%s %s a public suffix.\n", domain, is_public ? "is" : "is not");

		is_acceptable = psl_is_cookie_domain_acceptable(psl, domain, cookie_domain);
		printf("cookie domain '%s' %s acceptable for domain '%s'.\n",
			cookie_domain, is_acceptable ? "is" : "is not", domain);

		return 0;
	}

Command Line Tool
-----------------

Libpsl comes with a tool 'psl' that gives you access to most of the
library API via command line.

	$ psl --help

prints the usage.

License
-------

Libpsl is made available under the terms of the MIT license.<br>
See the LICENSE file that accompanies this distribution for the full text of the license.


Building from git
-----------------

Download project and prepare sources with

		git clone http://github.com/rockdaboot/libpsl
		./autogen.sh
		./configure
		make
		make check


Mailing List
------------

[Mailing List Archive](http://news.gmane.org/gmane.network.dns.libpsl.bugs)

[Mailing List](https://groups.google.com/forum/#!forum/libpsl-bugs)

To join the mailing list send an email to

<libpsl-bugs+subscribe@googlegroups.com>

and follow the instructions provided by the answer mail.

Or click [join](https://groups.google.com/forum/#!forum/libpsl-bugs/join).
