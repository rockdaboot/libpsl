/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 */

#include <stdio.h> /* snprintf */
#include <stdlib.h> /* exit, system */
#include <string.h> /* strlen */
#if defined _WIN32
#	include <malloc.h>
#endif
#include "common.h"

int run_valgrind(const char *valgrind, const char *executable)
{
	char cmd[BUFSIZ];
	int n, rc;

	n = snprintf(cmd, sizeof(cmd), "TESTS_VALGRIND="" %s %s", valgrind, executable);
	if ((unsigned)n >= sizeof(cmd)) {
		printf("Valgrind command line is too long (>= %u)\n", (unsigned) sizeof(cmd));
		return EXIT_FAILURE;
	}

	if ((rc = system(cmd))) {
		printf("Failed to execute with '%s' (system() returned %d)\n", valgrind, rc);
	}

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
