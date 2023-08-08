#include "search_server.h"  
  
SearchServer::SearchServer(const std::string& stop_words_text)   
        : SearchServer::SearchServer(   
            SplitIntoWords(stop_words_text))    
    {   
    } 

SearchServer::SearchServer(std::string_view stop_words_text)   
        : SearchServer::SearchServer(   
            SplitIntoWords(stop_words_text))    
    {   
    }
 //Обновлённое добавление документа 
void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {   
        if (!IsValidWord(document)) {    
            throw std::invalid_argument("Special symbol entered");   
        } else if ( document_id < 0 ) {    
            throw std::invalid_argument("Negative id entered");   
        } else if ( documents_.count(document_id) != 0) {    
            throw std::invalid_argument("Existing id entered");   
        }   
   
        auto words = SplitIntoWordsNoStop(document);
        std::string document_string_(document);
        const double inv_word_count = 1.0 / words.size();   
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, document_string_});
        words = SplitIntoWordsNoStop(documents_.at(document_id).string_);   
        for ( auto word : words) {   
            word_to_document_freqs_[word][document_id] += inv_word_count;   
            documents_to_word_freqs_[document_id][word] += inv_word_count;  // Добавляем в поле частоту слова по id 
        }
        documents_order_num.emplace(document_id);   
    }  
  
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {   
        return FindTopDocuments(std::execution::seq, raw_query, status);  
    }  
  
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {   
        return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);   
    }   
  
int SearchServer::GetDocumentCount() const {   
        return static_cast<int>(documents_.size());
    }   
  
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
        if ((document_id < 0) || (documents_.count(document_id) == 0)) {
            throw std::invalid_argument("Id is not found");
        }
        Query query = ParseQuery(raw_query);   
        std::vector<std::string_view> matched_words;   
        for (std::string_view word : query.minus_words) {   
            if (word_to_document_freqs_.count(word) == 0) {   
                continue;   
            }   
            if (word_to_document_freqs_.at(word).count(document_id)) {   
                return { {}, documents_.at(document_id).status };  
            }   
        }    
        for (std::string_view word : query.plus_words) {   
            if (word_to_document_freqs_.count(word) == 0) {   
                continue;   
            }   
            if (word_to_document_freqs_.at(word).count(document_id)) {   
                matched_words.push_back(word);   
            }   
        }  
        return { matched_words, documents_.at(document_id).status };   
    }  

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {   
        return MatchDocument(raw_query, document_id);
    } 

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const { 
        if ((document_id < 0) || (documents_.count(document_id) == 0)) {
            throw std::invalid_argument("Id is not found");
        }
        Query query = ParseQueryParallel(raw_query);   
        std::vector<std::string_view> matched_words;
        auto var = documents_to_word_freqs_.at(document_id);//требует поместить в переменную для захвата в лямда-функцию
        if (std::any_of(std::execution::par, query.minus_words.begin(),//проверка на минус-слова
                    query.minus_words.end(), [&var](std::string_view word) {
                        return var.count(word) > 0;
                    })) {
            return { {}, documents_.at(document_id).status };
        }
        matched_words.reserve(query.plus_words.size());//задаём размер вектора
        std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), std::back_inserter(matched_words), [&var] (std::string_view word) {
                        return var.count(word) > 0;
                    } );
        std::sort(std::execution::par, matched_words.begin(), matched_words.end());//также убираем дубликаты
        auto new_end = std::unique(matched_words.begin(), matched_words.end());
        matched_words.resize(static_cast<size_t>( new_end - matched_words.begin() ));
        return { matched_words, documents_.at(document_id).status };
    } 
  
bool SearchServer::IsValidWord(std::string_view word) {   
        return std::none_of(word.begin(), word.end(), [](char c) {   
            return c >= '\0' && c < ' ';   
        });        
   }  
bool SearchServer::IsStopWord(std::string_view word) const {   
        return stop_words_.count(word) > 0;   
    }   
std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {   
        std::vector<std::string_view> words;   
        for (const auto& word : SplitIntoWords(text)) { 
            if (!IsValidWord(word)) {    
            throw std::invalid_argument("Special symbol entered");   
        }
            if (!IsStopWord(word)) {   
                words.push_back(word);   
            }   
        }   
        return words;   
    }   
int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {   
        if (ratings.empty()) {   
            return 0;   
        }   
        int rating_sum = 0;   
        for (const int rating : ratings) {   
            rating_sum += rating;   
        }   
        return rating_sum / static_cast<int>(ratings.size());   
    }  
  
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {   
        bool is_minus = false;   
        if (text[0] == '-') {   
            is_minus = true;   
            text = text.substr(1);   
        }   
        if (text.empty()) {   
            throw std::invalid_argument("No text after minus");   
        } else if (!IsValidWord(text)) {   
            throw std::invalid_argument("Special symbol entered");   
        } else if (text[0] == '-') {   
            throw std::invalid_argument("More than 1 minus entered");   
        }   
        return { text, is_minus, IsStopWord(text) };   
    }  
  
SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
        Query result;   
        for (std::string_view word : SplitIntoWords(text)) {   
            QueryWord query_word = ParseQueryWord(word);;   
            if (!query_word.is_stop) {   
                if (query_word.is_minus) {   
                    result.minus_words.push_back(query_word.data);//меняем insert на push_back
                } else {   
                    result.plus_words.push_back(query_word.data);//меняем insert на push_back
                }   
            }   
        }
        std::sort(result.minus_words.begin(), result.minus_words.end());//сортируем, чтобы повторяющиеся элементы шли по порядку
        auto minus_end = std::unique(result.minus_words.begin(), result.minus_words.end());//удаляем дубликаты и возвращаем итератор на новый конец словаря
        result.minus_words.resize(static_cast<size_t>( minus_end - result.minus_words.begin() ));//убираем пустые элементы
        std::sort(result.plus_words.begin(), result.plus_words.end());
        auto plus_end = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.resize(static_cast<size_t>( plus_end - result.plus_words.begin() ));
        return result;   
    }  
  
SearchServer::Query SearchServer::ParseQueryParallel(const std::string_view text) const {   
        Query result;   
        for (std::string_view word : SplitIntoWords(text)) {   
            QueryWord query_word = ParseQueryWord(word);;   
            if (!query_word.is_stop) {   
                if (query_word.is_minus) {   
                    result.minus_words.push_back(query_word.data);   
                } else {   
                    result.plus_words.push_back(query_word.data);   
                }   
            }   
        }   
        return result;   
    } 

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {   
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());   
    }  
 
std::set<int>::const_iterator SearchServer::begin() { 
        return documents_order_num.begin(); 
    } 
 
std::set<int>::const_iterator SearchServer::end() { 
        return documents_order_num.end(); 
    } 
 
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const { 
    if ( documents_to_word_freqs_.count(document_id) != 0 ) { 
        return documents_to_word_freqs_.at(document_id); 
    } 
    static const std::map<std::string_view, double> empty_map; 
    return empty_map; 
} 
 
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) { 
    if ( documents_to_word_freqs_.count(document_id) != 0 ) { 
    documents_.erase(document_id); 
 
        std::set<int>::iterator position = std::find(documents_order_num.begin(), documents_order_num.end(), document_id); 
 
        documents_order_num.erase(position); 
 
    for ( auto [word, frequency] : documents_to_word_freqs_[document_id] ) { 
            word_to_document_freqs_[word].erase(document_id); 
            if ( word_to_document_freqs_[word].empty() ) { 
                word_to_document_freqs_.erase(word); 
            } 
        }     
    documents_to_word_freqs_.erase(document_id); 
    } 
} 
 
void SearchServer::RemoveDocument(int document_id) {
    SearchServer::RemoveDocument( std::execution::seq, document_id);
}
 
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) { 
    if ( documents_to_word_freqs_.count(document_id) != 0 ) { 
        documents_.erase(document_id); 
        std::vector<std::string_view> words;//вектор ключей
        const auto& var = documents_to_word_freqs_.at(document_id);
        words.reserve(var.size());
        std::transform(std::execution::par, var.begin(), var.end(), words.begin(), [] (const auto& word) {
            return word.first;
        } );//записываем адрес слов, встречающихся в документе
        std::for_each(std::execution::par, words.begin(), words.end(), [this, document_id] (std::string_view word) {
            word_to_document_freqs_.at(word).erase(document_id);
        });
        std::set<int>::iterator position = std::find(documents_order_num.begin(), documents_order_num.end(), document_id);
        if ( position != documents_order_num.end() ) {
        documents_order_num.erase(position); 
        };
        documents_to_word_freqs_.erase(document_id);
    } 
}