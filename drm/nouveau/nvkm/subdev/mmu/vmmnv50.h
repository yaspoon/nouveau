#ifndef __NV50_VMM_H__
#define __NV50_VMM_H__
#define nv50_vmm(p) container_of((p), struct nv50_vmm, base)
#include "vmm.h"

struct nv50_vmm {
	const struct nv50_vmm_func *func;
	struct nvkm_vmm base;

	struct list_head join;
};

struct nv50_vmm_func {
	u16 pd_offset;
};

struct nv50_vmm_join {
	struct list_head head;
	struct nvkm_gpuobj *inst;
};

int nv50_vmm_new_(const struct nv50_vmm_func *, struct nvkm_mmu *,
		  u64, u64, void *, u32, struct lock_class_key *,
		  struct nvkm_vmm **);

extern const struct nvkm_vmm_user nv50_vmm_user;
extern const struct nvkm_vmm_user g84_vmm_user;
#endif
