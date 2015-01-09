// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
        if ((err & FEC_WR) == 0) { panic("pgfault access is not write"); }

        if ((uvpd[PDX(addr)] & PTE_P) == 0 || (uvpt[PGNUM(addr)] & PTE_COW) == 0) {
           panic("The pgfault can not be handled due to access limit\n");
        }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
        // PTE_COW is not necessary since it is the state to trig pgfault handling
        if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_U|PTE_W|PTE_P)) < 0) {
           panic("sys_page_alloc(): allocation failed\n");
        }

        uint32_t alignaddr = ROUNDDOWN((uint32_t)addr, PGSIZE);
        // memmove (dst, src)
        memmove((void *)PFTEMP, (void *)alignaddr, PGSIZE);

        // what do we do with the old page? remember that page_insert will
        // remove the old page.
        if ((r = sys_page_map(0, (void *)PFTEMP, 0, (void *)alignaddr, PTE_W|PTE_U|PTE_P)) < 0) 
            panic("Can not map new page to PFTEMP\n");
   
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
        int r;
        void *addr = (void *) (pn * PGSIZE);
        int pte = uvpt[pn];
        int perm = pte & PTE_SYSCALL;

        // LAB 4: Your code here.
        if (!(perm & PTE_SHARE) && ((perm & PTE_W) || (perm & PTE_COW))) {
            perm &= ~PTE_W;  // set PTE_W to PTE_COW
            perm |= PTE_COW;
            r = sys_page_map(0, addr, envid, addr, perm);
            if (r < 0) { return r; }
            // self mapping
            // because sys_page_map will alloc a new page and invoke page_insert
            // because there is already a page mapped. it will be replaced with the new page
            // however, the old page will not be freed since it still has a ref count from the child copy
            return sys_page_map(0, addr, 0, addr, perm);
        }
        return sys_page_map(0, addr, envid, addr, perm);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
        struct Env *childenv;
        int envid;
        int addr;
        int r;

        // also do memory allocation for parent UXSTACK.
        set_pgfault_handler(pgfault);

        // we are the child
        envid = sys_exofork();
        if (envid < 0) { panic("sys_exofork: %e", envid); }
        if (envid == 0) {
            thisenv = &envs[ENVX(sys_getenvid())];
            return 0;
        }

        // We are the parent
        for (addr = UTEXT; addr < UTOP - PGSIZE; addr+=PGSIZE) {
            if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_U)) {
                if ((r = duppage(envid, PGNUM(addr))) < 0) {
                    panic("duppage(): duppage failed\n");
                }  
            }                   
        }

        // // Allocate a new exception stack.
        r = sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P|PTE_W|PTE_U);
        if (r < 0) { panic("env page allocation failed\n"); }

        // get the child env
        extern void _pgfault_upcall(void);
        sys_env_set_pgfault_upcall(envid, (void *) _pgfault_upcall);
      
        if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
                panic("sys_env_set_status: %e", r);
        }

        return envid;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
}

