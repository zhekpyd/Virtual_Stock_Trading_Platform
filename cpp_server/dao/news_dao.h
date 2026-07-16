#pragma once
#include <string>
#include <vector>
#include "model/news_analysis.h"

namespace news_dao {
bool exists(int source_id);
void insert(const NewsAnalysis& n);
std::vector<NewsAnalysis> get_all();
std::vector<NewsAnalysis> get_recent(int hours = 24);
}