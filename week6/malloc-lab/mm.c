/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
책에서 설명으로 적혀 있던 사이즈, 
헤더/풋터 4바이트 -> word size 
정렬 단위 8바이트 -> double word size
*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12) // 2^12 = 4096 바이트 (4KB) -> os가 메모리를 페이지 단위로 주기 때문에 힙을 늘리는 단위를 페이지 크기에 맞추는 게 자연스러움

/* Read and write a word at address p -> 주소 연산 관련 복잡한 함수를 선언해 둔 파트 */
#define PUT(p, val) (*(unsigned int *)(p)=(val))
#define GET(p) (*(unsigned int *)(p))

#define GET_ALLOC(p) (GET(p) & 0x1) // 헤더의 마지막 값을 보면 할당 여부를 볼 수 있음
#define GET_SIZE(p) (GET(p) & ~0x7) // 헤더의 마지막 3비트를 제외하고 보게 되면 사이즈를 알 수 있음

#define HDRP(bp) ((char *)(bp) - WSIZE) // bp가 payload의 시작주소 | 헤더 푸터 사이의 주소이기 때문에 bp에서 WSIZE(헤더 크기)를 빼면 
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE) // bp + GET_SIZE(HDRP(bp)) -> 다음 블록의 bp값을 얻음, 여기서 DSIZE(헤더 + 풋터) 를 뺴게 되면 풋터의 시작 주소를 알게 됨

#define PACK(size, alloc) ((size)|(alloc))

#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE(((char*)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE(((char*)(bp)-DSIZE)))

#define MAX(x, y) ((x) > (y) ? (x) : (y))

static char *heap_listp;

static void *extend_heap(size_t words);
static void *coleasce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

int mm_init(void)
{   
    // char *start = mem_heap_lo();
    // 원래 식에서는 ALIGNMENT 값에 WSIZE = 4 라는 값이 들어가는데...흠
    if ((heap_listp = mem_sbrk(2*ALIGNMENT))==(void *)-1){ // 검사 기준 최소 힙 할당 8의 배수 -> 16이어야 함. 이 부분을 맞추면 됨
        return -1; /* 만약 할당하는 int incr의 값이 음수면 mem_sbrk에서 (void *)-1을 반환함 */
    }
    PUT(heap_listp, 0); // 가장 앞 머리
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1)); // 프롤로그 헤더를 넣었기 때문에 sbrk를 WSIZE만큼 옮긴 후 넣어야 함, 뒤의 PACK 무조건 4바이트 만큼의 공간에 DSIZE의 값을 앞의 비트에 저장하고 나머지 1을 할당 비트로 합치는 내용
    PUT(heap_listp+2*WSIZE, PACK(DSIZE, 1)); // 프롤로그 풋터도 마찬가지
    PUT(heap_listp+3*WSIZE, PACK(0, 1)); // 에필로그 헤더
    heap_listp += 2*WSIZE; //heap_listp 는 힙의 첫 bp (프롤로그 블록, 에필로그의 사이)
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL){
        return -1;
    }
    return 0;
}

static void *coleasce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        return bp; // 둘다 1인 경우, 둘다 할당된 상태이기 때문에 현재의 bp를 반환
    }
    else if(!prev_alloc && next_alloc){
        // 이전 블록은 가용 상태이고 다음 블록은 할당 상태이기 때문에 가용 블록과 현재를 합침
        size += GET_SIZE(FTRP((PREV_BLKP(bp))));
        PUT(FTRP(bp),PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else if(prev_alloc && !next_alloc){
        // 다음 블록은 가용 상태이고 이전 블록은 할당 상태이기 때문에 가용 블록과 현재를 합침
        size += GET_SIZE(HDRP((NEXT_BLKP(bp))));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0)); //여기 Segment Fault 발생 #1
    }else{
        // 둘다 가용 가능 상태이므로 총 3개의 블록을 합침
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}


void* extend_heap(size_t words){
    char *bp;
    size_t size;
    
    if (words%2==0){ // words가 짝수인 경우 size = words * WSIZE
        size = words * WSIZE;
    }else{ // 만약 아닌 경우는 size = (words + 1) * WSIZE
        size = (words+1) * WSIZE;
    }
    if ((long)(bp = mem_sbrk(size))==-1) 
    /*
    sbrk가 힙을 늘리려 하는데 incr = size에서 size의 값이 0보다 작거나
    max_heap_addr 보다 크면 -1을 뱉음
    */
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0)); // 헤더에 size와 할당여부 (0 = 미할당) 합친 값으로 덮음
    PUT(FTRP(bp), PACK(size, 0)); // 풋터이므로 헤더와 똑같이
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); // 다음 bp의 값을 에필로그 블록으로 다시 설정, 에필로그 블록이기 때문에 1로 설정

    return coleasce(bp); // 미구현 함수로 구현해야함
}

void* find_fit(size_t size){
    // for문의 시작 주소로 heap의 프롤로그 블록 bp사용
    // 이후 반복문의 값으로 bp + current_block_size를 이용
    // bp = bp + current_block_size를 통해서 이동 시킨 후 해당 block의 크기를 잡아다가 범위 비교
    if (size == 0){
        return NULL;
    }
    char *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (GET_SIZE(HDRP(bp))>=size && !GET_ALLOC(HDRP(bp))){
            return bp;
        }
    }
    return NULL;
}

void place(void* bp, size_t size){
    /*
    요청한 블록을 가용 블록의 시작 부분에 배치해야 함
    나머지 부분의 크기가 최소 블록 크기와 같거나 큰 경우에만 분할

    place는 mm_malloc이 블록을 할당하는 과정에서 호출
    사용자가 malloc(size)를 요청하면 mm_malloc은 먼저 요청 크기를 정렬과
    오버헤드(헤더, 푸터)를 반영한 asize로 조정

    예를 들어 사용자 데이터 필요로 하는 것이 30 바이트라면
    30 + 4(헤더) + 4(풋터) => 38 => 8의 배수 -> 40바이트

    전체 64바이트가 존재한다고 하면 앞에 40바이트를 이러한 공간으로 할당한 후
    뒤의 빈 공간 24 바이트 할당 헤더 4바이트, 페이로드 16바이트, 풋터 4바이트 -> 24바이트
    */
   size_t origin_size = GET_SIZE(HDRP(bp));
   
   if ((origin_size-size) < 2*DSIZE) // 여기 Segment Fault 발생 #1
   {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
   }
   else
   {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(origin_size - size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(origin_size - size, 0)); 
   }
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    
    if(size == 0){
        return NULL; // size가 0이라면 반환할 것이 없기 때문에 NULL 반환
    }
    if (size <= DSIZE){ // size가 DSIZE(8바이트)보다 크다면 
        asize = 2 * DSIZE;
    }
    else{
        asize = DSIZE * ((size+(DSIZE) + (DSIZE - 1))/DSIZE);
    }
    if ((bp=find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp=extend_heap(extendsize/WSIZE))==NULL)
        return NULL;
    place(bp, asize);
    return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr),PACK(size, 0));
    PUT(FTRP(ptr),PACK(size, 0));
    coleasce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);

    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}