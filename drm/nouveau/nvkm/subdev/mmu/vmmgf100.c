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
#include "vmmgf100.h"

#include <core/gpuobj.h>
#include <subdev/fb.h>

static void
gf100_vmm_part(struct nvkm_vmm *base, struct nvkm_gpuobj *inst)
{
	/*XXX: ideally we'd do this, but there's some chicken-and-egg
	 *     problems with the bootstrapped instmem vmm.
	 */
#if 0
	nvkm_kmap(inst);
	nvkm_wo32(inst, 0x0200, 0x00000000);
	nvkm_wo32(inst, 0x0204, 0x00000000);
	nvkm_wo32(inst, 0x0208, 0x00000000);
	nvkm_wo32(inst, 0x020c, 0x00000000);
	nvkm_done(inst);
#endif
}

static int
gf100_vmm_join(struct nvkm_vmm *base, struct nvkm_gpuobj *inst)
{
	struct gf100_vmm *vmm = gf100_vmm(base);
	u64  addr = nvkm_memory_addr(vmm->pd);
	u64 limit = vmm->base.limit;

	nvkm_kmap(inst);
	nvkm_wo32(inst, 0x0200, lower_32_bits(addr));
	nvkm_wo32(inst, 0x0204, upper_32_bits(addr));
	nvkm_wo32(inst, 0x0208, lower_32_bits(limit));
	nvkm_wo32(inst, 0x020c, upper_32_bits(limit));
	nvkm_done(inst);
	return 0;
}

static const struct nvkm_vmm_page *
gf100_vmm_page(struct nvkm_vmm *base)
{
	struct gf100_vmm *vmm = gf100_vmm(base);
	static const struct nvkm_vmm_page
	page_17[] = {
		{ 17, NVKM_VMM_PAGE_COMP },
		{ 12 },
		{}
	};

	switch (vmm->base.mmu->subdev.device->fb->page) {
	case 17: return page_17;
	default:
		WARN_ON(1);
		return NULL;
	}
}

static void *
gf100_vmm_dtor(struct nvkm_vmm *base)
{
	struct gf100_vmm *vmm = gf100_vmm(base);
	nvkm_memory_del(&vmm->pd);
	return vmm;
}

static const struct nvkm_vmm_func
gf100_vmm_ = {
	.dtor = gf100_vmm_dtor,
	.page = gf100_vmm_page,
	.join = gf100_vmm_join,
	.part = gf100_vmm_part,
};

int
gf100_vmm_new_(const struct gf100_vmm_func *func, struct nvkm_mmu *mmu,
	       u64 addr, u64 size, void *argv, u32 argc,
	       struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	struct gf100_vmm *vmm;
	int ret;

	if (argc != 0)
		return -ENOSYS;

	if (!(vmm = kzalloc(sizeof(*vmm), GFP_KERNEL)))
		return -ENOMEM;
	vmm->func = func;
	*pvmm = &vmm->base;

	ret = nvkm_vmm_ctor(&gf100_vmm_, mmu, 40, addr, size, key, &vmm->base);
	if (ret)
		return ret;

	ret = nvkm_memory_new(mmu->subdev.device, NVKM_MEM_TARGET_INST,
			      (vmm->base.lpde + 1) * 8, 0x1000, true,
			      &vmm->pd);
	if (ret)
		return ret;

	return 0;
}

static const struct gf100_vmm_func
gf100_vmm = {
};

static int
gf100_vmm_new(struct nvkm_mmu *mmu, u64 addr, u64 size, void *argv, u32 argc,
	      struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	return gf100_vmm_new_(&gf100_vmm, mmu, addr, size, argv, argc, key, pvmm);
}

const struct nvkm_vmm_user
gf100_vmm_user = {
	.ctor = gf100_vmm_new,
};
