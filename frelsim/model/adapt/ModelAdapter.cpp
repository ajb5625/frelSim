#include "ModelAdapter.hpp"

namespace frelsim::model::adapt {

ModelAdapter::ModelAdapter(sim::proto::SimulationDescription simDescription) : simDescription_(simDescription) {
    instance_ = std::make_unique<core::Model>(simDescription_);
}

ModelAdapter::~ModelAdapter() {
    instance_.reset();
}

bool ModelAdapter::stepUntil(double stopTime) {
    return instance_->stepUntil(stopTime);
}

Values ModelAdapter::get(Identifiers ids) const {
    Identifiers outputs;
    Identifiers parameters;
    Values values;
    for (const auto& id : ids) {
        if (id.getScope() == "Parameter") {
            parameters.push_back(id);
        }
        else if (id.getScope() == "Output") {
            outputs.push_back(id);
        }
    }
    values = instance_->getParameters(parameters);
    auto outputsValues = instance_->getOutputs(outputs);
    values.insert(values.end(), outputsValues.begin(), outputsValues.end());
    return values;
}

void ModelAdapter::set(SetOperations ops) {
    SetOperations inputs;
    SetOperations params;
    for (const auto& op : ops) {
        if (op.first.getScope() == "Parameter") {
            params.push_back(op);
        }
        else if (op.first.getScope() == "Input") {
            inputs.push_back(op);
        }
    }
    instance_->setParameters(params);
    instance_->setInputs(inputs);
}


} // frelsim::simulation