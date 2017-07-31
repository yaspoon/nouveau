#ifndef __GF100_VMM_H__
#define __GF100_VMM_H__
#define gf100_vmm(p) container_of((p), struct gf100_vmm, base)
#include "vmm.h"

struct gf100_vmm {
	const struct gf100_vmm_func *func;
	struct nvkm_vmm base;

	struct nvkm_memory *pd;
};

struct gf100_vmm_func {
};

int gf100_vmm_new_(const struct gf100_vmm_func *, struct nvkm_mmu *,
		   u64, u64, void *, u32, struct lock_class_key *,
		   struct nvkm_vmm **);

extern const struct nvkm_vmm_user gf100_vmm_user;
extern const struct nvkm_vmm_user gp100_vmm_user;
#endif
