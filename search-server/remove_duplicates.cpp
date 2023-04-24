#include "remove_duplicates.h"
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids_to_delete;
    std::set<std::set<std::string>> set_of_sets;
    for (const int document_id : search_server) {
        std::set<std::string> words;
        const auto& doc_in_set =  search_server.GetWordFrequencies(document_id);
        for (const auto& [word, freq] : doc_in_set) {
            words.insert(word);
        }
        if ( set_of_sets.count(words) == 0 ) {
        set_of_sets.insert(words);
    } else {
        ids_to_delete.push_back(document_id);
    }
 }
    
    
    
    for ( const auto& document_id : ids_to_delete ) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id " << document_id << std::endl;
    }
}