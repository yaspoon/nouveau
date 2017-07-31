#ifndef __NV50_MMU_H__
#define __NV50_MMU_H__
#define nv50_mmu(p) container_of((p), struct nv50_mmu, base)
#include "priv.h"

struct nv50_mmu {
	const struct nv50_mmu_func *func;
	struct nvkm_mmu base;
};

struct nv50_mmu_func {
	const struct nvkm_vmm_user *uvmm;
};

int nv50_mmu_new_(const struct nv50_mmu_func *, struct nvkm_device *, int,
		  struct nvkm_mmu **);
#endif
