#include <stdio.h>
#include "utf8util.h"
#include "other/tlsf/tlsf.h"

#define USE_TLSF 0
#define NUM_ITER 100000
#define NUM_PTRS 1024
#define POOL_BYTES 0x10000000
#define asize(i,j) ( (((i) * 701 + 307*(j) + 111) & 0x7FFFFFFF) % ((POOL_BYTES/NUM_PTRS)) + (j)%1000 + 1 )

int main()
{
		void* pool = LZ::memory::allocator::malloc(POOL_BYTES);
		void *ptr_arr[NUM_PTRS];
		printf("POOL_BYTES = %i\n", POOL_BYTES);

#if USE_TLSF
		tlsf_t tlsf = tlsf_create_with_pool(pool, POOL_BYTES);
		printf("tlsf_size = %i\n", tlsf_size());
		for (int i = 0;i < NUM_PTRS;i++)
		{
			ptr_arr[i] = tlsf_malloc(tlsf, asize(i, i * 3));
			if (ptr_arr[i] == 0)break;
		}
		for (int i = 1,j;i < 100; i += 2)
		{
			j = (i * 53) % NUM_PTRS;
			if (ptr_arr[j] == 0) { printf("free ptr_arr[i] == NULL !!! i=%i\n", i); continue; }
			tlsf_free(tlsf, ptr_arr[j]); ptr_arr[j] = 0;
		}
		for (int k = 0,j;k < NUM_ITER;k++)
		{
			for (int i = 0;i < NUM_PTRS; i+=2)
			{
				j = (i * 37) % NUM_PTRS;
				if (ptr_arr[j] == 0) { printf("free ptr_arr[i] == NULL !!! k = %i i=%i\n", k, i); continue; }
				tlsf_free(tlsf, ptr_arr[j]); ptr_arr[j] = 0;
			}
			for (int i = 0;i < NUM_PTRS; i+=2)
			{
				j = (i * 37) % NUM_PTRS;
				if (ptr_arr[j] != 0) { printf("malloc ptr_arr[i] not NULL !!! k = %i i=%i\n", k, i); continue; }
				ptr_arr[j] = tlsf_malloc(tlsf, asize(i + k, k * i));
				if (ptr_arr[j] == 0)break;
			}
		}
		tlsf_destroy(tlsf);
#else
		LZ::memory::page_manager manager(pool, POOL_BYTES);
		printf("used_size = %i\n", manager.get_used_size());
		for (int i = 0;i < NUM_PTRS;i++)
		{
			ptr_arr[i] = manager.malloc(asize(i, i * 3));
			if (ptr_arr[i] == 0)break;
		}
		printf("used_size = %i\n", manager.get_used_size());
		for (int i = 1, j;i < 100; i += 2)
		{
			j = (i * 53) % NUM_PTRS;
			if (ptr_arr[j] == 0) { printf("free ptr_arr[i] == NULL !!! i=%i\n", i); continue; }
			manager.free(ptr_arr[j]); ptr_arr[j] = 0;
		}
		for (int k = 0, j;k < NUM_ITER;k++)
		{
			for (int i = 0;i < NUM_PTRS; i+=2)
			{
				j = (i * 37) % NUM_PTRS;
				if (ptr_arr[j] == 0){ printf("free ptr_arr[i] == NULL !!! k = %i i=%i\n", k, i); continue; }
				manager.free(ptr_arr[j]); ptr_arr[j] = 0;
			}
			//printf("used_size = %i\n", manager.get_used_size());
			for (int i = 0;i < NUM_PTRS; i+=2)
			{
				j = (i * 37) % NUM_PTRS;
				if (ptr_arr[j] != 0) { printf("malloc ptr_arr[i] not NULL !!! k = %i i=%i\n", k, i); continue; }
				ptr_arr[j] = manager.malloc(asize(i + k, k * i));
				if (ptr_arr[j] == 0)break;
			}
			//printf("used_size = %i\n", manager.get_used_size());
		}
		manager.destroy();
#endif

    return 0;
}
