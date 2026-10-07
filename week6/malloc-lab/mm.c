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

/* 가용 블록의 payload 안에 저장되는 pred/succ 포인터 접근 */
#define PRED(bp) (*(void **)(bp))                              // 이전 가용 블록 주소
#define SUCC(bp) (*(void **)((char *)(bp) + sizeof(void *)))   // 다음 가용 블록 주소

/* 최소 블록 크기: 헤더 + pred + succ + 풋터를 8의 배수로 맞춤 */
#define MIN_BLOCK_SIZE ALIGN(DSIZE + 2 * sizeof(void *))

static char *heap_listp;
static void *free_listp = NULL;   // 가용 리스트의 첫 블록

static void *extend_heap(size_t words);
static void *coleasce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);

/* 가용 블록 bp를 리스트 맨 앞에 삽입 (LIFO) */
static void insert_free_block(void *bp)
{

    PRED(bp) = NULL;              // 맨 앞이므로 이전 블록 없음
    SUCC(bp) = free_listp;        // 기존 첫 블록을 다음으로
    
    if (free_listp != NULL)       // 리스트가 비어 있지 않으면
        PRED(free_listp) = bp;    // 기존 첫 블록의 이전을 bp로

    free_listp = bp;              // 시작점을 bp로 갱신
}

/* 가용 블록 bp를 리스트에서 제거 */
static void remove_free_block(void *bp)
{
    void *pred = PRED(bp);
    void *succ = SUCC(bp);

    if (pred != NULL)             // 앞 블록이 있으면
        SUCC(pred) = succ;        // 앞 블록이 bp를 건너뛰고 succ를 가리키게
    else                          // bp가 맨 앞이었으면
        free_listp = succ;        // 시작점을 succ로

    if (succ != NULL)             // 뒤 블록이 있으면
        PRED(succ) = pred;        // 뒤 블록이 bp를 건너뛰고 pred를 가리키게
}

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

    free_listp = NULL;

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
        // 병합 없음 -> 아래에서 insert
    }
    else if(!prev_alloc && next_alloc){
        remove_free_block(PREV_BLKP(bp));
        // 이전 블록은 가용 상태이고 다음 블록은 할당 상태이기 때문에 가용 블록과 현재를 합침
        size += GET_SIZE(FTRP((PREV_BLKP(bp))));
        PUT(FTRP(bp),PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else if(prev_alloc && !next_alloc){
        remove_free_block(NEXT_BLKP(bp));
        // 다음 블록은 가용 상태이고 이전 블록은 할당 상태이기 때문에 가용 블록과 현재를 합침
        size += GET_SIZE(HDRP((NEXT_BLKP(bp))));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0)); //여기 Segment Fault 발생 #1
    }else{
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        // 둘다 가용 가능 상태이므로 총 3개의 블록을 합침
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    insert_free_block(bp);
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
#include <limits.h>

void* find_fit(size_t size){
    // for문의 시작 주소로 heap의 프롤로그 블록 bp사용
    // 이후 반복문의 값으로 bp + current_block_size를 이용
    // bp = bp + current_block_size를 통해서 이동 시킨 후 해당 block의 크기를 잡아다가 범위 비교
    if (size == 0){
        return NULL;
    }
    void *bp;
    void *diff_bp; 
    size_t cur_size;
    size_t diff_size = INT_MAX; 
    // 이 부분을 
    for (bp = free_listp; bp != NULL; bp = SUCC(bp)){
        cur_size = GET_SIZE(HDRP(bp));
        if (cur_size>=size){
            if (diff_size > cur_size - size){
                diff_bp = bp;
                diff_size = cur_size - size;
            }
        }
    }
    if (diff_size != INT_MAX){
        return diff_bp;
    }
    return NULL;
}
#define THRESHOLD 100

static void *place_impl(void *bp, size_t size)
{
    size_t origin_size = GET_SIZE(HDRP(bp));
    remove_free_block(bp);

    if ((origin_size - size) < MIN_BLOCK_SIZE)
    {
        PUT(HDRP(bp), PACK(origin_size, 1));
        PUT(FTRP(bp), PACK(origin_size, 1));
        return bp;
    }
    else if (size >= THRESHOLD)
    {
        PUT(HDRP(bp), PACK(origin_size - size, 0));
        PUT(FTRP(bp), PACK(origin_size - size, 0));
        insert_free_block(bp);

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        return bp;
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));

        PUT(HDRP(NEXT_BLKP(bp)), PACK(origin_size - size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(origin_size - size, 0));
        insert_free_block(NEXT_BLKP(bp));
        return bp;
    }
}

// void place(void* bp, size_t size){
//     bp = place_impl(bp, size);
// }
#define place(bp, size) ((bp) = place_impl((bp), (size)))

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0){
        return NULL; // size가 0이라면 반환할 것이 없기 때문에 NULL 반환
    }
    if (size <= DSIZE){ // size가 DSIZE(8바이트)보다 크다면 
        asize = MIN_BLOCK_SIZE;
    }
    else{
        asize = DSIZE * ((size+(DSIZE) + (DSIZE - 1))/DSIZE);
    }
    asize = MAX(asize, MIN_BLOCK_SIZE);

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
    /* 0단계: 특수한 경우 */
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t asize;
    if (size <= DSIZE)
        asize = MIN_BLOCK_SIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    asize = MAX(asize, MIN_BLOCK_SIZE);

    size_t old_size   = GET_SIZE(HDRP(ptr));
    void  *prev_bp    = PREV_BLKP(ptr);
    void  *next_bp    = NEXT_BLKP(ptr);
    size_t prev_alloc = GET_ALLOC(HDRP(prev_bp));
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t prev_size  = GET_SIZE(HDRP(prev_bp));
    size_t next_size  = GET_SIZE(HDRP(next_bp));

    /* 3단계: 기존 블록으로 충분 → 그대로 반환 */
    if (old_size >= asize)
        return ptr;

    /* 4단계: 다음 블록과 합치면 충분 → 주소 그대로, 데이터 이동 없음 */
    if (!next_alloc && old_size + next_size >= asize) {
        remove_free_block(next_bp);   // 추가
        size_t total = old_size + next_size;
        PUT(HDRP(ptr), PACK(total, 1));
        PUT(FTRP(ptr), PACK(total, 1));
        return ptr;
    }

    /* 5단계: 앞 블록(+ 가능하면 뒤 블록)과 합치면 충분 → 데이터를 앞으로 이동 */
    if (!prev_alloc) {
        size_t total = prev_size + old_size;
        if (!next_alloc)
            total += next_size;

        if (total >= asize) {
            remove_free_block(prev_bp);        // memmove 전에!
            if (!next_alloc)
                remove_free_block(next_bp);    // 뒤 블록도 합쳤다면 같이 뺌
            memmove(prev_bp, ptr, old_size - DSIZE);
            PUT(HDRP(prev_bp), PACK(total, 1));
            PUT(FTRP(prev_bp), PACK(total, 1));
            return prev_bp;
        }
    }
    if (next_size == 0 || (!next_alloc && GET_SIZE(HDRP(NEXT_BLKP(next_bp))) == 0)){

    }

    /* 6단계: 어느 것도 안 됨 → 새로 할당, 복사, 해제 */
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t copySize = old_size - DSIZE;   // 기존 payload 크기
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}