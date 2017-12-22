#include <sys/types.h>
#include <limits.h>
#include <stdio.h>

#include "run.h"
#include "util.h"

void *base = 0;

p_meta find_meta(p_meta *last, size_t size) {
  p_meta index = base;
  p_meta result = NULL;

  switch(fit_flag){
    case FIRST_FIT:
    {
      for (;index;index=index->next)
      {
        if ((index->size)>=size && index->free == 1)
        {
          result = index;
          break;
        }
        *last=index;
      }
    }
    break;

    case BEST_FIT:
    {
      for (;index;index=index->next)
      {
        if ((index->size)>=size && index->free==1)
        {
          if(result==NULL) result = index;
          else if (result->size>index->size) result =index;
        }
        *last=index;
      }

    }
    break;

    case WORST_FIT:
    {
      for (;index;index=index->next)
      {
        if ((index->size)>=size && index->free==1)
        {
	  if (result == NULL) result =index;
          else if (result ->size < index->size) result = index;
        }
        *last=index;
      }
    }
    break;

  }
  return result;
}

void *m_malloc(size_t size) {  
  int flag;
  p_meta newdata;
  
  newdata = sbrk(0);
  flag=size/4;
  if (size % 4 !=0) size=flag*4+4;
  
  if (size <= 0) return NULL;

  if (base == 0)
  {
    newdata=sbrk(size+META_SIZE);
    if (newdata<=0) return NULL;
    newdata->size=size;
    newdata->next=NULL;
    newdata->prev=NULL;
    newdata->free=0;
    base=newdata;
   
  }
  else
  {
    p_meta prevdata;
    prevdata=base;
    
    newdata=find_meta(&prevdata,size);
    if (newdata<=0)
    {
      newdata=sbrk(size+META_SIZE);
      if (newdata<=0) return NULL;
     
      newdata->size = size;
      newdata->next=NULL;
      newdata->free=0;
      newdata->prev=prevdata;
      prevdata->next=newdata;
    }
    else
    {
      if (newdata->size-size > META_SIZE)
      {
        prevdata=newdata;
        newdata=(void*)(prevdata)+size+META_SIZE;
        newdata->size = prevdata->size-META_SIZE-size;
        newdata->prev = prevdata;
        newdata->next = prevdata->next;
        newdata->free=1;
        newdata->next->prev=newdata;
        prevdata->next = newdata;
        prevdata->size = size;
        newdata=prevdata;
      }
    }
    newdata->free=0;
  }

  return newdata->data;
}

void m_free(void *ptr) {
  p_meta data_ptr;

  if (ptr<=0) return NULL;
  
  data_ptr=ptr-META_SIZE;
  data_ptr->free=1;

  if (data_ptr->prev != NULL && data_ptr->prev->free == 1)
  {
    data_ptr->prev->size += data_ptr->size+META_SIZE;
    data_ptr->prev->next = data_ptr->next;
    data_ptr->next->prev = data_ptr->prev;
    data_ptr=data_ptr->prev;
  }

  if (data_ptr->next != NULL && data_ptr->next->free == 1)
  {
    data_ptr->size += data_ptr->next->size+META_SIZE;
    data_ptr->next->next->prev = data_ptr;
    data_ptr->next=data_ptr->next->next;
  }

  if (data_ptr->next == NULL) data_ptr->prev->next=NULL;
}

void*m_realloc(void* ptr, size_t size)
{
  p_meta data_ptr,newdata;
  int flag;

  if (ptr<=0) return m_malloc(size);

  flag=size/4;
  if (size % 4 !=0) size=flag*4+4;

  data_ptr=ptr-META_SIZE;
    
  if(data_ptr->size >= size)
  {
    if ((data_ptr->size)-size > META_SIZE)
    {
      newdata=data_ptr;
      data_ptr=(void*)(newdata)+size+META_SIZE;
      data_ptr->size=newdata->size-size-META_SIZE;
      data_ptr->next=newdata->next;
      data_ptr->prev=newdata;
      data_ptr->free=1;
      data_ptr->next->prev=data_ptr;
      newdata->next=data_ptr;
      newdata->size=size;
      data_ptr=newdata;
    }
    return ptr;
  } 
  
  void *new_ptr;
  new_ptr=m_malloc(size);
  if (new_ptr<=0) return NULL;
  memcpy(new_ptr,ptr,size);  
  m_free(ptr);
  return new_ptr;
}
