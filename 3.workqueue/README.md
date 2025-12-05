## 1.실습 내용:

    system_wq + schedule_work를 사용한 후반기 처리,
    전용 wq를 사용한 후반기 처리,
    delayed_wq를 사용한 후반기 처리, 총 3가지의 workqueue를 사용하여 인터럽트 처리를 실습하였다.

<br>

## 2.결과 :

    2.interrupt와 마찬가지로 캐릭터 디바이스를 생성하고 인터럽트를 발생시킨 후 워크큐를 이용해 후반기 처리를 진행한다.

    system_wq는 커널이 기본적으로 제공하는 전역 워크큐로서 별도의 워크큐 생성없이 사용할 수 있다.
    
    alloc_workqueue는 독립적인 작업 처리 환경이 필요할 때 사용한다. 
    독립적 스레드 풀을 가지므로 실행 정책을 세밀하게 조절할 수 있다.
    
    delayed_work는 특정 시점 이후에 시행되어야하는 작업을 처리하기 위한 워크큐의 확장 구조이다.
    설정된 시간 후에 work를 처리하게 된다.
<br>

## 3.사진 :
![1.system_wq](image/syswq%20실행결과.png)

![2.alloc_wq](image/allocwq%20실행결과.png)

![3.delayed_wq](image/delaywq%20실행결과.png)
