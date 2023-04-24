#include "search_server.h" 
 
SearchServer::SearchServer(const std::string& stop_words_text)  
        : SearchServer::SearchServer(  
            SplitIntoWords(stop_words_text))   
    {  
    } 
 //Обновлённое добавление документа
void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {  
        if (!IsValidWord(document)) {   
            throw std::invalid_argument("Special symbol entered");  
        } else if ( document_id < 0 ) {   
            throw std::invalid_argument("Negative id entered");  
        } else if ( documents_.count(document_id) != 0) {   
            throw std::invalid_argument("Existing id entered");  
        }  
  
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);  
        const double inv_word_count = 1.0 / words.size();  
        for (const std::string& word : words) {  
            word_to_document_freqs_[word][document_id] += inv_word_count;  
            documents_to_word_freqs_[document_id][word] += inv_word_count;  // Добавляем в поле частоту слова по id
        }  
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});  
        documents_order_num.insert(document_id);  
    } 
 
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {  
        return FindTopDocuments(  
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {  
                return document_status == status;  
            });  
    } 
 
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {  
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);  
    }  
 
int SearchServer::GetDocumentCount() const {  
        return documents_.size();  
    }  
 
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {  
        Query query = ParseQuery(raw_query);  
  
        std::vector<std::string> matched_words;  
        for (const std::string& word : query.plus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            if (word_to_document_freqs_.at(word).count(document_id)) {  
                matched_words.push_back(word);  
            }  
        }  
        for (const std::string& word : query.minus_words) {  
            if (word_to_document_freqs_.count(word) == 0) {  
                continue;  
            }  
            if (word_to_document_freqs_.at(word).count(document_id)) {  
                matched_words.clear();  
                break;  
            }  
        }  
        return { matched_words, documents_.at(document_id).status };  
    }  
 
bool SearchServer::IsValidWord(const std::string& word) {  
        return none_of(word.begin(), word.end(), [](char c) {  
            return c >= '\0' && c < ' ';  
        });       
   } 
bool SearchServer::IsStopWord(const std::string& word) const {  
        return stop_words_.count(word) > 0;  
    }  
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {  
        std::vector<std::string> words;  
        for (const std::string& word : SplitIntoWords(text)) {  
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
 
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {  
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
 
SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {  
        Query result;  
        for (const std::string& word : SplitIntoWords(text)) {  
            QueryWord query_word = ParseQueryWord(word);;  
            if (!query_word.is_stop) {  
                if (query_word.is_minus) {  
                    result.minus_words.insert(query_word.data);  
                } else {  
                    result.plus_words.insert(query_word.data);  
                }  
            }  
        }  
        return result;  
    } 
 
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {  
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());  
    } 

std::set<int>::const_iterator SearchServer::begin() {
        return documents_order_num.begin();
    }

std::set<int>::const_iterator SearchServer::end() {
        return documents_order_num.end();
    }

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if ( documents_to_word_freqs_.count(document_id) != 0 ) {
        return documents_to_word_freqs_.at(document_id);
    }
    static const std::map<std::string, double> empty_map;
    return empty_map;
}

void SearchServer::RemoveDocument(int document_id) {
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


