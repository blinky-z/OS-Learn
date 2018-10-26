#include "Spinlock.h"

void Spinlock::acquire() {
    // самый просток спинлок:
    // while (lock != 0);
    // lock = 1;
    // Проблема:
    // один поток может убедиться, что lock равен 0 (выполнить инструкцию while (lock != 0)) и,
    // если его квант времени истечет и запустится другой поток, который также убедится, что lock равен 0, так как
    // первый поток не успел захватить лок, и получится ситуация, когда два потока будут работать с критической областью
    // одновременно
    // ------------------
    // Решение:
    // 1) Запретить прерывания, но у меня нет доступа к такой команде
    // 2) Использовать атомарную переменную самого языка, при чем, реализация атомарной переменной должна быть lock-free,
    // так как иначе принцип спинлока нарушится (поток не должен блокироваться)
    // Почему нужна атомарная переменная?
    // 1) Требуется сделать операцию проверки и установки нового значения лока НЕДЕЛИМОЙ операцией
    // или, по другому, атомарной операцией - то есть или операция выполняется полностью, или не выполняется вообще
    // 2) при этом необходима гарантия того, что значение не будет изменяться другим потоком во время попытки чтения или
    // изменения значения первым потоком, то есть исключение race condition
    //
    // Это исключает случай, когда проверка произошла успешно, а перед установкой нового значения произошло переключение,
    // то есть операция остановилась на половине пути, а не прошла полностью
    //
    // Выполнение всех этих условий гарантирует std::atomic_flag - специальный тип, реализующий операцию проверки
    // значения и установки нового значения как неделимую операцию, при чем исключащий race condition: в момент чтения
    // значения и установки нового никакой поток не сможет повлиять на значение переменной
    // -----------------------------------
    // также, существует такое понятие как volatile - однако, переменная такого типа тоже не подходит, так как volatile не
    // гарантирует, что в один момент времени с переменной будет работать только один поток, то есть race condition
    // все еще существует, а гарантирует только, то что все потоки будут видеть самое новое значение переменной, то есть
    // не будет ситуации, когда потоки будут читать старое значение.
    // volatile также не может быть использована в спинлоке потому, что хоть потоки и видят самое новое значение, два
    // потока все еще могут одновременно убедиться, что значение lock равно 0 (потому что один из них может прерваться
    // и не успеть захватить лок), и начать работать с критической областью

    while (!this->try_lock());
}

void Spinlock::release() {
    __atomic_clear(&lock, 0);
}

bool Spinlock::try_lock() {
    if (__atomic_test_and_set(&lock, 0)) { // prev value was 1 (lock is already acquired)
        return false;
    } else {
        return true; // successful
    }
}
