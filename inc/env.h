/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ENV_H
#define JOS_INC_ENV_H
// inc stands for include directory. this directory contains all the header files
#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t envid_t;

// An environment ID 'envid_t' has three parts:
//
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's index in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
// stands for the current environment.

#define LOG2NENV		10 // 2^10 = 1024 is the maximum number of environments
#define NENV			(1 << LOG2NENV) 
// The maximum number of environments is 1024
// LOG2NENV is the number of bits needed to represent the number of environments
// NENV is the number of environments
#define ENVX(envid)		((envid) & (NENV - 1))
// ENVX(envid) is the index of the environment in the envs[] array

// Values of env_status in struct Env
enum {
	ENV_FREE = 0,
	ENV_DYING,
	ENV_RUNNABLE,
	ENV_RUNNING,
	ENV_NOT_RUNNABLE
};

// Special environment types
enum EnvType {
	ENV_TYPE_USER = 0,
};
// default environment type is user environment
// other types can be added here
struct Env {
	struct Trapframe env_tf;	// Saved registers 
	struct Env *env_link;		// Next free Env (on free list)
	envid_t env_id;			// Unique environment identifier 
	//is this the same as pid in linux? 
	//no, pid is process id, this is environment id
	//what is the difference between process and environment?
	//process is a program in execution, environment is a process in execution

	envid_t env_parent_id;		// env_id of this env's parent
	enum EnvType env_type;		// Indicates special system environments
	unsigned env_status;		// Status of the environment
	uint32_t env_runs;		// Number of times environment has run

	// Address space
	pde_t *env_pgdir;		// Kernel virtual address of page dir
	
};

#endif // !JOS_INC_ENV_H

