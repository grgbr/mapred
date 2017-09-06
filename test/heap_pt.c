#include "dbnm_heap.h"
#include "array.h"
#include "karn_pt.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

struct hppt_iface {
	char  *hppt_name;
	int  (*hppt_load)(const char *pathname);
	void (*hppt_insert)(unsigned long long *nsecs);
	void (*hppt_extract)(unsigned long long *nsecs);
};

static struct pt_entries hppt_entries;

/******************************************************************************
 * Doubly linked list based binomial heap
 ******************************************************************************/

struct hppt_dbnm_key {
	struct dbnm_heap_node node;
	unsigned int          value;
};

static struct hppt_dbnm_key *dbnm_heap_keys;

static int
dbnm_heap_compare_min(const struct dbnm_heap_node *first,
                      const struct dbnm_heap_node *second)
{
	return ((struct hppt_dbnm_key *)first)->value -
	       ((struct hppt_dbnm_key *)second)->value;
}

static void
hppt_dbnm_insert_bulk(struct dbnm_heap *heap)
{
	int                   n;
	struct hppt_dbnm_key *k;

	dbnm_heap_init(heap);

	for (n = 0, k = dbnm_heap_keys; n < hppt_entries.pt_nr; n++, k++)
		dbnm_heap_insert(heap, &k->node, dbnm_heap_compare_min);
}

static int
dbnm_heap_validate(void)
{
	struct dbnm_heap      heap;
	struct hppt_dbnm_key *cur, *old;
	int                   n;

	hppt_dbnm_insert_bulk(&heap);

	old = dbnm_heap_entry(dbnm_heap_extract(&heap, dbnm_heap_compare_min),
	                      struct hppt_dbnm_key, node);

	for (n = 1; n < hppt_entries.pt_nr; n++) {
		cur = dbnm_heap_entry(dbnm_heap_extract(&heap,
		                                        dbnm_heap_compare_min),
		                      struct hppt_dbnm_key, node);

		if (old->value > cur->value) {
			fprintf(stderr, "Bogus heap scheme\n");
			return EXIT_FAILURE;
		}

		old = cur;
	}

	return EXIT_SUCCESS;
}

static int
hppt_dbnm_load(const char *pathname)
{
	struct hppt_dbnm_key *k;

	if (pt_open_entries(pathname, &hppt_entries))
		return EXIT_FAILURE;

	dbnm_heap_keys = malloc(hppt_entries.pt_nr * sizeof(*dbnm_heap_keys));
	if (!dbnm_heap_keys)
		return EXIT_FAILURE;

	pt_init_entry_iter(&hppt_entries);

	k = dbnm_heap_keys;
	while (!pt_iter_entry(&hppt_entries, &k->value))
		k++;

	return dbnm_heap_validate();
}

static void
hppt_dbnm_insert(unsigned long long *nsecs)
{
	struct timespec  start, elapse;
	struct dbnm_heap heap;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	hppt_dbnm_insert_bulk(&heap);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &elapse);

	elapse = pt_tspec_sub(&elapse, &start);
	*nsecs = pt_tspec2ns(&elapse);

	return;
}

static void
hppt_dbnm_extract(unsigned long long *nsecs)
{
	struct timespec  start, elapse;
	struct dbnm_heap heap;
	int              n;

	hppt_dbnm_insert_bulk(&heap);

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (n = 0; n < hppt_entries.pt_nr; n++)
		dbnm_heap_extract(&heap, dbnm_heap_compare_min);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &elapse);

	elapse = pt_tspec_sub(&elapse, &start);
	*nsecs = pt_tspec2ns(&elapse);

	return;
}

static const struct hppt_iface hppt_algos[] = {
	{
		.hppt_name    = "dbnm",
		.hppt_load    = hppt_dbnm_load,
		.hppt_insert  = hppt_dbnm_insert,
		.hppt_extract = hppt_dbnm_extract
	},
};

static const struct hppt_iface *
hppt_setup_algo(const char *algo_name)
{
	unsigned int a;

	for (a = 0; a < array_nr(hppt_algos); a++)
		if (!strcmp(algo_name, hppt_algos[a].hppt_name))
			return &hppt_algos[a];

	fprintf(stderr, "Invalid \"%s\" sort algorithm\n", algo_name);

	return NULL;
}

static void
usage(const char *me)
{
	fprintf(stderr,
	        "Usage: %s [OPTIONS] FILE ALGORITHM LOOPS\n"
	        "where OPTIONS:\n"
	        "    -p|--prio  PRIORITY\n"
	        "    -h|--help\n",
	        me);
}

int main(int argc, char *argv[])
{
	const struct hppt_iface *algo;
	unsigned int                 l, loops = 0;
	int                          prio = 0;
	unsigned long long           nsecs;

	while (true) {
		int                        opt;
		static const struct option lopts[] = {
			{"help",    0, NULL, 'h'},
			{"prio",    1, NULL, 'p'},
			{0,         0, 0,    0}
		};

		opt = getopt_long(argc, argv, "hp:", lopts, NULL);
		if (opt < 0)
			/* No more options: go parsing positional arguments. */
			break;

		switch (opt) {
		case 'p': /* priority */
			if (pt_parse_sched_prio(optarg, &prio)) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}

			break;

		case 'h': /* Help message. */
			usage(argv[0]);
			return EXIT_SUCCESS;

		case '?': /* Unknown option. */
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	/*
	 * Check positional arguments are properly specified on command
	 * line.
	 */
	argc -= optind;
	if (argc != 3) {
		fprintf(stderr, "Invalid number of arguments\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	algo = hppt_setup_algo(argv[optind + 1]);
	if (!algo)
		return EXIT_FAILURE;

	if (pt_parse_loop_nr(argv[optind + 2], &loops))
		return EXIT_FAILURE;

	if (algo->hppt_load(argv[optind]))
		return EXIT_FAILURE;

	if (pt_setup_sched_prio(prio))
		return EXIT_FAILURE;

	for (l = 0; l < loops; l++) {
		algo->hppt_insert(&nsecs);
		printf("insert: nsec=%llu\n", nsecs);
	}

	for (l = 0; l < loops; l++) {
		algo->hppt_extract(&nsecs);
		printf("extract: nsec=%llu\n", nsecs);
	}

	return EXIT_SUCCESS;
}
