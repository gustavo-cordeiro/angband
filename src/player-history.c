/**
 * \file player-history.c
 * \brief Character auto-history creation, management, and display
 *
 * Copyright (c) 2007 J.D. White
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "cave.h"
#include "game-world.h"
#include "obj-desc.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player-history.h"

/**
 * Number of slots available at birth in the player history list.  Defaults to
 * 10 and will expand automatically as new history entries are added, up the
 * the maximum defined value.
 */
#define HISTORY_BIRTH_SIZE  10
#define HISTORY_MAX 5000



/* The historical list for the character */
struct history_info *history_list;

/* Index of first writable entry */
static size_t history_ctr;

/* Current size of history list */
static size_t history_size;

/**
 * Initialise an empty history list.
 */
static void history_init(size_t entries)
{
	history_ctr = 0;
	history_size = entries;
	history_list = mem_zalloc(history_size * sizeof(struct history_info));
}


/**
 * Clear any existing history.
 */
void history_clear(void)
{
	if (!history_list) return;

	mem_free(history_list);
	history_list = NULL;
	history_ctr = 0;
	history_size = 0;
}


/**
 * Set the number of history items.
 */
static bool history_set_num(size_t num)
{
	if (num > HISTORY_MAX)
		num = HISTORY_MAX;

	if (num <= history_size)
		return FALSE;

	/* Reallocate the list */
	history_list = mem_realloc(history_list, num * sizeof(struct history_info));
	history_size = num;

	return TRUE;
}


/**
 * Return the number of history entries.
 */
size_t history_get_num(void)
{
	return history_ctr;
}


/**
 * Mark artifact number `id` as known.
 */
static bool history_know_artifact(struct artifact *artifact)
{
	size_t i = history_ctr;
	assert(artifact);

	while (i--) {
		if (history_list[i].a_idx == artifact->aidx) {
			hist_wipe(history_list[i].type);
			hist_on(history_list[i].type, HIST_ARTIFACT_KNOWN);
			return TRUE;
		}
	}

	return FALSE;
}


/**
 * Mark artifact number `id` as lost forever, either due to leaving it on a
 * level, or due to a store purging its inventory after the player sold it.
 */
bool history_lose_artifact(struct artifact *artifact)
{
	size_t i = history_ctr;
	assert(artifact);

	while (i--) {
		if (history_list[i].a_idx == artifact->aidx) {
			hist_on(history_list[i].type, HIST_ARTIFACT_LOST);
			return TRUE;
		}
	}

	/* If we lost an artifact that didn't previously have a history, then we
	 * missed it */
	history_add_artifact(artifact, FALSE, FALSE);

	return FALSE;
}


/**
 * Add an entry with text `event` to the history list, with type `type`
 * ("HIST_xxx" in player-history.h), and artifact number `id` (0 for
 * everything else).
 *
 * Return TRUE on success.
 */
bool history_add_full(bitflag *type, struct artifact *artifact, s16b dlev,
		s16b clev, s32b turnno, const char *text)
{
	/* Allocate or expand the history list as needed */
	if (!history_list)
		history_init(HISTORY_BIRTH_SIZE);
	else if ((history_ctr == history_size) &&
			 !history_set_num(history_size + 10))
		return FALSE;

	/* History list exists and is not full.  Add an entry at the current
	 * counter location. */
	hist_copy(history_list[history_ctr].type, type);
	history_list[history_ctr].dlev = dlev;
	history_list[history_ctr].clev = clev;
	history_list[history_ctr].a_idx = artifact ? artifact->aidx : 0;
	history_list[history_ctr].turn = turnno;
	my_strcpy(history_list[history_ctr].event,
	          text, sizeof(history_list[history_ctr].event));

	history_ctr++;

	return TRUE;
}


/**
 * Add an entry with text `event` to the history list, with type `type`
 * ("HIST_xxx" in player-history.h), and artifact number `id` (0 for
 * everything else).
 *
 * Return TRUE on success.
 */
bool history_add(const char *event, int type, struct artifact *artifact)
{
	bitflag h[HIST_SIZE];
	hist_wipe(h);
	hist_on(h, type);

	return history_add_full(h, artifact, player->depth, player->lev, player->total_energy / 100, event);
}


/**
 * Returns TRUE if the artifact is KNOWN in the history log.
 */
bool history_is_artifact_known(struct artifact *artifact)
{
	size_t i = history_ctr;
	assert(artifact);

	while (i--) {
		if (hist_has(history_list[i].type, HIST_ARTIFACT_KNOWN) &&
				history_list[i].a_idx == artifact->aidx)
			return TRUE;
	}

	return FALSE;
}


/**
 * Returns TRUE if the artifact denoted by a_idx is an active entry in
 * the history log (i.e. is not marked HIST_ARTIFACT_LOST).  This permits
 * proper handling of the case where the player loses an artifact but (in
 * preserve mode) finds it again later.
 */
static bool history_is_artifact_logged(struct artifact *artifact)
{
	size_t i = history_ctr;
	assert(artifact);

	while (i--) {
		/* Don't count ARTIFACT_LOST entries; then we can handle
		 * re-finding previously lost artifacts in preserve mode  */
		if (hist_has(history_list[i].type, HIST_ARTIFACT_LOST))
			continue;

		if (history_list[i].a_idx == artifact->aidx)
			return TRUE;
	}

	return FALSE;
}


/**
 * Adding artifacts to the history list is trickier than other operations.
 * This is a wrapper function that gets some of the logic out of places
 * where it really doesn't belong.  Call this to add an artifact to the history
 * list or make the history entry visible--history_add_artifact will make that
 * determination depending on what object_is_known returns for the artifact.
 */
bool history_add_artifact(struct artifact *artifact, bool known, bool found)
{
	struct object object_type_body;
	struct object *fake = &object_type_body;

	char o_name[80];
	char buf[80];

	assert(artifact);

	/* Make fake artifact for description purposes */
	object_wipe(fake);
	make_fake_artifact(fake, artifact);
	object_desc(o_name, sizeof(o_name), fake,
				ODESC_PREFIX | ODESC_BASE | ODESC_SPOIL);
	object_wipe(fake);
	strnfmt(buf, sizeof(buf), (found)?"Found %s":"Missed %s", o_name);

	/* Known objects gets different treatment */
	if (known) {
		/* Try revealing any existing artifact, otherwise log it */
		if (history_is_artifact_logged(artifact))
			history_know_artifact(artifact);
		else
			history_add(buf, HIST_ARTIFACT_KNOWN, artifact);
	} else {
		if (!history_is_artifact_logged(artifact)) {
			bitflag type[HIST_SIZE];
			hist_wipe(type);
			hist_on(type, HIST_ARTIFACT_UNKNOWN);
			if (!found)
				hist_on(type, HIST_ARTIFACT_LOST);
			history_add_full(type, artifact, player->depth, player->lev,
							 player->total_energy / 100, buf);
		} else {
			return FALSE;
		}
	}

	return TRUE;
}


/**
 * Convert all ARTIFACT_UNKNOWN history items to HIST_ARTIFACT_KNOWN.
 * Use only after player retirement/death for the final character dump.
 */
void history_unmask_unknown(void)
{
	size_t i = history_ctr;

	while (i--) {
		if (hist_has(history_list[i].type, HIST_ARTIFACT_UNKNOWN)) {
			hist_off(history_list[i].type, HIST_ARTIFACT_UNKNOWN);
			hist_on(history_list[i].type, HIST_ARTIFACT_KNOWN);
		}
	}
}

/**
 * Present a copy of the history fot UI use
 */
size_t history_get_list(struct history_info **list)
{
	*list = history_list;
	return history_ctr;
}
