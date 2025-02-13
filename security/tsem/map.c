// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2023 Enjellic Systems Development, LLC
 * Author: Dr. Greg Wettstein <greg@enjellic.com>
 *
 * This file implements mapping of events into security event points.
 */

#include "tsem.h"

static int get_COE_mapping(struct tsem_event *ep, u8 *mapping)
{
	int retn = 0, size;
	u8 *p;
	SHASH_DESC_ON_STACK(shash, tfm);

	shash->tfm = tsem_digest();
	retn = crypto_shash_init(shash);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.uid;
	size = sizeof(ep->COE.uid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.euid;
	size = sizeof(ep->COE.euid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.suid;
	size = sizeof(ep->COE.suid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.gid;
	size = sizeof(ep->COE.gid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.egid;
	size = sizeof(ep->COE.egid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.sgid;
	size = sizeof(ep->COE.sgid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.fsuid;
	size = sizeof(ep->COE.fsuid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.fsgid;
	size = sizeof(ep->COE.fsgid);
	retn = crypto_shash_update(shash, p, size);
	if (retn)
		goto done;

	p = (u8 *) &ep->COE.capeff;
	size = sizeof(ep->COE.capeff);
	retn = crypto_shash_finup(shash, p, size, mapping);

 done:
	return retn;
}

static int get_cell_mapping(struct tsem_event *ep, u8 *mapping)
{
	int retn = 0, size;
	u8 *p;
	struct sockaddr_in *ipv4;
	struct sockaddr_in6 *ipv6;
	struct tsem_mmap_file_args *mm_args = &ep->CELL.mmap_file;
	struct tsem_socket_connect_args *scp = &ep->CELL.socket_connect;
	struct tsem_socket_accept_args *sap = &ep->CELL.socket_accept;
	SHASH_DESC_ON_STACK(shash, tfm);

	shash->tfm = tsem_digest();
	retn = crypto_shash_init(shash);
	if (retn)
		goto done;

	if (ep->event == TSEM_MMAP_FILE) {
		p = (u8 *) &mm_args->reqprot;
		size = sizeof(mm_args->reqprot);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &mm_args->prot;
		size = sizeof(mm_args->prot);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &mm_args->flags;
		size = sizeof(mm_args->flags);
		if (!mm_args->file) {
			retn = crypto_shash_finup(shash, p, size, mapping);
			goto done;
		}

		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;
	}

	switch (ep->event) {
	case TSEM_FILE_OPEN:
	case TSEM_MMAP_FILE:
	case TSEM_BPRM_SET_CREDS:
		p = (u8 *) &ep->file.flags;
		size = sizeof(ep->file.flags);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.uid;
		size = sizeof(ep->file.uid);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.gid;
		size = sizeof(ep->file.gid);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.mode;
		size = sizeof(ep->file.mode);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.name_length;
		size = sizeof(ep->file.name_length);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.name;
		size = tsem_digestsize();
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.s_magic;
		size = sizeof(ep->file.s_magic);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.s_id;
		size = sizeof(ep->file.s_id);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.s_uuid;
		size = sizeof(ep->file.s_uuid);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->file.digest;
		size = tsem_digestsize();
		retn = crypto_shash_finup(shash, p, size, mapping);
		break;

	case TSEM_SOCKET_CREATE:
		p = (u8 *) &ep->CELL.socket_create.family;
		size = sizeof(ep->CELL.socket_create.family);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->CELL.socket_create.type;
		size = sizeof(ep->CELL.socket_create.type);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->CELL.socket_create.protocol;
		size = sizeof(ep->CELL.socket_create.protocol);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->CELL.socket_create.kern;
		size = sizeof(ep->CELL.socket_create.kern);
		retn = crypto_shash_finup(shash, p, size, mapping);
		if (retn)
			goto done;
		break;

	case TSEM_SOCKET_CONNECT:
	case TSEM_SOCKET_BIND:
		p = (u8 *) &scp->family;
		size = sizeof(scp->family);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		switch (scp->family) {
		case AF_INET:
			ipv4 = (struct sockaddr_in *) &scp->u.ipv4;
			p = (u8 *) &ipv4->sin_port;
			size = sizeof(ipv4->sin_port);
			retn = crypto_shash_update(shash, p, size);
			if (retn)
				goto done;

			p = (u8 *) &ipv4->sin_addr.s_addr;
			size = sizeof(ipv4->sin_addr.s_addr);
			retn = crypto_shash_finup(shash, p, size, mapping);
			break;

		case AF_INET6:
			ipv6 = (struct sockaddr_in6 *) &scp->u.ipv6;
			p = (u8 *) &ipv6->sin6_port;
			size = sizeof(ipv6->sin6_port);
			retn = crypto_shash_update(shash, p, size);
			if (retn)
				goto done;

			p = (u8 *) ipv6->sin6_addr.in6_u.u6_addr8;
			size = sizeof(ipv6->sin6_addr.in6_u.u6_addr8);
			retn = crypto_shash_update(shash, p, size);
			if (retn)
				goto done;

			p = (u8 *) &ipv6->sin6_flowinfo;
			size = sizeof(ipv6->sin6_flowinfo);
			retn = crypto_shash_update(shash, p, size);
			if (retn)
				goto done;

			p = (u8 *) &ipv6->sin6_scope_id;
			size = sizeof(ipv6->sin6_scope_id);
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;

		case AF_UNIX:
			p = scp->u.path;
			size = strlen(scp->u.path);
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;

		default:
			p = (u8 *) scp->u.mapping;
			size = tsem_digestsize();
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;
		}
		break;

	case TSEM_SOCKET_ACCEPT:
		p = (u8 *) &sap->family;
		size = sizeof(sap->family);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &sap->type;
		size = sizeof(sap->type);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &sap->port;
		size = sizeof(sap->port);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		switch (sap->family) {
		case AF_INET:
			p = (u8 *) &sap->u.ipv4;
			size = sizeof(sap->u.ipv4);
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;

		case AF_INET6:
			p = (u8 *) sap->u.ipv6.in6_u.u6_addr8;
			size = sizeof(sap->u.ipv6.in6_u.u6_addr8);
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;

		case AF_UNIX:
			p = sap->u.path;
			size = strlen(sap->u.path);
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;

		default:
			p = sap->u.mapping;
			size = tsem_digestsize();
			retn = crypto_shash_finup(shash, p, size, mapping);
			if (retn)
				goto done;
			break;
		}
		break;

	case TSEM_TASK_KILL:
		p = (u8 *) &ep->CELL.task_kill.cross_model;
		size = sizeof(ep->CELL.task_kill.cross_model);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->CELL.task_kill.signal;
		size = sizeof(ep->CELL.task_kill.signal);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = (u8 *) &ep->CELL.task_kill.target;
		size = sizeof(ep->CELL.task_kill.target);
		retn = crypto_shash_finup(shash, p, size, mapping);
		if (retn)
			goto done;
		break;

	case TSEM_GENERIC_EVENT:
		p = (u8 *) tsem_names[ep->CELL.event_type];
		size = strlen(tsem_names[ep->CELL.event_type]);
		retn = crypto_shash_update(shash, p, size);
		if (retn)
			goto done;

		p = tsem_context(current)->zero_digest;
		size = tsem_digestsize();
		retn = crypto_shash_finup(shash, p, size, mapping);
		if (retn)
			goto done;
		break;

	default:
		break;
	}

 done:
	return retn;
}

static int get_event_mapping(int event, u8 *task_id, u8 *COE_id, u8 *cell_id,
			     u8 *mapping)
{
	int retn = 0;
	u32 event_id = (u32) event;
	SHASH_DESC_ON_STACK(shash, tfm);

	shash->tfm = tsem_digest();
	retn = crypto_shash_init(shash);
	if (retn)
		goto done;

	retn = crypto_shash_update(shash, tsem_names[event_id],
				   strlen(tsem_names[event_id]));
	if (retn)
		goto done;
	if (task_id) {
		retn = crypto_shash_update(shash, task_id, tsem_digestsize());
		if (retn)
			goto done;
	}
	retn = crypto_shash_update(shash, COE_id, tsem_digestsize());
	if (retn)
		goto done;
	retn = crypto_shash_finup(shash, cell_id, tsem_digestsize(), mapping);

 done:
	return retn;
}

static int map_event(enum tsem_event_type event, struct tsem_event *ep,
		     u8 *task_id, u8 *event_mapping)
{
	int retn;
	u8 COE_mapping[HASH_MAX_DIGESTSIZE];
	u8 cell_mapping[HASH_MAX_DIGESTSIZE];

	retn = get_COE_mapping(ep, COE_mapping);
	if (retn)
		goto done;

	retn = get_cell_mapping(ep, cell_mapping);
	if (retn)
		goto done;

	retn = get_event_mapping(event, task_id, COE_mapping, cell_mapping,
				 event_mapping);
 done:
	return retn;
}

/**
 * tsem_map_task() - Create the task identity description structure.
 * @file: A pointer to the file structure defining the executable.
 * @task_id: Pointer to the buffer that the task id will be copied to.
 *
 * This function creates the security event state point that will be used
 * as the task identifier for the generation of security state points
 * that are created by the process that task identifier is assigned to.
 *
 * Return: This function returns 0 if the mapping was successfully
 *	   created and an error value otherwise.
 */
int tsem_map_task(struct file *file, u8 *task_id)
{
	int retn = 0;
	u8 null_taskid[HASH_MAX_DIGESTSIZE];
	struct tsem_event *ep;
	struct tsem_event_parameters params;

	params.u.file = file;
	ep = tsem_event_init(TSEM_BPRM_SET_CREDS, &params, false);
	if (IS_ERR(ep)) {
		retn = PTR_ERR(ep);
		ep = NULL;
		goto done;
	}

	memset(null_taskid, '\0', tsem_digestsize());
	retn = map_event(TSEM_BPRM_SET_CREDS, ep, null_taskid, task_id);
	tsem_event_put(ep);

 done:
	return retn;
}

/**
 * tsem_map_event() - Create a security event mapping.
 * @event: The number of the event to be mapped.
 * @params: A pointer to the structure containing the event description
 *	    parameters.
 *
 * This function creates a structure to describe a security event
 * and maps the event into a security state coefficient.
 *
 * Return: On success the function returns a pointer to the tsem_event
 *	   structure that describes the event.  If an error is encountered
 *	   an error return value is encoded in the pointer.
 */
struct tsem_event *tsem_map_event(enum tsem_event_type event,
				  struct tsem_event_parameters *params)
{
	int retn = 0;
	struct tsem_event *ep;
	struct tsem_task *task = tsem_task(current);

	ep = tsem_event_init(event, params, false);
	if (IS_ERR(ep))
		goto done;

	if (task->context->external)
		goto done;

	retn = map_event(event, ep, task->task_id, ep->mapping);
	if (retn) {
		tsem_event_put(ep);
		ep = ERR_PTR(retn);
	}

 done:
	return ep;
}


/**
 * tsem_map_event_locked() - Create a security event mapping while atomic.
 * @event: The number of the event to be mapped.
 * @params: A pointer to the structure containing the event description
 *	    parameters.
 *
 * This function creates a structure to describe a security event
 * and maps the event into a security state coefficient.
 *
 * Return: On success the function returns a pointer to the tsem_event
 *	   structure that describes the event.  If an error is encountered
 *	   an error return value is encoded in the pointer.
 */
struct tsem_event *tsem_map_event_locked(enum tsem_event_type event,
					 struct tsem_event_parameters *params)
{
	int retn = 0;
	struct tsem_event *ep;
	struct tsem_task *task = tsem_task(current);

	ep = tsem_event_init(event, params, true);
	if (IS_ERR(ep))
		goto done;

	if (task->context->external)
		goto done;

	retn = map_event(event, ep, task->task_id, ep->mapping);
	if (retn) {
		tsem_event_put(ep);
		ep = ERR_PTR(retn);
	}

 done:
	return ep;
}
