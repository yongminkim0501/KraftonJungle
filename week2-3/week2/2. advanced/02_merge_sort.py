"""
[머지 정렬 구현]

문제 설명:
- 머지 정렬(Merge Sort) 알고리즘을 구현합니다.
- 분할 정복(Divide and Conquer) 방식을 사용합니다.
- 배열을 절반으로 나누고, 각각을 정렬한 후 병합합니다.

입력:
- arr: 정렬되지 않은 정수 배열

출력:
- 오름차순으로 정렬된 배열

예제:
입력: [38, 27, 43, 3, 9, 82, 10]
출력: [3, 9, 10, 27, 38, 43, 82]

힌트:
- 배열을 절반으로 분할 (재귀)
- 각 부분을 재귀적으로 정렬
- 정렬된 두 부분을 병합
"""

def merge(arr, left, mid, right):
    """
    두 개의 정렬된 부분 배열을 병합하는 함수
    
    Args:
        arr: 원본 배열
        left: 왼쪽 부분의 시작 인덱스
        mid: 왼쪽 부분의 끝 인덱스
        right: 오른쪽 부분의 끝 인덱스
    """
    left_arr = arr[left:mid]
    right_arr = arr[mid + 1:right]

    temp_arr = []
    i = 0
    j = 0

    while (i != len(left_arr) and j != len(right_arr)):
        if len(left_arr) == i and len(right_arr) != j:
            temp_arr += right_arr[j:]
        if len(right_arr) == j and len(left_arr) != i:
            temp_arr += left_arr[i:]
        if len(right_arr) == 0 and len(left_arr) == 0:
            return temp_arr

        if left_arr[i] > right_arr[j]:
            temp_arr.append(right_arr[j])
            j = j + 1
        else:
            temp_arr.append(left_arr[i])
            i = i + 1
    # TODO: 왼쪽과 오른쪽 부분 배열을 임시 배열로 복사
    pass
    
    # TODO: 두 배열을 병합
    pass
    
    
    # TODO: left_arr와 right_arr를 비교하며 작은 값을 arr에 복사
    pass
    
    # TODO: 남은 원소들을 복사
    # left_arr에 남은 원소가 있으면 복사
    # right_arr에 남은 원소가 있으면 복사
    pass

def merge_sort_helper(arr, left, right):
    """
    머지 정렬 재귀 함수
    
    Args:
        arr: 배열
        left: 시작 인덱스
        right: 끝 인덱스
    """

    if left >= right: return arr

    mid = (left + right) // 2

    merge_sort_helper(arr, left, mid)
    merge_sort_helper(arr, mid + 1, right)

    temp_arr = []
    i = left
    j = mid + 1

    while (i <= mid and (j <= right)):
        if arr[i] > arr[j]:
            temp_arr.append(arr[j])
            j = j + 1
        else:
            temp_arr.append(arr[i])
            i = i + 1

    if i <= mid:
        temp_arr += arr[i:mid + 1]

    else:
        temp_arr += arr[j:right + 1]

    arr[left:right + 1] = temp_arr

    return arr

    # TODO: base case - left가 right보다 작을 때만 정렬
    ## 중간 지점 계산 .
    ## 왼쪽 절반 재귀 정렬
    ## 오른쪽 절반 재귀 정렬
    ## 정렬된 두 절반을 병합
    pass

def merge_sort(arr):
    """
    머지 정렬 메인 함수
    
    Args:
        arr: 정렬할 배열
    
    Returns:
        정렬된 배열
    """
    if len(arr) > 1:
        merge_sort_helper(arr, 0, len(arr) - 1)
    return arr

# 테스트 케이스
if __name__ == "__main__":
    # 테스트 케이스 1
    arr1 = [38, 27, 43, 3, 9, 82, 10]
    print("=== 테스트 케이스 1 ===")
    print(f"정렬 전: {arr1}")
    result1 = merge_sort(arr1.copy())
    print(f"정렬 후: {result1}")
    print()
    
    # 테스트 케이스 2
    arr2 = [12, 11, 13, 5, 6, 7]
    print("=== 테스트 케이스 2 ===")
    print(f"정렬 전: {arr2}")
    result2 = merge_sort(arr2.copy())
    print(f"정렬 후: {result2}")
    print()
    
    # 테스트 케이스 3: 역순
    arr3 = [9, 8, 7, 6, 5, 4, 3, 2, 1]
    print("=== 테스트 케이스 3: 역순 ===")
    print(f"정렬 전: {arr3}")
    result3 = merge_sort(arr3.copy())
    print(f"정렬 후: {result3}")
    print()
    
    # 테스트 케이스 4: 중복 원소
    arr4 = [5, 2, 8, 2, 9, 1, 5, 5]
    print("=== 테스트 케이스 4: 중복 원소 ===")
    print(f"정렬 전: {arr4}")
    result4 = merge_sort(arr4.copy())
    print(f"정렬 후: {result4}")


