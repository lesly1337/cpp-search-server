#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
 
using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);
    
    return words;
}
    
struct Document {
    int id;
    double relevance;
};
 
class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    // Считает TF
    void AddDocument(int document_id, const string& document) {
        ++document_count_; //Увеличиваем общее количество документов
        for (const string& word : SplitIntoWordsNoStop(document)) {
                word_to_document_freqs_[word][document_id] += 1.0 / SplitIntoWordsNoStop(document).size(); //Добавляем TF для каждого слова в этом документе в словарь
        }
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const {        
        const Query query_word = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_word);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
private:
    map<string, map<int, double>> word_to_document_freqs_; //revelance теперь double
    set<string> stop_words_;
    int document_count_ = 0;
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    struct Query {
        vector<string> plus_words;
        vector<string> minus_words;
    };
    
    bool IsMinusWord ( string text ) const {
        bool is_minus = false;
        if (text[0] == '-' ) {
            is_minus = true;
        }
        return is_minus;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
  
    Query ParseQuery(const string& text) const {
        Query query;
        for ( string& word : SplitIntoWords(text)) {
            if ( IsStopWord(word) != true  ) {
                if ( IsMinusWord(word) == true ) {
                    word = word.substr(1);
                    query.minus_words.push_back(word);
                } else {
                    query.plus_words.push_back(word);
                }
            }
        }
        return query;
    }
 
    vector<Document> FindAllDocuments(const Query& query) const {
        //Меняем word_to_documents на word_to_document_freqs_
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double& idf = log(document_count_ * 1.0 / word_to_document_freqs_.at(word).size()); //Считаем IDF
            for (const auto [document_id, tf] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += idf * tf; //Повышаем релевантность с помощью TF-IDF
            }
        }
        //не меняем
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            
        for (const auto [document_id, tf] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
 
        vector<Document> matched_documents;
        for (auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance});
        }
        return matched_documents;
    }
};
 
 
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
 
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    
    return search_server;
}
 
 
int main() {
    const SearchServer search_server = CreateSearchServer();
 
    const string query = ReadLine();
    for (auto [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s
             << endl;
    }
 
    return 0;
}


