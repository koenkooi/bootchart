/*
 * log.c
 *
 * Copyright (c) 2009 Intel Coproration
 * Authors:
 *   Auke Kok <auke-jan.h.kok@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "bootchart.h"


double gettime_ns(void)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	return (now.tv_sec + (now.tv_usec / 1000000.0));
}


void log_uptime(void)
{
	FILE *f;
	char str[32];
	double now;
	double uptime;

	f = fopen("/proc/uptime", "r");
	if (!f)
		return;
	if (!fscanf(f, "%s %*s", str)) {
		fclose(f);
		return;
	}
	fclose(f);
	uptime = strtod(str, NULL);

	now = gettime_ns();

	/* start graph at kernel boot time */
	if (relative)
		graph_start = now;
	else
		graph_start = now - uptime;
}


void log_sample(int sample)
{
	FILE *f;
	DIR *proc;
	char key[256];
	char val[256];
	char rt[256];
	char wt[256];
	int c;
	int p;
	struct dirent *ent;


	/* block stuff */
	f = fopen("/proc/vmstat", "r");
	if (!f) {
		perror("fopen(\"/proc/vmstat\")");
		exit (EXIT_FAILURE);
	}

	while (fscanf(f, "%s %s", key, val) > 0) {
		if (!strcmp(key, "pgpgin"))
			blockstat[sample].bi = atoi(val);
		if (!strcmp(key, "pgpgout")) {
			blockstat[sample].bo = atoi(val);
			break;
		}
	}

	fclose(f);

	/* overall CPU utilization */
	f = fopen("/proc/schedstat", "r");
	if (!f) {
		perror("fopen(\"/proc/schedstat\")");
		exit (EXIT_FAILURE);
	}

	while (!feof(f)) {
		int n;
		char line[4096];

		if (!fgets(line, 4095, f))
			continue;

		n = sscanf(line, "%s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s %s", key, rt, wt);

		if (n < 3)
			continue;

		if (strstr(key, "cpu")) {
			c = key[3] - '0';
			cpustat[c].sample[sample].runtime = atoll(rt);
			cpustat[c].sample[sample].waittime = atoll(wt);
		}
	}

	fclose(f);

	/* find all processes */
	proc = opendir("/proc");
	if (!proc)
		return;

	while ((ent = readdir(proc)) != NULL) {
		char filename[PATH_MAX];
		int pid;

		if ((ent->d_name[0] < '0') || (ent->d_name[0] > '9'))
			continue;

		pid = atoi(ent->d_name);

		if (pid > MAXPIDS)
			continue;

		sprintf(filename, "/proc/%d/schedstat", pid);

		f = fopen(filename, "r");
		if (!f)
			continue;
		if (!fscanf(f, "%s %s %*s", rt, wt)) {
			fclose(f);
			continue;
		}
		fclose(f);

		if (!ps[pid]) {
			char line[80];
			char t[32];

			pscount++;

			/* alloc & insert */
			ps[pid] = malloc(sizeof(struct ps_struct));
			if (!ps[pid]) {
				perror("malloc ps[pid]");
				exit (EXIT_FAILURE);
			}

			/* mark our first sample */
			ps[pid]->first = sample;

			/* get name, start time */
			sprintf(filename, "/proc/%d/sched", pid);
			f = fopen(filename, "r");

			if (!fgets(line, 79, f)) {
				fclose(f);
				continue;
			}
			if (!sscanf(line, "%s %*s %*s", key)) {
				fclose(f);
				continue;
			}

			strncpy(ps[pid]->name, key, 16);
			/* discard line 2 */
			if (!fgets(line, 79, f)) {
				fclose(f);
				continue;
			}

			if (!fgets(line, 79, f)) {
				fclose(f);
				continue;
			}
			if (!sscanf(line, "%*s %*s %s", t)) {
				fclose(f);
				continue;
			}
			fclose(f);

			ps[pid]->starttime = strtod(t, NULL);

			/* ppid */
			sprintf(filename, "/proc/%d/stat", pid);
			f = fopen(filename, "r");
			if (!f)
				continue;
			if (!fscanf(f, "%*s %*s %*s %i", &p)) {
				fclose(f);
				continue;
			}
			fclose(f);
			ps[pid]->ppid = p;

		}

		ps[pid]->pid = pid;
		ps[pid]->last = sample;
		ps[pid]->sample[sample].runtime = atoll(rt);
		ps[pid]->sample[sample].waittime = atoll(wt);

	}

	// FIXME: why does this give glibc double free or crap?
	closedir(proc);
}

