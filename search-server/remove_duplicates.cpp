#include "remove_duplicates.h"
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids_to_delete;
    std::map<std::set<std::string>, int> words_set_plus_id;
    for (const int document_id : search_server) {
        std::set<std::string> words; // временный контейнер
 //Нашёл похожий случай https://stackoverflow.com/questions/681943/how-can-i-get-a-stdset-of-keys-to-a-stdmap + помощь наставника
        transform( search_server.GetWordFrequencies(document_id).begin(), search_server.GetWordFrequencies(document_id).end(), inserter(words, words.begin() ), [] (const std::pair<std::string, double>& p) {
            return p.first; 
        } );
        if ( words_set_plus_id.count(words) == 0 ) {
        words_set_plus_id[words] = document_id;
    } else {
        ids_to_delete.push_back(document_id);
    }
 }
    
    for ( const auto& document_id : ids_to_delete ) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id " << document_id << std::endl;
    }
}