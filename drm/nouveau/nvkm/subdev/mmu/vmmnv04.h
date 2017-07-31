#ifndef __NV04_VMM_H__
#define __NV04_VMM_H__
#define nv04_vmm(p) container_of((p), struct nv04_vmm, base)
#include "vmm.h"

struct nv04_vmm {
	struct nvkm_vmm base;
};

int nv04_vmm_new_(const struct nvkm_vmm_func *, struct nvkm_mmu *, int,
		  u64, u64, struct lock_class_key *, struct nv04_vmm **);
void *nv04_vmm_dtor(struct nvkm_vmm *);
const struct nvkm_vmm_page *nv04_vmm_page(struct nvkm_vmm *);

extern const struct nvkm_vmm_user nv04_vmm_user;
extern const struct nvkm_vmm_user nv41_vmm_user;
#endif
