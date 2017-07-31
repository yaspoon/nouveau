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
#include "vmmnv04.h"
#include "nv04.h"

void *
nv04_vmm_dtor(struct nvkm_vmm *base)
{
	struct nv04_vmm *vmm = nv04_vmm(base);
	return vmm;
}

int
nv04_vmm_new_(const struct nvkm_vmm_func *func, struct nvkm_mmu *mmu,
	      int vma_bits, u64 addr, u64 size, struct lock_class_key *key,
	      struct nv04_vmm **pvmm)
{
	struct nv04_vmm *vmm;

	if (!(vmm = *pvmm = kzalloc(sizeof(*vmm), GFP_KERNEL)))
		return -ENOMEM;

	return nvkm_vmm_ctor(func, mmu, vma_bits, addr, size, key, &vmm->base);
}

const struct nvkm_vmm_page *
nv04_vmm_page(struct nvkm_vmm *base)
{
	static const struct nvkm_vmm_page page[] = {
		{ 12 },
		{}
	};
	return page;
}

static const struct nvkm_vmm_func
nv04_vmm = {
	.dtor = nv04_vmm_dtor,
	.page = nv04_vmm_page,
};

static int
nv04_vmm_new(struct nvkm_mmu *mmu, u64 addr, u64 size, void *argv, u32 argc,
	     struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	struct nv04_vmm *vmm;
	int ret;

	if (argc != 0)
		return -ENOSYS;

	ret = nv04_vmm_new_(&nv04_vmm, mmu, 27, addr, size, key, &vmm);
	*pvmm = vmm ? &vmm->base : NULL;
	if (ret)
		return ret;

	return 0;
}

const struct nvkm_vmm_user
nv04_vmm_user = {
	.ctor = nv04_vmm_new,
};
