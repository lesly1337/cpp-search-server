// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
#include <vector>

using namespace std;
// Решение 1
int main() {
    int num = 0;
    for ( int i = 1; i <= 1000; ++i ) {
        string s = to_string(i);
        for ( char c : s ) {
            if ( c == '3' ) {
               ++num ;
               break;
            } else {
                continue;
            }
        }
    
    }
        cout << "Number of 3: "s << num << endl; //Ответ: 271
}


// Закомитьте изменения и отправьте их в свой репозиторий.
