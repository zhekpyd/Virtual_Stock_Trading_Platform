#include "service/kline_fetcher.h"
#include "utils/http_client.h"
#include <nlohmann/json.hpp>
#include <ctime>
#include <iostream>
#include <algorithm>

std::vector<StockHistory> fetch_kline(const std::string& code, const std::string& name,
                                       const std::string& type, const std::string& beg_date) {
    std::vector<StockHistory> result;

    std::string symbol = (code[0] == '6') ? "sh" + code : "sz" + code;
    int datalen = (type == "daily") ? 30 : 250;
    std::string url = "http://money.finance.sina.com.cn/quotes_service/api/json_v2.php/"
                      "CN_MarketData.getKLineData?symbol=" + symbol +
                      "&scale=240&ma=no&datalen=" + std::to_string(datalen);

    std::string data = http_get(url);
    std::cout << "K线HTTP返回: " << data.length() << " 字节, symbol=" << symbol << std::endl;
    if (data.empty()) return result;

    auto json = nlohmann::json::parse(data);
    if (!json.is_array()) return result;

    for (const auto& item : json) {
        StockHistory h;
        h.trade_date  = item.value("day", "");
        h.open_price  = std::stod(item.value("open", "0"));
        h.close_price = std::stod(item.value("close", "0"));
        h.high_price  = std::stod(item.value("high", "0"));
        h.low_price   = std::stod(item.value("low", "0"));
        h.volume      = std::stoll(item.value("volume", "0"));
        result.push_back(h);
    }

    if (type == "weekly" || type == "monthly") {
        std::reverse(result.begin(), result.end());
        std::vector<StockHistory> agg;
        StockHistory cur = result[0];
        for (size_t i = 1; i < result.size(); i++) {
            bool split = false;
            if (type == "weekly") {
                // 判断 result[i] 是不是周一，是就劈新蜡烛
                int y, m, d;
                sscanf(result[i].trade_date.c_str(), "%d-%d-%d", &y, &m, &d);
                struct tm tm = {}; tm.tm_year=y-1900; tm.tm_mon=m-1; tm.tm_mday=d; tm.tm_isdst=-1;
                mktime(&tm);
                split = (tm.tm_wday == 1);
            } else {
                // 月份变了
                split = (cur.trade_date.substr(0,7) != result[i].trade_date.substr(0,7));
            }

            if (split) {
                agg.push_back(cur);
                cur = result[i];
            } else {
                if (result[i].high_price > cur.high_price) cur.high_price = result[i].high_price;
                if (result[i].low_price  < cur.low_price)  cur.low_price  = result[i].low_price;
                cur.close_price = result[i].close_price;
                cur.volume += result[i].volume;
            }
        }
        agg.push_back(cur);
        result = agg;
        std::cout << "聚合后: " << result.size() << " 条" << std::endl;
    }

    return result;
}