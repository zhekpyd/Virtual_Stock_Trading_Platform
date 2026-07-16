#pragma once
#include <crow.h>

namespace predict_ctrl {
crow::response get_predictions(const crow::request& req);
}