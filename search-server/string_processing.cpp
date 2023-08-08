#include "string_processing.h" 
std::vector<std::string_view> SplitIntoWords(std::string_view text) {  
    std::vector<std::string_view> words;
    text.remove_prefix(std::min(text.find_first_not_of(' '), text.size())); // подсмотрел пример https://en.cppreference.com/w/cpp/string/basic_string_view/remove_prefix
    while ( true ) {
        auto space = text.find(' ');
        words.push_back(text.substr(0, space));
        if ( space == text.npos ) {
            break;
        } else {
            text.remove_prefix(space + 1);//двигаем начало string_view
        }
    } 
  
    return words;  
}