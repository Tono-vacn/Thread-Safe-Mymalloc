#include "my_malloc.h"
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>

unsigned long given_mem = 0;
unsigned long free_mem = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nolock = PTHREAD_MUTEX_INITIALIZER;

__thread mem_blk * global_head_origin = NULL;


void sol_mem_blk(mem_blk * cur){
  if(cur==NULL){
    perror("invalid block to set sol");
    return;
  }
  cur->prev=NULL;
  cur->next=NULL;
}

void set_blk_size(mem_blk * cur, size_t size){
  if(size<=0||cur==NULL){
    perror("invalid block to set size");
    return;
  }
  cur->size = size;
}

void plus_given_mem(size_t size){
  given_mem+=size;
}

void plus_free_mem(size_t size){
  free_mem+=size;
}

void minus_free_mem(size_t size){
  free_mem-=size;
}

mem_blk * get_new_mem_blk(size_t size){
  if(size>0){
    pthread_mutex_lock(&nolock);
    mem_blk* new_mem_blk = (mem_blk*)sbrk(size+sizeof(mem_blk));
    pthread_mutex_unlock(&nolock);

    if(new_mem_blk==(void*)-1){
      perror("unable to perform sbrk");
      return NULL;
    }
    set_blk_size(new_mem_blk,size);
    sol_mem_blk(new_mem_blk);
    //given_mem+=(size+sizeof(mem_blk));
    plus_given_mem(size+sizeof(mem_blk));
    return new_mem_blk;
  }
  else{
    perror("invalid size for new mem_blk");
    return NULL;
  }
}

mem_blk * divide_mem_blk(mem_blk * cur, size_t size){
if(cur==NULL || size<=0){
  perror("invalid mem_blk to divide");
  return NULL;
}else{
  //free_mem -=(size+sizeof(mem_blk));
  minus_free_mem(size+sizeof(mem_blk));
  set_blk_size(cur,cur->size-(size+sizeof(mem_blk)));
  mem_blk * new_mem_blk = (mem_blk *)((void*)cur+sizeof(mem_blk)+cur->size);
  sol_mem_blk(new_mem_blk);
  set_blk_size(new_mem_blk, size);
  return new_mem_blk;
}
}

mem_blk * merge_mem_blk(mem_blk* prv, mem_blk * nxt){
  if(prv==NULL){
    return nxt;
  }else if (nxt==NULL){
    return prv;
  }else if ((void*)prv+prv->size+sizeof(mem_blk)==(void*)nxt){
    set_blk_size(prv,prv->size+nxt->size+sizeof(mem_blk));
    if(nxt->next!=NULL){
      nxt->next->prev = prv;
      prv->next = nxt->next;
    }else{
      prv->next = NULL;
    }
    sol_mem_blk(nxt);
    return prv;
  }else{
    return NULL;
  }
  
}

void use_mem_blk(mem_blk * cur){
  if(cur==NULL){
    perror("invalid mem_blk to use from free mem_blk list");
    //return NULL;
  }else{
  if(cur->prev==NULL){
    head_origin = cur->next;
  }else{
    cur->prev->next = cur->next;
  }

  if(cur->next!=NULL){
    cur->next->prev = cur->prev;
  }
  sol_mem_blk(cur);
  minus_free_mem(cur->size+sizeof(mem_blk));
  //free_mem-=(cur->size+sizeof(mem_blk));
  }

}

void use_mem_blk_thread(mem_blk * cur){
  if(cur==NULL){
    perror("invalid mem_blk to use from free mem_blk list");
    //return NULL;
  }else{
  if(cur->prev==NULL){
    global_head_origin = cur->next;
  }else{
    cur->prev->next = cur->next;
  }

  if(cur->next!=NULL){
    cur->next->prev = cur->prev;
  }
  sol_mem_blk(cur);
  minus_free_mem(cur->size+sizeof(mem_blk));
  //free_mem-=(cur->size+sizeof(mem_blk));
  }

}

void back_to_free(mem_blk * cur){
  if(cur==NULL){
    perror("invalid mem_blk to free");
  }

  if(head_origin==NULL){
    sol_mem_blk(cur);
    head_origin = cur;
    plus_free_mem(cur->size+sizeof(mem_blk));
    //free_mem +=(cur->size+sizeof(mem_blk));
  }else{
    mem_blk * ptr = head_origin;
    mem_blk * ptr_prv = NULL;
    while(ptr!=NULL){
      if(cur<ptr){
        break;
      }
      ptr_prv = ptr;
      ptr = ptr->next;
    }
    cur->next = ptr;
    cur->prev = ptr_prv;
    if(ptr){
      ptr->prev = cur;
    }

    if(ptr_prv){    
        ptr_prv->next = cur;
      }
      else{
        head_origin = cur;
      }

    //free_mem +=(cur->size+sizeof(mem_blk));
    plus_free_mem(cur->size+sizeof(mem_blk));

    //merge
    if(ptr){
      merge_mem_blk(cur,ptr);
    }

    if(ptr_prv){
      merge_mem_blk(ptr_prv,cur);
    }

  }


}

void back_to_free_thread(mem_blk * cur){
  if(cur==NULL){
    perror("invalid mem_blk to free");
  }

  if(global_head_origin==NULL){
    sol_mem_blk(cur);
    global_head_origin = cur;
    plus_free_mem(cur->size+sizeof(mem_blk));
    //free_mem +=(cur->size+sizeof(mem_blk));
  }else{
    mem_blk * ptr = global_head_origin;
    mem_blk * ptr_prv = NULL;
    while(ptr!=NULL){
      if(cur<ptr){
        break;
      }
      ptr_prv = ptr;
      ptr = ptr->next;
    }
    cur->next = ptr;
    cur->prev = ptr_prv;
    if(ptr){
      ptr->prev = cur;
    }

    if(ptr_prv){    
        ptr_prv->next = cur;
      }
      else{
        global_head_origin = cur;
      }

    //free_mem +=(cur->size+sizeof(mem_blk));
    plus_free_mem(cur->size+sizeof(mem_blk));

    //merge
    if(ptr){
      merge_mem_blk(cur,ptr);
    }

    if(ptr_prv){
      merge_mem_blk(ptr_prv,cur);
    }

  }


}

void * divide_or_not(mem_blk * cur, size_t size){
    if(cur->size>size+sizeof(mem_blk)){
      mem_blk * new_mem_blk = divide_mem_blk(cur, size);
      return (void*) new_mem_blk+sizeof(mem_blk);
    }else{
      use_mem_blk(cur);
      return (void*)cur+sizeof(mem_blk);
    }
}

void * divide_or_not_thread(mem_blk * cur, size_t size){
    if(cur->size>size+sizeof(mem_blk)){
      mem_blk * new_mem_blk = divide_mem_blk(cur, size);
      return (void*) new_mem_blk+sizeof(mem_blk);
    }else{
      use_mem_blk_thread(cur);
      return (void*)cur+sizeof(mem_blk);
    }
}

// mem_blk * get_first_fit(size_t size){
//   if(size<=0){
//     perror("invalid input size for ff_malloc");
//     return NULL;
//   }
//   mem_blk * cur = head_origin;
//   while(cur!=NULL){
//     if(cur->size>=size){
//       return cur;
//     }
//     cur = cur->next;
//   }
//   return NULL;
// }

// void * ff_malloc(size_t size){
//   if(size<0){
//     perror("invalid input size");
//     return NULL;
//   }

//   mem_blk * first_fit = get_first_fit(size);
//   if(first_fit==NULL){
//     mem_blk * res = get_new_mem_blk(size);
//     return (void*)res + sizeof(mem_blk);
//   }else{
//     return divide_or_not(first_fit, size);
//   }

//   // mem_blk * cur = head_origin;
//   // while(cur!=NULL){
//   //   if(cur->size>=size){
//   //     return divide_or_not(cur, size);
//   //   }
//   //   cur = cur->next;
//   // }

//   //mem_blk * res = get_new_mem_blk(size);
//   //return (void*)res + sizeof(mem_blk);
// }


void simple_free(void * ptr){
  if(ptr==NULL){
    perror("invalid value to free");
    return;
  }
  mem_blk * to_be_freed = (mem_blk*)(ptr-sizeof(mem_blk));
  back_to_free(to_be_freed);
}

void simple_free_thread(void * ptr){
  if(ptr==NULL){
    perror("invalid value to free");
    return;
  }
  mem_blk * to_be_freed = (mem_blk*)(ptr-sizeof(mem_blk));
  back_to_free_thread(to_be_freed);
}

// void ff_free(void * ptr){
//   simple_free(ptr);
// }

mem_blk * get_best_fit(size_t size){
  if(size<=0){
    perror("invalid input size for bf_malloc");
    return NULL;
  }
  mem_blk * cur = head_origin;
  mem_blk * rec = NULL;
  size_t rec_size = INT_MAX;
  while(cur!=NULL){
    if(cur->size>=size){
      if(cur->size==size){
        return cur;
      }
      else if(cur->size>size && cur->size<rec_size){
        rec_size = cur->size;
        rec = cur;
      }
    }
    cur= cur->next;
  }
  return rec;

}

mem_blk * get_best_fit_thread(size_t size){
  if(size<=0){
    perror("invalid input size for bf_malloc");
    return NULL;
  }
  mem_blk * cur = global_head_origin;
  mem_blk * rec = NULL;
  size_t rec_size = INT_MAX;
  while(cur!=NULL){
    if(cur->size>=size){
      if(cur->size==size){
        return cur;
      }
      else if(cur->size>size && cur->size<rec_size){
        rec_size = cur->size;
        rec = cur;
      }
    }
    cur= cur->next;
  }
  return rec;

}

void * bf_malloc(size_t size){
  if(size<0){
    perror("invalid input size for bf_malloc");
    return NULL;
  }
  mem_blk* best_fit = get_best_fit(size);
  if (best_fit==NULL){
    mem_blk * res = get_new_mem_blk(size);
    return (void*)res+sizeof(mem_blk);
  }else{
    return divide_or_not(best_fit,size);
  }
}

void bf_free(void * ptr){
  simple_free(ptr);
}

unsigned long get_data_segment_free_space_size(){
  return free_mem;
}

unsigned long get_data_segment_size(){
  return given_mem;
}

void * ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  mem_blk * res = bf_malloc(size);
  pthread_mutex_unlock(&lock);
  return res;
}

void ts_free_lock(void * ptr){
  pthread_mutex_lock(&lock);
  simple_free(ptr);
  pthread_mutex_unlock(&lock);
}

void * ts_malloc_nolock(size_t size){
  if(size<0){
    perror("invalid input size for bf_malloc");
    return NULL;
  }
  mem_blk* best_fit = get_best_fit_thread(size);
  if (best_fit==NULL){
    mem_blk * res = get_new_mem_blk(size);
    return (void*)res+sizeof(mem_blk);
  }else{
    return divide_or_not_thread(best_fit,size);
  }
}

void ts_free_nolock(void * ptr){
  if(ptr==NULL){
    perror("invalid value to free");
    return;
  }
  mem_blk * to_be_freed = (mem_blk*)(ptr-sizeof(mem_blk));
  back_to_free_thread(to_be_freed);
}