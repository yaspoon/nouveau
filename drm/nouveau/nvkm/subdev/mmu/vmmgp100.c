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

static void
gp100_vmm_join(struct gf100_vmm *vmm, struct nvkm_gpuobj *inst)
{
	u64 addr = nvkm_memory_addr(vmm->pd);

	nvkm_kmap(inst);
	nvkm_wo32(inst, 0x0200, lower_32_bits(addr) | 0x00000030);
	nvkm_wo32(inst, 0x0204, upper_32_bits(addr));
	nvkm_wo32(inst, 0x0208, lower_32_bits(vmm->base.limit));
	nvkm_wo32(inst, 0x020c, upper_32_bits(vmm->base.limit));
	nvkm_done(inst);
}

static const struct gf100_vmm_func
gp100_vmm = {
	.join = gp100_vmm_join,
};

static int
gp100_vmm_new(struct nvkm_mmu *mmu, u64 addr, u64 size, void *argv, u32 argc,
	      struct lock_class_key *key, struct nvkm_vmm **pvmm)
{
	return gf100_vmm_new_(&gp100_vmm, mmu, addr, size, argv, argc, key, pvmm);
}

const struct nvkm_vmm_user
gp100_vmm_user = {
	.ctor = gp100_vmm_new,
};
