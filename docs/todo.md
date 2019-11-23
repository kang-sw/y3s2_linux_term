
# 순서대로 할 일

## 지침
- 가능한 rendering thread는 최적화. 무조건 렌더링 작업이 가장 길어지게 돼있음.

## TODO - Core System
1. [x] Drawing library 선택
   1. [x] Cairo 활용
2. [x] Resource system 정의
3. [ ] Rendering Interface 정의 
   1. [x] Rendering Loop
   2. [ ] Queueing Draw Calls
4. [ ] Global Function Timer 구현.
   * 단, 게임 스레드에서 실행되게끔.
5. [ ] Rendering Implementation
   1. [ ] 이미지 렌더
   2. [ ] 폰트 렌더
   3. [ ] 랙탱글 렌더
6. [ ] Delegate 구현
   * Game Object는 Handle을 통해서 
7. [ ] Game Object 구현
   1. [ ] Update 반응
   2. [ ] Event 반응
      1. [ ] OnClicked
      2. [ ] OnCollide
   3. [ ] Draw 반응
8. [ ] Game Round 구현
   * Game Round부터 상속 가능... 게임의 성질 정의
9.  [ ] Update 로직 구현
   1.  [ ] Program Instance의 Update
       1.  [ ] Play Waves
       2.  [ ] GameState.Update()
   2.  [ ] Game State의 Update
       1.  [ ] GameRound.HandleInput();
       2.  [ ] GameRound.Update();
       3.  [ ] Update Active Game Objects
       4.  [ ] Draw Active Game Objects
10. [ ] Collision 로직
11. [ ] 입력 처리
12. [ ] 사운드 처리

## TODO - Game
1. 