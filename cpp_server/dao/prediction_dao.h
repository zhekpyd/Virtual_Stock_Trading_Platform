#pragma once
#include "model/prediction.h"
#include <vector>
#include <string>

namespace prediction_dao {

void insert(const Prediction& p);
std::vector<Prediction> get_for_code(const std::string& code);
std::vector<Prediction> get_pending(int day_offset);
void verify(int id, double actual_price);

}  // namespace prediction_dao