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
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <target.h>
#include <operations.h>

#define HTM_DUMP_BASENAME "htm.dump"

static char *get_htm_dump_filename(void)
{
	char *filename;
	int i;

	filename = strdup(HTM_DUMP_BASENAME);
	if (!filename)
		return NULL;

	i = 0;
	while (access(filename, F_OK) == 0) {
		free(filename);
		if (asprintf(&filename, "%s.%d", HTM_DUMP_BASENAME, i) == -1)
			return NULL;
		i++;
	}

	return filename;
}

int run_htm_start(int optind, int argc, char *argv[])
{
	struct target *target;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Starting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

int run_htm_stop(int optind, int argc, char *argv[])
{
	struct target *target;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Stopping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_stop(target) != 1)
			printf("Couldn't stop HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

int run_htm_status(int optind, int argc, char *argv[])
{
	struct target *target;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_status(target) != 1)
			printf("Couldn't get HTM@%d#%d status\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
		printf("\n\n");
	}

	return rc;
}

int run_htm_reset(int optind, int argc, char *argv[])
{
	uint64_t old_base = 0, base, size;
	struct target *target;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Resetting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
				base, size);
			printf("In case of system crash/xstop use the following to dump the trace on the BMC:\n");
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n",
					base, size);
		}
		rc++;
	}

	return rc;
}

int run_htm_dump(int optind, int argc, char *argv[])
{
	struct target *target;
	char *filename;
	int rc = 0;

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	/* size = 0 will dump everything */
	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Dumping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_dump(target, 0, filename) == 1)
			printf("Couldn't dump HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}
	free(filename);

	return rc;
}

int run_htm_trace(int optind, int argc, char *argv[])
{
	uint64_t old_base = 0, base, size;
	struct target *target;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		/*
		 * Don't mind if stop fails, it will fail if it wasn't
		 * running, if anything bad is happening reset will fail
		 */
		htm_stop(target);
		printf("Resetting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_reset(target, &base, &size) != 1)
			printf("Couldn't reset HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		if (old_base != base) {
			printf("The kernel has initialised HTM memory at:\n");
			printf("base: 0x%016" PRIx64 " for 0x%016" PRIx64 " size\n",
					base, size);
			printf("./pdbg getmem 0x%016" PRIx64 " 0x%016" PRIx64 " > htm.dump\n\n",
					base, size);
		}
		old_base = base;
	}

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Starting HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_start(target) != 1)
			printf("Couldn't start HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}

	return rc;
}

int run_htm_analyse(int optind, int argc, char *argv[])
{
	struct target *target;
	char *filename;
	int rc = 0;

	for_each_class_target("nhtm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		htm_stop(target);
	}

	filename = get_htm_dump_filename();
	if (!filename)
		return 0;

	printf("Dumping HTM trace to file [chip].[#]%s\n", filename);
	for_each_class_target("htm", target) {
		if (dt_node_is_disabled(target->dn))
			continue;

		printf("Dumping HTM@%d#%d\n",
			dt_get_chip_id(target->dn), target->index);
		if (htm_dump(target, 0, filename) != 1)
			printf("Couldn't dump HTM@%d#%d\n",
				dt_get_chip_id(target->dn), target->index);
		rc++;
	}
	free(filename);

	return rc;
}


