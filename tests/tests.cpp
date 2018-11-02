#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include "../Mutex.h"
#include "../Spinlock.h"

using namespace std;

#if TEST_MUTEX
Mutex lock;
#else
Spinlock lock;
#endif

int THREADS_NUM;
int OP_PER_THREAD;

void do_increment(int thread_num, int* cnt) {
    for (int i = 0; i < OP_PER_THREAD; i++) {
        lock.acquire();
        (*cnt)++;
        cout << "[Thread " << thread_num << "] " << *cnt << endl;
        lock.release();
    }
}

TEST_CASE("Concurrent count increment") {
    THREADS_NUM = 100;
    OP_PER_THREAD = 100;

    int cnt = 0;

    for (int current_try = 0; current_try < 10; current_try++) {
        vector<thread> threads;
        threads.reserve(THREADS_NUM);
        for (int current_thread = 0; current_thread < THREADS_NUM; current_thread++) {
            threads.emplace_back(thread(do_increment, current_thread, &cnt));
        }

        for (int i = 0; i < THREADS_NUM; i++) {
            threads[i].join();
        }

        REQUIRE(cnt == THREADS_NUM * OP_PER_THREAD);
        cnt = 0;
    }
}

// Работа wait такова:
// Для начала приведем семантику вызова wait(lock, Predicate), в котором используется предикат:
//while (!pred()) {
//wait(lock); <- здесь вызывается обычный void wait( std::unique_lock<std::mutex>& lock) без предиката
//}
//Тогда можем сделать следующие выводы:
// 1) Если тред дошел до строчки с wait в первый раз, то вызов wait проверяет предикат (без получения оповещения):
// если он true, то тред продолжает работу
// если он false, то тред вызывает wait(lock), который атомарно разблокирует мьютекс и помещает тред в заблокированное
// состояние в ожидании оповещения
//
// Это важный момент - Тред не ждет получения оповещения в самый первый вызов wait(lock, Predicate), так как возможен случай,
// когда оповещение могло просто потеряться, но предикат был изменен
//
// Теперь, если condition variable находится в состоянии ожидания (то есть тред заблокирован) и получает оповещение:
// 1) Тред разблокируется и захватывает мьютекс снова
// 2) Проверяется предикат:
// 3) Если предикат true: тред продолжает работу (мьютекс заблокирован)
//    Если предикат false: разблоирует мьютекс и тред помещается снова в заблокированное состояние

//Как пример, расммотрим работу двух функций - receiver и sender:
//
//void waitingForWork(){ <- receiver
//    std::cout << "Waiting " << std::endl;
//    std::unique_lock<std::mutex> lck(mutex_);
//    condVar.wait(lck, []{ return dataReady; });
//    std::cout << "Running " << std::endl;
//}
//
//void setDataReady(){ <- sender
//    {
//        std::lock_guard<std::mutex> lck(mutex_);
//        dataReady = true;
//    }
//    std::cout << "Data prepared" << std::endl;
//    condVar.notify_one(); <- вызов этой команде не трубется произодить в области, защищенной мьютексом
//}

// Рассмотрим 2 ситуации: если первый тред захватил лок первым, и если второй тред захватил лок первым
// 1) Receiver захватил лок первый
// Тогда второй тред блокируется в ожидании разблокировки мьютекса
// 1) Первый тред, захватив лок, продолжает работу и доходит до строчки с wait
// 2) Wait сразу, без ожидания оповещений, проверяет предикат, видит, что предикат равен false
// (так как второй тред в это время был заблокирован), разблокирует мьютекс и тред блокируется в ожидании оповещений на condVar
// 3) Второй поток дождался освобождения мьютекса, тред разблокируется и устанавливается предикат в значение true
// 4) Второй поток разблокирует мьютекс и продолжает работу, далее посылая оповещений командой notify_one()
// 5) Первый тред получает оповещение и проверяет предикат, видит что предикат равен true, и тред продолжает работу
//
// 2) Sender захватил лок первый:
// Тогда первый тред блокируется в ожидании разблокировки мьютекса
// 1) Второй тред устанавливает значение предиката в true и разблокирует мьютекс
//    Здесь возможны 2 ситуации:
//        1) Первый тред может продолжить работу в этот же момент и проверяет, что предикает равен true и продолжает работу
//        2) Первый тред не продолжает работу в этот же момент, тогда второй тред продолжает работу и посылает оповещение
// первому потоку. Первый поток не отслеживает оповщение, а сразу проверяет что предикат равен true и продолжает работу.

// Зачем нужен предикат
// Если не использовать предикат, то возможна ситуация, когда сигнал был послан до того, как поток начал ожидание сигнала,
// и сигнал может потеряться (lost wakeup),
// а также возможна ситуация, когда произошло ложное оповещение (spurious wakeup), и тред был разбловирован, однако на самом
// деле ожиданиемого ивента не происходило и программа будет работать неправильно.
// Поэтому мы заручаемся поддержкой не только сигналов, но и памяти. По состояния памяти (т.е. переменной) мы может
// гарантировать правильное происхождение ивентов
//
//
// Почему нужно использовать мьютекс в sender потоке? Что, если сделать предикат атомарной переменной?
// Допустим, что код потоков будет таким:
//void waitingForWork(){
//    std::cout << "Waiting " << std::endl;
//    std::unique_lock<std::mutex> lck(mutex_);
//    condVar.wait(lck, []{ return dataReady.load(); });
//    std::cout << "Running " << std::endl;
//}
//
// void setDataReady(){ <- sender
//     dataReady.store(true);
//     std::cout << "Data prepared" << std::endl;
//     condVar.notify_one();
// }
//
// Тогда, еще раз рассмотрим работу двух потоков:
// 1) Если первый поток доходит до строчки с wait(), он проверяет первый раз:
//      1) Если второй поток успел положить значение true в предикат до того, как первый поток начал проверять его, то
// первый поток убеждается в происхождении ивента и успешно продолжает работу, все проходит нормально
//      2) Если первый поток видит значение false, то тред вызывает wait() и поток блокируется. НО, именно между моментом
// проверки предиката и блокировки треда в ожидании оповещений появляется временное окно, т.е. эти действия не происходят
// атомарно. Именно в этот прмомежуток времени мог произойти context switch и второй тред отправил сигнал до того, как
// возможность выполнения передалась обартно к первому потоку, чтобы он смог вызвать wait() и начать ожидать оповещения.
// Итог: сигнал, посланный вторым потоком (sender) теряется, первый поток (receiver) больше никогда не получит оповещение,
// а значит, первый поток блокируется навсегда. Такая ситуация известна как Deadlock.
// Даннная проблема решается применением мьютекса во втором треде. Если первый поток захватил лок, то второй поток не
// может работать с предикатом и посылать оповещения. Первый поток увидит значение false, пройдет какой-то промежуток
// времени, но во время него второй поток не сможет получить доступ к работе над предикатом, так как он заблокируется на
// мьютексе, далее первый поток атомарно разблокирует мьютекс и входит  в заблокированное состояние в ожидании оповещения.
// Только после того, как вызов wait() разблокировал мьютекс, второй сделает работу и пошлет оповещение

// Без использования мьютекса вторым потоком могла возникнуть такая ситуация:
//     Thread 1                         Thread 2
//
//     unique_lock
//     while (!condition)
//     <временное окно,
//     происходит context switch>
//                                      condition = true
//                                      condVar.notify_one()
//
//     condVar.wait()

// Почему нужно использовать mutex в вызове wait()?
// Для избежания race condition.
// Важная особенность wait заключается в том, что он атомарно разблокирует мьютекс и помещает тред в
// заблокированное состояние. Если бы данная операция не была атомарной, то между моментом разблокировки мьютекса
// и началом ожидания оповещеня сигнал мог послаться другим тредом и потеряться, так как первый тред не успел
// войти в состояние ожидания сигнала. Как итог, происходит deadlock
// То есть, передавая мьютекс как аргумент, мы исключаем данную ситуацию.
// Кроме того, команда wait() не только атомарно разблокирует мьютекс и помещает тред в заблокированное состояние, но
// также, когда произойдет wakeup, то есть придет сигнал, атомарно разблокирует тред и повторно блокирует мьютекс.
// Дело в том, что wakeup мог быть ложным (spurious wakeup), и второй тред еще даже не приступал к работе, или же еще не
// успел послать настоящий сигнал.
// Если бы разблокировка треда и повторная блокировка мьютекса не были неделимыми действиями, то второй тред мог вступить
// в работу и послать сигнал, который потеряется
// Если бы повторной блокировки мьютекса, переданного как аргумент, не было вообще, то второй тред смог бы изменять
// состояние памяти в то время, как память проверяется первым потоком, и возник бы race condition. Более того, не
// используя повторную блокировку мьютекса, сигнал мог вообще потеряться и произошел бы deadlock.

// Важно различать следующие моменты:
// Мы используем мьютекс в sender треде, так как не хотим, чтобы между моментом проверки предиката и вызовом wait()
// произошло послание сигнала другим тредом
// Вызов wait() производит операции разблокировки мьютекса и помещения тред в заблокированное состояние в ожидании сигнала
//атомарно - это исключает другую ситуацию, когда, даже если второй тред был защищен локом, но после разблокировки мьютекса,
// если бы произошел context switch, мог послать сигнал, который потерялся бы


// predicate - нам нужен предикат, так как сигналы могут теряться, поэтому нам нужна
// дополнительная гарантия в виде памяти (сейчас мы говорим об аппаратных вещах, сигналы могут теряться, тогда помогает
// отслеживание памяти)
// с помощью отслеживания состояния памяти мы можем исключить lost wakeup и spurious wakeup

// spurious wakeup - ложное пробуждение condition variable, то есть когда сигнала от треда, который должен был послать
// оповещение, на самом деле не было
// lost wakeup - ситуация, когда сигнал теряется, то есть оповещение произошло тогда, когда сигнал никто не ожидал
// То есть вызов notify() произошел до вызова notify()

mutex first_thread_acquired_mutex;
condition_variable is_first_thread_acquired_cond;
bool first_thread_acquired_pred = false;

mutex second_thread_tried_to_acquire_mutex;
condition_variable is_second_thread_tried_to_acquire_cond;
bool second_thread_tried_to_acquire_pred = false;

mutex first_thread_released_mutex;
condition_variable is_first_thread_released_cond;
bool first_thread_released_pred = false;

void do_acquire() {
    // we need to ensure that first thread acquired a lock before we call try_lock() func
    unique_lock<mutex> first_thread_acquired_unique_lock(first_thread_acquired_mutex);
    cout << "Thread 2 is waiting until the lock is acquired by Thread 1" << endl;
    is_first_thread_acquired_cond.wait(first_thread_acquired_unique_lock, [] { return first_thread_acquired_pred; });

    cout << "Thread 2 got a notify and trying to acquire a lock while Thread 1 is holding it" << endl;
    bool locked = lock.try_lock();
    REQUIRE(!locked);
    cout << "Thread 2 tried to acquire a lock | Result: " + to_string(locked) << endl;

    {
        lock_guard<mutex> lockGuard(second_thread_tried_to_acquire_mutex);
        second_thread_tried_to_acquire_pred = true;
        cout << "Thread 2 tried to acquire a lock so need to wake up Thread 1" << endl;
    }
    is_second_thread_tried_to_acquire_cond.notify_one();

    cout << "Thread 2 is waiting until the lock is released by Thread 1" << endl;
    unique_lock<mutex> first_thread_released_unique_lock(first_thread_released_mutex);
    is_first_thread_released_cond.wait(first_thread_released_unique_lock, [] { return first_thread_released_pred; });

    cout << "Thread 2 got a notify and trying to acquire a lock while Thread 1 released it" << endl;
    locked = lock.try_lock();
    cout << "Thread 2 tried to acquire a lock | Result: " + to_string(locked) << endl;
    REQUIRE(locked);

    lock.release();
}

void find_n_prime_number(int n) {
    lock.acquire();
    {
        lock_guard<mutex> lockGuard(first_thread_acquired_mutex);
        first_thread_acquired_pred = true;
        cout << "Thread 1 acquired a lock" << endl;
    }
    is_first_thread_acquired_cond.notify_one();

    unique_lock<mutex> second_thread_tried_to_acquire_unique_lock(second_thread_tried_to_acquire_mutex);
    cout << "Thread 1 is waiting until Thread 2 try to acquire a lock" << endl;
    is_second_thread_tried_to_acquire_cond.wait(second_thread_tried_to_acquire_unique_lock,
                                                [] { return second_thread_tried_to_acquire_pred; });

    cout << "Thread 1 got a notify and started calculating n-th prime number" << endl;

    int prime_numbers_cnt = 0;
    int current_num = 2;

    while (true) {
        prime_numbers_cnt++;
        for (int j = 2; j <= static_cast<int>(sqrt(current_num)); j++) {
            if (current_num % j == 0) {
                prime_numbers_cnt--;
                break;
            }
        }

        if (prime_numbers_cnt == n) { // found n-th prime number
            break;
        }
        current_num++;
    }

    cout << "Thread 1 found n-th prime number" << endl;

    lock.release();
    cout << "Thread 1 released a lock" << endl;
    {
        lock_guard<mutex> lockGuard(first_thread_released_mutex);
        first_thread_released_pred = true;
        cout << "Thread 1 is notifying that lock was released so Thread 2 can try to acquire a lock again" << endl;
    }
    is_first_thread_released_cond.notify_one();
}

TEST_CASE("Unable acquire a lock while other thread already acquired it") {
    THREADS_NUM = 2;

    vector<thread> threads;
    threads.reserve(THREADS_NUM);

    threads.emplace_back(thread(do_acquire)); // put second thread to work
    threads.emplace_back(thread(find_n_prime_number, 50000)); // put first thread to work (long work)

    for (int i = 0; i < THREADS_NUM; i++) {
        threads[i].join();
    }
}