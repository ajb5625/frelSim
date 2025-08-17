#pragma once
#include "../adapt/SimAdapter.hpp"

namespace frelsim::simulation {

class FrelsimAdapter : public adapt::SimAdapter {

    public:
        FrelsimAdapter();

        ~FrelsimAdapter() override;

        bool stepUntil(double stopTime) override;

        Values get(Identifiers ids) const override;

        void set(SetOperations ops) override;

};

} // frelsim::simulation