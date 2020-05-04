#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size; 
    bp->s.ptr = p->s.ptr->s.ptr;
  } else{
    bp->s.ptr = p->s.ptr;
  }
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else{
    p->s.ptr = bp;
  }
  freep = p;
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}


void*
pmalloc(void)
{
  Header *p, *prevp, *hdp, *va;
  //open("stdout", 1);
  //sleep(20);
  //printf(1,"pmalloc---------------------------------------------------------------------------------------------------------------------------------------\n");
  //sleep(10000);
  uint nunits;
  nunits = (PGSIZE + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }

  //printf(1,"freep is %d\n",freep);

  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    //printf(1,"curr p %d freep %d\n",p,freep);
    if(p->s.size >= nunits ){
      //printf(1,"found a p = %d\n",p);
      va = (Header *)PGROUNDUP((uint)(p+1));
      va -= 1;
      uint search_end = p->s.size - nunits;
      search_end += ((uint)p);
      if(p->s.size == nunits && ((uint)(p+1))%PGSIZE == 0){
        //printf(1,"exact and aligned\n");
        va = p;
        prevp->s.ptr = p->s.ptr;
      }
      if(p->s.size > nunits){

        if(((uint)(p+1))%PGSIZE == 0 ){
          //printf(1,"bigger and aligned\n");
          hdp = p + nunits;
          
          hdp->s.size = p->s.size - nunits;
          hdp->s.ptr = p->s.ptr;
          prevp->s.ptr = hdp;
          p->s.size = nunits;
          va = p;
        }
          
          
        if(((uint)(p+1))%PGSIZE != 0){
          //printf(1,"bigger and not aligned\n");
          if ((uint)(va) < search_end){
            // printf(1,"before search end\n");
            /*hdp = va + nunits;
            if(hdp < p + p->s.size){
              p->s.ptr = hdp;
              hdp->s.ptr = p->s.ptr;
              hdp->s.size = p->s.size - (((uint)hdp)  - ((uint)p)) - 1;
            //prevp->s.ptr = hdp;
            }*/
            
            //if(va > p)
            //    printf(1,"va is bigger address\n");
            hdp = va + nunits;
            hdp->s.ptr = p->s.ptr;
            hdp->s.size = p + p->s.size - hdp;
            p->s.ptr = hdp;
            p->s.size = va - p;
            // if ((int)hdp->s.size < 0)
            //    printf(1,"p.size is %d\n",p->s.size);
            va->s.size = nunits;
            //prevp->s.ptr = p->s.ptr;
          }
          if ((uint)(va) == search_end){
            // printf(1,"at search end\n");
            //p->s.ptr = va;
            p->s.size = p->s.size - nunits;
            va->s.size = nunits;
            //va->s.str = p->s.str;
            //prevp->s.ptr = p->s.ptr;
          }
          
          if ((uint)(va) > search_end){
            // printf(1,"loopingggggggggggggggggggggggg\n");
            if (p == freep){
              if((p = morecore(nunits)) == 0)
                return 0;
            }
            continue;
          }
          //printf(1,"times we get here?\n",p);
        }
      }
      
        freep = prevp;
        //printf(1,"breturn freep %d\n",freep);

        bit_on(va+1,10);
        // if((uint)(va + 1) % PGSIZE == 0 )
        //    printf(1,"page is aligned\n");
        return (void*)(va+1);
      }
      else {
        //printf(1,"looping\n");
        //printf(1,"updated freep %d %d\n",freep,p);
        if(p == freep){
          if((p = morecore(nunits)) == 0)
            return 0;
        }
        /*else continue;*/}


  }
}




// we have a circular structure,in malloc we go over all headers looking for
// free memory. Here we are required to add a new page anyway regardless whether we have free memory.


int protect_page(void* ap){
  //printf(1,"in protect page, succed add is %d\n",ap);
  if (bit_check(ap, 10) == 0) {
      // printf(1,"in protect page, not made by pmalloc\n");
    return -1;
  }
  
  if (((uint)ap)%PGSIZE != 0) {
    // printf(1,"in protect page, not aligned\n");
    return -1;
  }
  bit_off(ap, 1);
  //printf(1,"in protect page, succed add is %d\n",ap);
  return 1;
}

int pfree(void* ap){
  if (bit_check(ap,10) == 0 ) { // check that it was made by pmalloc
    return -1;
  }
  // if (bit_check(ap, 1) == 0) { // check that it's not protected - dont free.
  //   return -1;
  // }
  // //printf(1,"before call to free\n");

  bit_on(ap, 1);
  bit_off(ap, 10);
  free(ap);
  return 1;
}
