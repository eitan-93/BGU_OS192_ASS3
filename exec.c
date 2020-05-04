#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

void move_to_backup(int * backup_file_page, int *backup_insertion, int * backupIndexes, int * mem_page_backup){
  struct proc *currproc = myproc();
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; ++i) {
  //   /**  backup proc pagesDS before clean**/
  //   backup_file_page[i] = currproc->file_page[i];
  //   backup_insertion[i] = currproc->insertion_array[i];

    /**  clean proc pagesDS before exec **/
    currproc->file_page[i] = -1;
    currproc->insertion_array[i] = -1;
    /**  backup pages queue **/
    //mem_page_backup[i] = currproc->memory_pages[i];
    /**  clear pages queue **/
    currproc->memory_pages[i] = -1;
  }
  // /**  backup proc page counters before clean **/
  backupIndexes[0] = currproc->phs_mem_ctr;
  backupIndexes[1] = currproc->swap_file_ctr;
  backupIndexes[2] = currproc->insertion_counter;
  backupIndexes[3] = currproc->total_of_alloc;
  backupIndexes[4] = currproc->num_of_pgflts;
  backupIndexes[5] = currproc->total_page_out_counter;


  /**  clean proc page counters before exec **/
  currproc->phs_mem_ctr = 0;
  currproc->swap_file_ctr = 0;
  currproc->insertion_counter = 0;
  currproc->total_of_alloc = 0;
  currproc->num_of_pgflts = 0;
  currproc->total_page_out_counter = 0;

}

void restore_backup(int * backup_file_page, int *backup_insertion, int * backupIndexes, int * mem_page_backup){
  struct proc *currproc = myproc();
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; ++i) {
    /**  restore proc pagesDS after failed exec **/
    currproc->file_page[i] = backup_file_page[i];
    currproc->insertion_array[i] = backup_insertion[i];
    currproc->memory_pages[i] = mem_page_backup[i];
  }


  // /**  restore proc page counters after failed exec **/
  currproc->phs_mem_ctr = backupIndexes[0] ;
  currproc->swap_file_ctr = backupIndexes[1];
  currproc->insertion_counter = backupIndexes[2];
  currproc->total_of_alloc = backupIndexes[3];
  currproc->num_of_pgflts = backupIndexes[4];
  currproc->total_page_out_counter = backupIndexes[5];

}
int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;
  #ifndef NONE
      // curproc->phs_mem_ctr = 0;
      // // curproc->swap_file_ctr = 0;
      // curproc->insertion_counter = 0;
      // // curproc->total_of_alloc = 0;
      // // curproc->num_of_pgflts = 0;
      // // curproc->total_page_out_counter = 0;
      // for (i = 0; i < MAX_PSYC_PAGES; ++i) {
      //   // curproc->file_page[i] = -1;
      //   curproc->memory_pages[i] = -1;
      //   curproc->insertion_array[i] = -1;
      // }
    
    int backup_file_page[MAX_PSYC_PAGES];
    int backupIndexes[6];
    int mem_page_backup[MAX_PSYC_PAGES];
    int backup_insertion[MAX_PSYC_PAGES];
    if(curproc->pid > 2){
    move_to_backup(backup_file_page, backup_insertion, backupIndexes, mem_page_backup);
    }
  #endif

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;

  #ifndef NONE
    if(curproc-> pid > 2){
      removeSwapFile(curproc);
      createSwapFile(curproc);
    }
  #endif



  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  #ifndef NONE
    // cprintf("exec fail\n");
    restore_backup(backup_file_page, backup_insertion, backupIndexes,mem_page_backup);
  #endif

  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
