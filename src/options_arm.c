/* Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

#define AMI_BMC "/proc/ractrends/Helper/FwInfo"
#define OPENFSI_BMC "/sys/bus/platform/devices/gpio-fsi/fsi0/"

static const char witherspoon[] = "witherspoon";
static const char romulus[] = "romulus";
static const char zaius[] = "zaius";

enum backend default_backend(void)
{
	int rc;
	rc = access(AMI_BMC, F_OK);
	if (rc == 0) /* AMI BMC */
		return I2C;

	rc = access(OPENFSI_BMC, F_OK);
	if (rc == 0) /* Kernel interface. OpenBMC */
		return KERNEL;

	/*
	 * "This should never be the default" - Apopple 2017
	 */
	fprintf(stderr, "Couldn't locate a good backend.\n");
	fprintf(stderr, "It is possible that the FSI backend will work.\n");
	fprintf(stderr, "You will need to select this along with the correct\n");
	fprintf(stderr, "target yourself on the commandline\n");
	fprintf(stderr, "`pdbg -b fsi -d [p8 | p9w | p9r | p9z] ...`\n");
	return FAKE;
}

void print_backends(FILE *stream)
{
	fprintf(stream, "I2C KERNEL FSI");
}

bool backend_is_possible(enum backend backend)
{
	if (backend == I2C && access(AMI_BMC, F_OK) == 0)
		return true;
	if (backend == KERNEL && access(OPENFSI_BMC, F_OK) == 0)
		return true;

	return backend == FSI;
}

void print_targets(FILE *stream)
{
	fprintf(stream, "KERNEL: No target is necessary\n");
	fprintf(stream, "I2C: No target is necessary\n");
	fprintf(stream, "FSI: p8 p9w p9r p9z\n");
}

const char *default_target(enum backend backend)
{
	FILE *dt_compatible;
	char line[256];
	char *p;

	if (backend == I2C || backend == KERNEL) /* No target nessesary */
		return NULL;

	dt_compatible = fopen("/proc/device-tree/compatible", "r");
	if (!dt_compatible)
		return NULL;

	p = fgets(line, sizeof(line), dt_compatible);
	fclose(dt_compatible);
	if (!p) /* Uh oh*/
		return NULL;

	if (strstr(witherspoon, line))
		return witherspoon;

	if (strstr(romulus, line))
		return romulus;

	if (strstr(zaius, line))
		return zaius;

	return NULL;
}

bool target_is_possible(enum backend backend, const char *target)
{
	const char *def;

	if (!backend_is_possible(backend))
		return false;

	if (backend == I2C || backend == KERNEL) /* No target is nessesary */
		return true;

	def = default_target(backend);
	if (!def || !target)
		return false;

	return strcmp(def, target) == 0;
}
