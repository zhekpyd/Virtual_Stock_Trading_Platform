#pragma once
#include <string>

struct NewsAnalysis {
    int id;
    int source_id;
    std::string title;
    std::string content;
    std::string tags;
    std::string summary;
    std::string created_at;
    std::string stocks_impact;  // JSON数组
};