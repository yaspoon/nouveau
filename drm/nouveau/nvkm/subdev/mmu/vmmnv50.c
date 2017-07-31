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
#include "vmmnv50.h"

static void
nv50_vmm_part(struct nvkm_vmm *base, struct nvkm_gpuobj *inst)
{
	struct nv50_vmm *vmm = nv50_vmm(base);
	struct nv50_vmm_join *join;

	list_for_each_entry(join, &vmm->join, head) {
		if (join->inst == inst) {
			list_del(&join->head);
			kfree(join);
			return;
		}
	}

	WARN_ON(1);
}

static int
nv50_vmm_join(struct nvkm_vmm *base, struct nvkm_gpuobj *inst)
{
	struct nv50_vmm *vmm = nv50_vmm(base);
	struct nv50_vmm_join *join;

	if (!(join = kzalloc(sizeof(*join), GFP_KERNEL)))
		return -ENOMEM;

	join->inst = inst;
	list_add_tail(&join->head, &vmm->join);
	return 0;
}

static const struct nvkm_vmm_page *
nv50_vmm_page(struct nvkm_vmm *base)
{
	static const struct nvkm_vmm_page
	page[] = {
		{ 16, NVKM_VMM_PAGE_COMP },
		{ 12 },
		{}
	};
	return page;
}

static void *
nv50_vmm_dtor(struct nvkm_vmm *base)
{
	struct nv50_vmm *vmm = nv50_vmm(base);
	return vmm;
}

static const struct nvkm_vmm_func
nv50_vmm_ = {
	.dtor = nv50_vmm_dtor,
	.page = nv50_vmm_page,
	.page_block = 1 << 29,
	.join = nv50_vmm_join,
	.part = nv50_vmm_part,
};

int
nv50_vmm_new_(const struct nv50_vmm_func *func, struct nvkm_mmu *mmu,
	      u64 addr, u64 size, void *argv, u32 argc,
	      struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	struct nv50_vmm *vmm;
	int ret;

	if (argc != 0)
		return -ENOSYS;

	if (!(vmm = kzalloc(sizeof(*vmm), GFP_KERNEL)))
		return -ENOMEM;
	vmm->func = func;
	INIT_LIST_HEAD(&vmm->join);
	*pvmm = &vmm->base;

	ret = nvkm_vmm_ctor(&nv50_vmm_, mmu, 40, addr, size, key, &vmm->base);
	if (ret)
		return ret;

	return 0;
}

static const struct nv50_vmm_func
nv50_vmm = {
	.pd_offset = 0x1400,
};

static int
nv50_vmm_new(struct nvkm_mmu *mmu, u64 addr, u64 size, void *argv, u32 argc,
	     struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	return nv50_vmm_new_(&nv50_vmm, mmu, addr, size, argv, argc, key, pvmm);
}

const struct nvkm_vmm_user
nv50_vmm_user = {
	.ctor = nv50_vmm_new,
};
