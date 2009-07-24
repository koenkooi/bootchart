/*
 * bootchart.h
 *
 * Copyright (c) 2009 Intel Coproration
 * Authors:
 *   Auke Kok <auke-jan.h.kok@intel.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define VERSION "0.1"

#define MAXCPUS         8
#define MAXPIDS     65535
#define MAXSAMPLES   4096


struct block_stat_struct {
	/* /proc/vmstat pgpgin & pgpgout */
	int bi;
	int bo;
};

struct cpu_stat_sample_struct {
	/* /proc/schedstat fields 10 & 11 (after name) */
	double runtime;
	double waittime;
};

struct cpu_stat_struct {
	/* per cpu array */
	struct cpu_stat_sample_struct sample[MAXSAMPLES];
};

/* per process, per sample data we will log */
struct ps_sched_struct {
	/* /proc/<n>/schedstat fields 1 & 2 */
	double runtime;
	double waittime;
};

/* process info */
struct ps_struct {
	struct ps_struct *next;
	struct ps_struct *prev;

	/* must match - otherwise it's a new process with same PID */
	char name[16];
	int pid;
	int ppid;

	/* index to first/last seen timestamps */
	int first;
	int last;

	/* records actual start time, may be way before bootchart runs */
	double starttime;

	struct ps_sched_struct sample[MAXSAMPLES];
};


extern double graph_start;
extern double sampletime[];
extern struct ps_struct *ps[]; /* ll */
extern struct block_stat_struct blockstat[];
extern struct cpu_stat_struct cpustat[];
extern int pscount;
extern int relative;
extern int samples;
extern double interval;

extern FILE *of;

extern double gettime_ns(void);
extern void log_uptime(void);
extern void log_sample(int sample);

extern void svg_do(void);
