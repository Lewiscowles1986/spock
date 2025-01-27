/*-------------------------------------------------------------------------
 *
 * spock_worker.h
 *              spock worker helper functions
 *
 * Copyright (c) 2022-2023, pgEdge, Inc.
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, The Regents of the University of California
 *
 *-------------------------------------------------------------------------
 */
#ifndef SPOCK_WORKER_H
#define SPOCK_WORKER_H

#include "storage/lock.h"

#include "spock.h"

typedef enum {
	SPOCK_WORKER_NONE,		/* Unused slot. */
	SPOCK_WORKER_MANAGER,	/* Manager. */
	SPOCK_WORKER_APPLY,		/* Apply. */
	SPOCK_WORKER_SYNC		/* Special type of Apply that synchronizes
								 * one table. */
} SpockWorkerType;

typedef struct SpockApplyWorker
{
	Oid			subid;				/* Subscription id for apply worker. */
	bool		sync_pending;		/* Is there new synchronization info pending?. */
	XLogRecPtr	replay_stop_lsn;	/* Replay should stop here if defined. */
	RepOriginId	replorigin;			/* Remote origin id of apply worker. */
	TimestampTz	last_ts;			/* Last remote commit timestamp. */
} SpockApplyWorker;

typedef struct SpockSyncWorker
{
	SpockApplyWorker	apply; /* Apply worker info, must be first. */
	NameData	nspname;	/* Name of the schema of table to copy if any. */
	NameData	relname;	/* Name of the table to copy if any. */
} SpockSyncWorker;

typedef struct SpockWorker {
	SpockWorkerType	worker_type;

	/* Generation counter incremented at each registration */
	uint16 generation;

	/* Pointer to proc array. NULL if not running. */
	PGPROC *proc;

	/* Time at which worker crashed (normally 0). */
	TimestampTz	crashed_at;

	/* Database id to connect to. */
	Oid		dboid;

	/* Type-specific worker info */
	union
	{
		SpockApplyWorker apply;
		SpockSyncWorker sync;
	} worker;

} SpockWorker;

typedef struct SpockContext {
	/* Write lock for the entire context. */
	LWLock	   *lock;

	/* Access lock for the Conflict Tracking Hash. */
	LWLock	   *cth_lock;

	/* Interval for pruning the conflict_tracker table */
	int		ctt_prune_interval;
	Datum	ctt_last_prune;

	/* Supervisor process. */
	PGPROC	   *supervisor;

	/* Signal that subscription info have changed. */
	bool		subscriptions_changed;

	/* Background workers. */
	int			total_workers;
	SpockWorker  workers[FLEXIBLE_ARRAY_MEMBER];
} SpockContext;

typedef enum spockStatsType
{
	SPOCK_STATS_INSERT_COUNT = 0,
	SPOCK_STATS_UPDATE_COUNT,
	SPOCK_STATS_DELETE_COUNT,
	SPOCK_STATS_CONFLICT_COUNT,
	SPOCK_STATS_DCA_COUNT,

	SPOCK_STATS_NUM_COUNTERS = SPOCK_STATS_DCA_COUNT + 1
} spockStatsType;

typedef struct spockStatsKey
{
	/* hash key */
	Oid			dboid;		/* Database Oid */
	Oid			subid;		/* Subscription (InvalidOid for sender) */
	Oid			relid;		/* Table Oid */
} spockStatsKey;

typedef struct spockStatsEntry
{
	spockStatsKey	key;	/* hash key */

	int64			counter[SPOCK_STATS_NUM_COUNTERS];
	slock_t			mutex;
} spockStatsEntry;

extern HTAB				   *SpockHash;
extern HTAB				   *SpockConflictHash;
extern SpockContext		   *SpockCtx;
extern SpockWorker		   *MySpockWorker;
extern SpockApplyWorker	   *MyApplyWorker;
extern SpockSubscription   *MySubscription;
extern int					spock_stats_max_entries_conf;
extern int					spock_stats_max_entries;
extern bool					spock_stats_hash_full;

#define SPOCK_STATS_MAX_ENTRIES(_nworkers) \
	(spock_stats_max_entries_conf < 0 ? (1000 * _nworkers) \
									  : spock_stats_max_entries_conf)

extern volatile sig_atomic_t got_SIGTERM;

extern void handle_sigterm(SIGNAL_ARGS);

extern void spock_subscription_changed(Oid subid, bool kill);

extern void spock_worker_shmem_init(void);

extern int spock_worker_register(SpockWorker *worker);
extern void spock_worker_attach(int slot, SpockWorkerType type);

extern SpockWorker *spock_manager_find(Oid dboid);
extern SpockWorker *spock_apply_find(Oid dboid, Oid subscriberid);
extern List *spock_apply_find_all(Oid dboid);

extern SpockWorker *spock_sync_find(Oid dboid, Oid subid,
											const char *nspname, const char *relname);
extern List *spock_sync_find_all(Oid dboid, Oid subscriberid);

extern SpockWorker *spock_get_worker(int slot);
extern bool spock_worker_running(SpockWorker *w);
extern void spock_worker_kill(SpockWorker *worker);

extern const char * spock_worker_type_name(SpockWorkerType type);
extern void handle_stats_counter(Relation relation, Oid subid,
								spockStatsType typ, int ntup);
/*
extern void handle_pr_counters(Relation relation, char *slotname, Oid nodeid, spockStatsType typ, int ntup);
*/

#endif /* SPOCK_WORKER_H */
