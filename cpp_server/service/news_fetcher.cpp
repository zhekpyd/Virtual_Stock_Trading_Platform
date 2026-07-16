#include "service/news_fetcher.h"
#include "utils/http_client.h"
#include "dao/news_dao.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <iostream>

static std::string ollama_tag(const std::string& title, const std::string& content) {
    std::string prompt =
        "你是一位专业股票分析师。分析以下新闻，严格返回JSON格式（不要其他内容）：\n"
        "{\n"
        "  \"tags\":[\"政策解读\"],\n"
        "  \"summary\":\"一句话摘要\",\n"
        "  \"stocks\":[\n"
        "    {\"code\":\"002594\",\"name\":\"比亚迪\",\"score\":0.8},\n"
        "    {\"code\":\"601088\",\"name\":\"中国神华\",\"score\":-0.4}\n"
        "  ]\n"
        "}\n"
        "规则：\n"
        "- tags: 选两个最相关标签（财经新闻,政治新闻,时事新闻,科技新闻,社会热点,国际新闻,行业新闻,公司新闻,政策解读,市场分析）\n"
        "- summary: 一句话总结\n"
        "- stocks: 新闻可能影响的所有股票及情绪分（-1利空到1利好），没有就空数组\n"
        "- 考虑宏观经济政策、行业趋势、公司基本面。领导讲话、行业政策都要联想到相关股票\n\n"
        "标题：" + title + "\n内容：" + content;

    nlohmann::json req;
    req["model"] = "qwen2.5:7b";
    req["prompt"] = prompt;
    req["stream"] = false;

    std::ofstream jf("_news_body.json");
    jf << req.dump();
    jf.close();

    std::string cmd;
#ifdef _WIN32
    cmd = "curl.exe -s -X POST http://localhost:11434/api/generate "
          "-H \"Content-Type: application/json\" -d @_news_body.json 2>nul";
#else
    cmd = "curl -s -X POST http://localhost:11434/api/generate "
          "-H \"Content-Type: application/json\" -d @_news_body.json 2>/dev/null";
#endif
    std::string result;
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (pipe) { char buf[1024]; while (fgets(buf, sizeof(buf), pipe)) result += buf;
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    }
    std::remove("_news_body.json");

    try {
        auto j = nlohmann::json::parse(result);
        std::string text = j.value("response", "");
        size_t s = text.find('{'), e = text.rfind('}');
        if (s != std::string::npos && e != std::string::npos) {
            auto t = nlohmann::json::parse(text.substr(s, e - s + 1));
            return t.dump();   // {"tags":[...],"summary":"..."}
        }
    } catch (...) {}
    return "{}";
}

void fetch_and_analyze_news() {
    std::cout << "[新闻] 开始拉取新闻列表..." << std::endl;
    std::string ids_json = http_get("http://26.34.251.168:18080/api/news/allIds");
    if (ids_json.empty()) return;

    auto j = nlohmann::json::parse(ids_json);
    auto list = j["data"];
    if (!list.is_array()) return;

    int total = 0, skipped = 0;
    for (auto& id_val : list) {
        int news_id = id_val.get<int>();
        if (news_dao::exists(news_id)) { skipped++; continue; }

        std::string detail = http_get("http://26.34.251.168:18080/api/news/detail?newsId=" + std::to_string(news_id));
        if (detail.empty()) continue;
        auto d = nlohmann::json::parse(detail)["detail"];

        std::string title   = d.value("title", "");
        std::string content = d.value("content", "");
        // 简单去 HTML
        std::string clean; bool in = false;
        for (char c : content) { if (c=='<') in=true; else if (c=='>') in=false; else if (!in) clean+=c; }

        std::string tags_json = ollama_tag(title, clean);

        NewsAnalysis na;
        na.source_id = news_id;
        na.title     = title;
        na.content   = clean;
        try {
            auto tj = nlohmann::json::parse(tags_json);
            na.tags = tj["tags"].dump();
            if (tj.contains("summary")) na.summary = tj["summary"].get<std::string>();
            if (tj.contains("stocks")) na.stocks_impact = tj["stocks"].dump();
            else na.stocks_impact = "[]";
        } catch (...) {
            na.tags = "[]";
            na.summary = tags_json;
            na.stocks_impact = "[]";
        }
        news_dao::insert(na);
        total++;
        std::cout << "[新闻] #" << news_id << " " << title << " 分析完成" << std::endl;
    }
    std::cout << "[新闻] 新增 " << total << " 条，跳过 " << skipped << " 条" << std::endl;
}