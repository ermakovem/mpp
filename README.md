Result:

[root@9f85b60c77b1 mpp]# ./hw_1.out

Параллельный sort:
        1. Время выполнения: 2.38421 секунд
        1. Время выполнения: 2.38421 секунд
        2. Время выполнения: 2.43773 секунд
        3. Время выполнения: 2.35468 секунд
        4. Время выполнения: 2.3722 секунд
        5. Время выполнения: 2.34251 секунд
        Среднее время выполнения: 2.37827
Последовательный sort:
        1. Время выполнения: 6.4277 секунд
        2. Время выполнения: 6.52812 секунд
        3. Время выполнения: 6.39651 секунд
        4. Время выполнения: 6.4427 секунд
        5. Время выполнения: 6.45506 секунд
        Среднее время выполнения: 6.45002
Ускорение в 2.71207 раз


To run:

0. git clone https://github.com/cmuparlay/parlaylib.git
1. g++ -O2 -I./parlaylib/include/ hw_1.cpp -o hw_1.out
2. export PARLAY_NUM_THREADS=4
3. ./hw_1.out