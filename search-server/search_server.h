#pragma once  
#include <string>  
#include <vector>  
#include <map>  
#include <set>  
#include <cmath>   
#include <algorithm>  
#include <stdexcept>
#include <execution>
#include <future>
#include "document.h"  
#include "string_processing.h"  
#include "concurrent_map.h" 
  
class SearchServer {   
public:   
       
    SearchServer() = default;  
    inline static constexpr int INVALID_DOCUMENT_ID = -1;   
   
    template <typename StringContainer>   
    explicit SearchServer(const StringContainer& stop_words);  
   
    explicit SearchServer(const std::string& stop_words_text); 
    
    explicit SearchServer(std::string_view stop_words_text); //Добавлен ещё 1 конструктор
   
   
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);  
   
   //Добавляем параллельные версии FindTopDocuments
    template <typename DocumentPredicate>   
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;//1
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const; //2
    
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;//3
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;  //4
    
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;//5
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const; //6 
    
    
    int GetDocumentCount() const;  
   
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;  
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;
  
    std::set<int>::const_iterator begin(); 
     
    std::set<int>::const_iterator end(); 
     
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const; 
     
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
   
private:   
    struct DocumentData {   
        int rating;   
        DocumentStatus status;
        std::string string_;//добавлено поле хранения document как строку
    };   
    const std::set<std::string, std::less<>> stop_words_;  //чтобы избавиться от создания временных объектов 
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; 
    std::map<int, std::map<std::string_view, double>> documents_to_word_freqs_;  //Добавим контейнер с частотой слов по его id 
    std::map<int, DocumentData> documents_;   
    std::set<int> documents_order_num; // контейнер с порядковыми номерами   
   
   static bool IsValidWord(std::string_view word);  
   
   
    bool IsStopWord(std::string_view word) const;  
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;  
   
    static int ComputeAverageRating(const std::vector<int>& ratings);  
   
    struct QueryWord {   
        std::string_view data;
        bool is_minus;   
        bool is_stop;   
    };   
    //Обновлённый парсинг   
    QueryWord ParseQueryWord(std::string_view text) const;  
   
    struct Query { //Заменена на vector string_view
        std::vector<std::string_view> plus_words;   
        std::vector<std::string_view> minus_words;   
    };   
   
    Query ParseQuery(std::string_view text) const;
    Query ParseQueryParallel(std::string_view text) const;
   
    double ComputeWordInverseDocumentFreq(std::string_view word) const;  
    //Добавлены последовательная и параллельная версия FindAllDocuments
    template <typename DocumentPredicate>   
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;  
    
    template <typename DocumentPredicate>   
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&,const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>   
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;  
    
    };  
      
//Реализация  
      
    template <typename StringContainer>   
    SearchServer::SearchServer(const StringContainer& stop_words)   
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {   
            for ( const auto& word : stop_words) {   
                if ( !IsValidWord(word) ) {   
                    throw std::invalid_argument("Special symbol entered");   
                }   
            }   
    }   
      
    
    template <typename ExecutionPolicy, typename DocumentPredicate>   
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const { //2  
        const int MAX_RESULT_DOCUMENT_COUNT = 5;   
        const double MATH_ERROR = 1e-6;   
          
        Query query = ParseQuery(raw_query);   
   
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);   
   
        std::sort(policy, matched_documents.begin(), matched_documents.end(),   
             [&MATH_ERROR](const Document& lhs, const Document& rhs) {   
                 if (std::abs(lhs.relevance - rhs.relevance) < MATH_ERROR) {   
                     return lhs.rating > rhs.rating;   
                 }   
                     return lhs.relevance > rhs.relevance;   
             });   
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {   
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);   
        }   
   
        return matched_documents;   
    }
    
    
    template <typename DocumentPredicate>   
    std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {  //1 
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);   
    }
    template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {   
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {   
                return document_status == status;   
            });   //4
    }
    
    template <typename ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {   
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);   //6
    }

    //no policy 
    template <typename DocumentPredicate>   
    std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {   
        std::map<int, double> document_to_relevance;   
        for ( auto word : query.plus_words) {   
            if (word_to_document_freqs_.count(word) == 0) {   
                continue;   
            }   
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);   
            for (auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {   
                const auto& document_data = documents_.at(document_id);   
                if (document_predicate(document_id, document_data.status, document_data.rating)) { 
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;   
                }   
            }   
        }   
   
        for (std::string_view word : query.minus_words) {   
            if (word_to_document_freqs_.count(word) == 0) {   
                continue;   
            }   
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {   
                document_to_relevance.erase(document_id);   
            }   
        }   
   
        std::vector<Document> matched_documents;   
        for (const auto [document_id, relevance] : document_to_relevance) {   
            matched_documents.push_back(   
                {document_id, relevance, documents_.at(document_id).rating});   
        }   
        return matched_documents;   
    }
    //seq
    template <typename DocumentPredicate>   
    std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
        return FindAllDocuments(query, document_predicate);
    }
    
    //parallel
    template <typename DocumentPredicate>   
    std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query, DocumentPredicate document_predicate) const {   
        std::vector<Document> matched_documents;
        ConcurrentMap<int, double> document_to_relevance(20);//разбиваем на 20 словарей
        
        std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [this, &document_to_relevance, &document_predicate] (std::string_view word) {//пробегаемся по + словам
            if (word_to_document_freqs_.count(word)) {   
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);   
            for (auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);   
                if (document_predicate(document_id, document_data.status, document_data.rating)) {//попытался сделать асинхронно, но выдаёт ошибку  
                       document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }   
         }   
      });
        std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, &document_to_relevance, &document_predicate] (std::string_view word) {//пробегаемся по - словам
            if (word_to_document_freqs_.count(word) == 0) {   
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {//попытался сделать асинхронно, но выдаёт ошибку  
                    document_to_relevance.Erase(document_id);  
            }    
         }   
      });
       
        
            for ( auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap() ) {
                matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
            }
        return matched_documents;   
    }
