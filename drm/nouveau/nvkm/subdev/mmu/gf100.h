#ifndef __GF100_MMU_H__
#define __GF100_MMU_H__
#define gf100_mmu(p) container_of((p), struct gf100_mmu, base)
#include "priv.h"

struct gf100_mmu {
	const struct gf100_mmu_func *func;
	struct nvkm_mmu base;
};

struct gf100_mmu_func {
	int (*uvmm)(struct gf100_mmu *, int, const struct nvkm_vmm_user **);
};

int gf100_mmu_new_(const struct gf100_mmu_func *, struct nvkm_device *, int,
		   struct nvkm_mmu **);
#endif
