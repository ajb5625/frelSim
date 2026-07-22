#include "ModelAdapter.hpp"

namespace frelsim::model::adapt {

ModelAdapter::ModelAdapter(sim::proto::SimulationDescription const& simDescription) : simDescription_(simDescription) {
    instance_ = factory::createModel(simDescription_);
    instance_->initialize();
}

ModelAdapter::~ModelAdapter() {
    instance_.reset();
}

double ModelAdapter::guaranteeUntil(double maxTime) {
    return instance_->guaranteeUntil(maxTime);
}

bool ModelAdapter::stepUntil(double stopTime) {
    return instance_->stepUntil(stopTime);
}

Values ModelAdapter::get(Identifiers ids) const {
    Identifiers outputs;
    Identifiers parameters;
    Identifiers inputs;
    Values values;
    for (const auto& id : ids) {
        if (id.getScope() == "Parameter") {
            parameters.push_back(id);
        }
        else if (id.getScope() == "Output") {
            outputs.push_back(id);
        }
        else if (id.getScope() == "Input") {
            inputs.push_back(id);
        }
    }
    values = instance_->getParameters(parameters);
    auto outputsValues = instance_->getOutputs(outputs);
    values.insert(values.end(), outputsValues.begin(), outputsValues.end());
    auto inputsValues = instance_->getInputs(inputs);
    values.insert(values.end(), inputsValues.begin(), inputsValues.end());
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


} // frelsim::model::adapt
