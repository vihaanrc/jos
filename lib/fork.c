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

	addr = ROUNDDOWN(addr, PGSIZE);

    // Only handle write faults to COW pages
    if (!(err & FEC_WR)) {
        // Not a write - let's determine what type of fault it is
        if (!(uvpt[PGNUM(addr)] & PTE_P))
            panic("pgfault: page not present");
        else
            panic("pgfault: not a write");
    }
    
    if (!(uvpt[PGNUM(addr)] & PTE_COW)) {
        // Not a COW page - might be a write to a read-only page
        panic("pgfault: not a copy-on-write page");
    }

    // Now handle the COW fault
    if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_P | PTE_W | PTE_U)) < 0) {
        panic("pgfault: sys_page_alloc failed: %e", r);
    }

    // Copy the data
    memmove((void *)PFTEMP, addr, PGSIZE);
    
    // Map the new page at the fault address with write permission
    if ((r = sys_page_map(0, (void *)PFTEMP, 0, addr, PTE_P | PTE_W | PTE_U)) < 0) {
        panic("pgfault: sys_page_map failed: %e", r);
    }
    
    // Unmap the temporary page
    if ((r = sys_page_unmap(0, (void *)PFTEMP)) < 0) {
        panic("pgfault: sys_page_unmap failed: %e", r);
    }
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

	// LAB 4: Your code here.

	void * addr = (void *)(pn * PGSIZE);
	uint32_t pte = uvpt[pn];


	if (pte & PTE_SHARE) {
		r = sys_page_map(0, addr, envid, addr, pte & PTE_SYSCALL);
		if (r < 0)
			return r;
		return 0;
	}
	// check if the page is writable or copy-on-write
	if ((pte & PTE_W) || (pte & PTE_COW)) {
		// create a copy-on-write mapping
		if ((r = sys_page_map(0, addr, envid, addr, PTE_P | PTE_COW | PTE_U)) < 0) {
			return r;
		}
		if ((r = sys_page_map(0, addr, 0, addr, PTE_P | PTE_COW | PTE_U)) < 0) {
			return r;
		}

	} else {
		// create a normal mapping
		if ((r = sys_page_map(0, addr, envid, addr, PTE_P | PTE_U)) < 0) {
			return r;
		}
	}
	// panic("duppage not implemented");
	return 0;
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
	envid_t envid = sys_exofork();
	if (envid < 0) {
		return envid;
	}

	if (envid == 0) { 
		// child process
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	set_pgfault_handler(pgfault);

	for (uintptr_t addr = 0; addr < UTOP; addr += PGSIZE) {
		
		unsigned pn = PGNUM(addr);
		if (!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[pn] & PTE_P)) { //if the page is not present
			continue;
		} 

		if (addr >= UXSTACKTOP - PGSIZE && addr < UXSTACKTOP) { // skip the user exception stack
			continue;
		}

		if (duppage(envid, pn) <0)
			panic("fork: duppage failed");
	}
	 if ((sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_W | PTE_U)) < 0) {
		panic("fork: sys_page_alloc failed");

	}

	if ((sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)) < 0) {
		panic("fork: sys_env_set_pgfault_upcall failed");
	}

	if ((sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		panic("fork: sys_env_set_status failed");
	}
	return envid;
	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}