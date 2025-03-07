// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
//inlcude env.c
#include <kern/env.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

static int single_stepping =0;
struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// Single-step execution (one instruction)
int
mon_single_step(int argc, char **argv, struct Trapframe *tf)
{
    if (tf == NULL) {
        cprintf("No trap frame to single-step from.\n");
        return 0;
    }
    
    // Set trap flag in EFLAGS to enable single-stepping
    tf->tf_eflags |= FL_TF;

	env_pop_tf(tf);
	
    
    // Return 1 to indicate we want to exit the monitor
    return 0;
}

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "test", "test print commands for exercise 7", mon_test },
	{ "help", "Display this list of commands", mon_help },
	{ "hidden", "Run hidden test cases", exec_hidden_cases},
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the current backtrace", mon_backtrace},
	{ "si", "Execute a single instruction", mon_single_step },

	
};

/***** Implementations of basic kernel monitor commands *****/
int 
mon_test(int argc, char **argv, struct Trapframe *tf) {
	int x = 1, y = 3, z = 4;
    cprintf("x %d, y %x, z %d\n", x, y, z);
	return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])
	cprintf("Stack backtrace:\n");
	uint32_t*ebp = (uint32_t *)read_ebp();
	//until ebp is 0
	while (ebp != 0x0) {
		
		uint32_t eip = ebp[1];
		uint32_t arg1 = ebp[2];
		uint32_t arg2 = ebp[3];
		uint32_t arg3 = ebp[4];
		uint32_t arg4 = ebp[5];
		uint32_t arg5 = ebp[6];
		//print the backtrace
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, eip, arg1, arg2, arg3, arg4, arg5);
		
		struct Eipdebuginfo info;
		debuginfo_eip(eip, &info);
		
		cprintf("\t%s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
		//get the next ebp
		ebp = (uint32_t *)ebp[0];
	}

	return 0;
}



int exec_hidden_cases(int argc, char **argv, struct Trapframe *tf) {
	//hidden_test_cases();
	return 0;
}
/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);
	if (tf && single_stepping) {
        cprintf("Single-stepping at eip=%08x\n", tf->tf_eip);
	}
	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
