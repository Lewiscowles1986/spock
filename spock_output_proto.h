/*-------------------------------------------------------------------------
 *
 * spock_output_proto.h
 *		spock protocol
 *
 * Copyright (c) 2015, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  spock_output_proto.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SPOCK_OUTPUT_PROTO_H
#define SPOCK_OUTPUT_PROTO_H

#include "lib/stringinfo.h"
#include "replication/reorderbuffer.h"
#include "utils/relcache.h"

#include "spock_output_plugin.h"

/*
 * Protocol capabilities
 *
 * SPOCK_PROTO_VERSION_NUM is our native protocol and the greatest version
 * we can support. SPOCK_PROTO_MIN_VERSION_NUM is the oldest version we
 * have backwards compatibility for. We negotiate protocol versions during the
 * startup handshake. See the protocol documentation for details.
 */
#define SPOCK_PROTO_VERSION_NUM 1
#define SPOCK_PROTO_MIN_VERSION_NUM 1

/*
 * The startup parameter format is versioned separately to the rest of the wire
 * protocol because we negotiate the wire protocol version using the startup
 * parameters sent to us. It hopefully won't ever need to change, but this
 * field is present in case we do need to change it, e.g. to a structured json
 * object. We can look at the startup params version to see whether we can
 * understand the startup params sent by the client and to fall back to
 * reading an older format if needed.
 */
#define SPOCK_STARTUP_PARAM_FORMAT_FLAT 1

/*
 * For similar reasons to the startup params
 * (SPOCK_STARTUP_PARAM_FORMAT_FLAT) the startup reply message format is
 * versioned separately to the rest of the protocol. The client has to be able
 * to read it to find out what protocol version was selected by the upstream
 * when using the native protocol.
 */
#define SPOCK_STARTUP_MSG_FORMAT_FLAT 1

typedef enum SpockProtoType
{
	SpockProtoNative,
	SpockProtoJson
} SpockProtoType;

typedef void (*spock_write_rel_fn) (StringInfo out, SpockOutputData * data,
						   Relation rel, Bitmapset *att_list);

typedef void (*spock_write_begin_fn) (StringInfo out, SpockOutputData * data,
													  ReorderBufferTXN *txn);
typedef void (*spock_write_commit_fn) (StringInfo out, SpockOutputData * data,
							   ReorderBufferTXN *txn, XLogRecPtr commit_lsn);

typedef void (*spock_write_origin_fn) (StringInfo out, const char *origin,
													   XLogRecPtr origin_lsn);

typedef void (*spock_write_insert_fn) (StringInfo out, SpockOutputData * data,
										   Relation rel, HeapTuple newtuple,
										   Bitmapset *att_list);
typedef void (*spock_write_update_fn) (StringInfo out, SpockOutputData * data,
											Relation rel, HeapTuple oldtuple,
											HeapTuple newtuple,
											Bitmapset *att_list);
typedef void (*spock_write_delete_fn) (StringInfo out, SpockOutputData * data,
										   Relation rel, HeapTuple oldtuple,
										   Bitmapset *att_list);

typedef void (*write_startup_message_fn) (StringInfo out, List *msg);

typedef struct SpockProtoAPI
{
	spock_write_rel_fn write_rel;
	spock_write_begin_fn write_begin;
	spock_write_commit_fn write_commit;
	spock_write_origin_fn write_origin;
	spock_write_insert_fn write_insert;
	spock_write_update_fn write_update;
	spock_write_delete_fn write_delete;
	write_startup_message_fn write_startup_message;
} SpockProtoAPI;

extern SpockProtoAPI *spock_init_api(SpockProtoType typ);

#endif /* SPOCK_OUTPUT_PROTO_H */
