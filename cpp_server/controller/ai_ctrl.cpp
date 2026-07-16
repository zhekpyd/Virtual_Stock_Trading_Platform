#include "controller/ai_ctrl.h"
#include "dao/stock_dao.h"
#include "utils/http_client.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <cctype>

namespace ai_ctrl {

crow::response analyze(const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    std::string question = body.value("question", "");
    if (question.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 question";
        return crow::response(400, resp.dump());
    }

// 自动识别问题中的股票（6位代码或名字）
    std::string context;
    auto stocks = stock_dao::get_all_stocks();

    // 先扫6位数字代码
    for (size_t i = 0; i + 6 <= question.length(); i++) {
        if (isdigit(question[i]) && isdigit(question[i+1]) && isdigit(question[i+2]) &&
            isdigit(question[i+3]) && isdigit(question[i+4]) && isdigit(question[i+5])) {
            std::string found = question.substr(i, 6);
            for (const auto& s : stocks) {
                if (s.code == found) {
                    context = "[股票信息]\n名称: " + s.name + "\n代码: " + found +
                        "\n当前价: " + std::to_string(s.price) +
                        "\n涨跌幅: " + std::to_string(s.change_pct) + "%" +
                        "\n今日最高: " + std::to_string(s.high_day) +
                        "\n今日最低: " + std::to_string(s.low_day) + "\n\n";
                    break;
                }
            }
            break;
        }
    }

    // 代码没找到就扫名字
    if (context.empty()) {
        for (const auto& s : stocks) {
            if (!s.name.empty() && question.find(s.name) != std::string::npos) {
                context = "[股票信息]\n名称: " + s.name + "\n代码: " + s.code +
                    "\n当前价: " + std::to_string(s.price) +
                    "\n涨跌幅: " + std::to_string(s.change_pct) + "%" +
                    "\n今日最高: " + std::to_string(s.high_day) +
                    "\n今日最低: " + std::to_string(s.low_day) + "\n\n";
                break;
            }
        }
    }

    // 附上市场概况，AI 自行判断是否使用
    if (!stocks.empty()) {
        std::vector<Stock> sorted = stocks;
        std::sort(sorted.begin(), sorted.end(), [](const Stock& a, const Stock& b) {
            return a.change_pct > b.change_pct;
        });
        int n = std::min(10, (int)sorted.size());
        context += "[市场概况]\n涨幅前" + std::to_string(n) + ":\n";
        for (int i = 0; i < n; i++) {
            context += sorted[i].name + "(" + sorted[i].code + ") +" +
                std::to_string(sorted[i].change_pct).substr(0,5) + "% 价" +
                std::to_string(sorted[i].price).substr(0,7) + "\n";
        }
        context += "\n跌幅前10:\n";
        for (int i = std::max(0, (int)sorted.size() - 10); i < (int)sorted.size(); i++) {
            context += sorted[i].name + "(" + sorted[i].code + ") " +
                std::to_string(sorted[i].change_pct).substr(0,5) + "% 价" +
                std::to_string(sorted[i].price).substr(0,7) + "\n";
        }
        context += "\n";
    }

    std::string prompt = context + "[用户提问]\n" + question;

    // 构造 JSON 请求体写入临时文件
    nlohmann::json ollama_req;
    ollama_req["model"] = "qwen2.5:7b";
    ollama_req["prompt"] = prompt;
    ollama_req["stream"] = false;
    std::ofstream jf("_ai_body.json");
    jf << ollama_req.dump();
    jf.close();

    #ifdef _WIN32
        std::string cmd = "curl.exe -s -X POST http://localhost:11434/api/generate "
                          "-H \"Content-Type: application/json\" "
                          "-d @_ai_body.json 2>nul";
    #else
        std::string cmd = "curl -s -X POST http://localhost:11434/api/generate "
                          "-H \"Content-Type: application/json\" "
                          "-d @_ai_body.json 2>/dev/null";
    #endif


    std::string result;
    #ifdef _WIN32
        FILE* pipe = _popen(cmd.c_str(), "r");
    #else
        FILE* pipe = popen(cmd.c_str(), "r");
    #endif
    if (pipe) {
        char buf[1024];
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        #ifdef _WIN32
            _pclose(pipe);
        #else
            pclose(pipe);
        #endif

    }
    std::remove("_ai_body.json");


    // 解析 JSON 提取 response 字段
    try {
        auto ai_json = nlohmann::json::parse(result);
        result = ai_json.value("response", result);
    } catch(...) {}

    nlohmann::json resp;
    if (result.empty()) {
        resp["success"] = false;
        resp["message"] = "AI 服务未启动";
        return crow::response(500, resp.dump());
    }
    resp["success"] = true;
    resp["analysis"] = result;
    return crow::response(200, resp.dump());
}

}  // namespace ai_ctrl