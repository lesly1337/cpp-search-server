#pragma once  
#include <set>  
#include <vector>  
#include <string>  
std::vector<std::string_view> SplitIntoWords(std::string_view text); 
 
template <typename StringContainer>   
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {   
    std::set<std::string, std::less<>> non_empty_strings;   
    for (std::string_view str : strings) {   
        if (!str.empty()) {   
            non_empty_strings.emplace(str);//заменил insert на emplace   
        }   
    }   
    return non_empty_strings;  
  }