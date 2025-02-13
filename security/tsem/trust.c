// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2023 Enjellic Systems Development, LLC
 * Author: Dr. Greg Wettstein <greg@enjellic.com>
 *
 * Implements management of a TPM trust root for the in kernel TMA.
 */

#include <linux/tpm.h>

#include "tsem.h"

static struct workqueue_struct *tpm_update_wq;

static u8 zero_aggregate[HASH_MAX_DIGESTSIZE];

static struct tpm_chip *tpm;

static struct tpm_digest *digests;

struct hardware_aggregate {
	struct list_head list;
	char *name;
	u8 value[HASH_MAX_DIGESTSIZE];
};

DEFINE_MUTEX(hardware_aggregate_mutex);
LIST_HEAD(hardware_aggregate_list);

static struct hardware_aggregate *find_aggregate(void)
{
	struct hardware_aggregate *aggregate;

	list_for_each_entry(aggregate, &hardware_aggregate_list, list) {
		if (!strcmp(aggregate->name,
			    tsem_context(current)->digestname))
			goto done;
	}
	aggregate = NULL;

 done:
	return aggregate;
}

static struct hardware_aggregate *add_aggregate(u8 *new_aggregate)
{
	struct hardware_aggregate *aggregate;

	aggregate = kzalloc(sizeof(*aggregate), GFP_KERNEL);
	if (!aggregate)
		return NULL;

	aggregate->name = kstrdup(tsem_context(current)->digestname,
				  GFP_KERNEL);
	if (!aggregate->name) {
		kfree(aggregate);
		return NULL;
	}
	memcpy(aggregate->value, new_aggregate, tsem_digestsize());

	list_add(&aggregate->list, &hardware_aggregate_list);

	return aggregate;
}

/**
 * tsem_trust_aggregate() - Return a pointer to the hardware aggregate.
 *
 * This function returns a pointer to the hardware aggregate encoded
 * with the hash function for the current modeling domain.
 *
 * Return: A pointer is returned to the hardware aggregate value that
 *	   has been cached.
 */
u8 *tsem_trust_aggregate(void)
{
	u8 aggregate[HASH_MAX_DIGESTSIZE], *retn = zero_aggregate;
	u16 size;
	unsigned int lp;
	struct tpm_digest pcr;
	struct hardware_aggregate *hw_aggregate;
	SHASH_DESC_ON_STACK(shash, tfm);

	if (!tpm)
		return retn;

	mutex_lock(&hardware_aggregate_mutex);

	hw_aggregate = find_aggregate();
	if (hw_aggregate) {
		retn = hw_aggregate->value;
		goto done;
	}

	shash->tfm = tsem_digest();
	if (crypto_shash_init(shash))
		goto done;

	if (tpm_is_tpm2(tpm))
		pcr.alg_id = TPM_ALG_SHA256;
	else
		pcr.alg_id = TPM_ALG_SHA1;
	memset(pcr.digest, '\0', TPM_MAX_DIGEST_SIZE);

	for (lp = 0; lp < tpm->nr_allocated_banks; lp++) {
		if (pcr.alg_id == tpm->allocated_banks[lp].alg_id) {
			size = tpm->allocated_banks[lp].digest_size;
			break;
		}
	}

	for (lp = 0; lp < 8; ++lp) {
		if (tpm_pcr_read(tpm, lp, &pcr))
			goto done;
		if (crypto_shash_update(shash, pcr.digest, size))
			goto done;
	}
	if (!crypto_shash_final(shash, aggregate)) {
		hw_aggregate = add_aggregate(aggregate);
		if (hw_aggregate)
			retn = hw_aggregate->value;
	}

 done:
	mutex_unlock(&hardware_aggregate_mutex);

	if (retn == zero_aggregate)
		pr_warn("tsem: Error generating platform aggregate\n");

	return retn;
}

static void tpm_update_worker(struct work_struct *work)
{
	int amt, bank, digestsize;
	struct tsem_event *ep;

	ep = container_of(work, struct tsem_event, work);
	digestsize = ep->digestsize;

	for (bank = 0; bank < tpm->nr_allocated_banks; bank++) {
		if (tpm->allocated_banks[bank].digest_size > digestsize) {
			amt = digestsize;
			memset(digests[bank].digest, '\0',
			       tpm->allocated_banks[bank].digest_size);
		} else
			amt = tpm->allocated_banks[bank].digest_size;
		memcpy(digests[bank].digest, ep->mapping, amt);
	}

	if (tpm_pcr_extend(tpm, CONFIG_SECURITY_TSEM_ROOT_MODEL_PCR,
			   digests))
		pr_warn("tsem: Failed TPM update.\n");

	tsem_event_put(ep);
}

/**
 * tsem_trust_add_point() - Add a measurement to the trust root.
 * @ep: A pointer to the security event description whose measurement
 *	is to be extended into the TPM.
 *
 * This function extends the platform configuration register being
 * used to document the hardware root of trust for internally modeled
 * domains with a security event coefficient value.
 *
 * Return: If the extension fails the error return value from the
 *	   TPM command is returned, otherwise a value of zero is
 *	   returned.
 */
int tsem_trust_add_event(struct tsem_event *ep)
{
	bool retn;

	if (!tpm)
		return 0;

	tsem_event_get(ep);
	ep->digestsize = tsem_digestsize();

	INIT_WORK(&ep->work, tpm_update_worker);
	retn = queue_work(tpm_update_wq, &ep->work);

	return 0;
}

static int __init trust_init(void)
{
	int retn = -EINVAL, lp;

	tpm = tpm_default_chip();
	if (!tpm)
		return retn;

	tpm_update_wq = alloc_ordered_workqueue("tsem_tpm", 0);
	if (IS_ERR(tpm_update_wq)) {
		retn = PTR_ERR(tpm_update_wq);
		goto done;
	}

	digests = kcalloc(tpm->nr_allocated_banks, sizeof(*digests), GFP_NOFS);
	if (!digests) {
		tpm = NULL;
		return retn;
	}
	for (lp = 0; lp < tpm->nr_allocated_banks; lp++)
		digests[lp].alg_id = tpm->allocated_banks[lp].alg_id;
	retn = 0;

 done:
	if (retn) {
		destroy_workqueue(tpm_update_wq);
		kfree(digests);
	}

	return retn;
}

device_initcall_sync(trust_init);
