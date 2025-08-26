#pragma once
#include "../core/Model.hpp"

namespace frelsim::model::factory {

std::unique_ptr<core::Model> createModel(std::string const& modelType); 

} // frelsim::model::factory