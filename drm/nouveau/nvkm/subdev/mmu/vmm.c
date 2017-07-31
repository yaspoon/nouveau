/*
 * Copyright 2017 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "vmm.h"

void *
nvkm_vmm_dtor(struct nvkm_vmm *vmm)
{
	void *data = vmm;
	if (vmm->func)
		data = vmm->func->dtor(vmm);
	nvkm_mm_fini(&vmm->mm);
	vfree(vmm->pgt);
	return data;
}

int
nvkm_vmm_ctor(const struct nvkm_vmm_func *func, struct nvkm_mmu *mmu,
	      int vma_bits, u64 addr, u64 size, struct lock_class_key *key,
	      struct nvkm_vmm *vmm)
{
	static struct lock_class_key _key;
	int ret;

	vmm->func = func;
	vmm->mmu = mmu;
	kref_init(&vmm->kref);

	vmm->start = addr;
	vmm->limit = size ? (addr + size) : (1ULL << vma_bits);;
	if (vmm->start > vmm->limit || --vmm->limit >= 1ULL << vma_bits)
		return -EINVAL;

	__mutex_init(&vmm->mutex, "&vmm->mutex", key ? key : &_key);
	kref_init(&vmm->refcount);
	vmm->fpde = vmm->start >> (mmu->func->pgt_bits + 12);
	vmm->lpde = vmm->limit >> (mmu->func->pgt_bits + 12);

	vmm->pgt = vzalloc((vmm->lpde - vmm->fpde + 1) * sizeof(*vmm->pgt));
	if (!vmm->pgt)
		return -ENOMEM;

	ret = nvkm_mm_init(&vmm->mm, vmm->start >> 12,
			   (vmm->limit - vmm->start + 1) >> 12,
			   func->page_block ? func->page_block >> 12 : 1);
	if (ret) {
		vfree(vmm->pgt);
		return ret;
	}

	return 0;
}

static void
nvkm_vmm_del(struct kref *kref)
{
	struct nvkm_vmm *vmm = container_of(kref, typeof(*vmm), kref);
	vmm = nvkm_vmm_dtor(vmm);
	kfree(vmm);
}

void
nvkm_vmm_unref(struct nvkm_vmm **pvmm)
{
	struct nvkm_vmm *vmm = *pvmm;
	if (vmm) {
		kref_put(&vmm->kref, nvkm_vmm_del);
		*pvmm = NULL;
	}
}

struct nvkm_vmm *
nvkm_vmm_ref(struct nvkm_vmm *vmm)
{
	if (vmm)
		kref_get(&vmm->kref);
	return vmm;
}

int
nvkm_vmm_new(struct nvkm_mmu *mmu, u64 addr, u64 size, void *argv, u32 argc,
	     struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	const struct nvkm_vmm_user *uvmm = NULL;
	struct nvkm_vmm *vmm = NULL;
	int ret;
	mmu->func->uvmm(mmu, 0, &uvmm);
	ret = uvmm->ctor(mmu, addr, size, argv, argc, key, &vmm);
	if (ret)
		nvkm_vmm_unref(&vmm);
	*pvmm = vmm;
	return ret;
}
