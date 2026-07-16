#include "controller/predict_ctrl.h"
#include "dao/prediction_dao.h"
#include <nlohmann/json.hpp>

namespace predict_ctrl {

crow::response get_predictions(const crow::request& req) {
    const char* c = req.url_params.get("code");
    std::string code = c ? c : "";
    if (code.empty()) {
        nlohmann::json resp;
        resp["success"] = false;
        resp["message"] = "缺少 code";
        return crow::response(400, resp.dump());
    }

    auto preds = prediction_dao::get_for_code(code);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& p : preds) {
        nlohmann::json item;
        item["day"]            = p.day_offset;
        item["predicted_rate"] = p.predicted_rate;
        item["confidence"]     = p.confidence;
        item["price_now"]      = p.price_now;
        item["actual_price"]   = p.actual_price;
        item["actual_rate"]    = p.actual_rate;
        item["status"]         = p.status;
        arr.push_back(item);
    }

    nlohmann::json resp;
    resp["success"]     = true;
    resp["predictions"] = arr;
    return crow::response(200, resp.dump());
}

}  // namespace predict_ctrl