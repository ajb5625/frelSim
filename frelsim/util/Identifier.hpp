#pragma once
#include <string>

namespace frelsim::util {

class Identifier final {
    public: 
        Identifier() = default;

        Identifier(const std::string& uri);

        const std::string& getUri() const;

        void setDomain(const std::string& domain);

        void setScope(const std::string& scope);

        void setName(const std::string& name);

        const std::string& getDomain() const;

        const std::string& getScope() const;

        const std::string& getName() const;

    private:
        std::string domain_;

        std::string scope_;

        std::string name_;

        std::string uri_; 

};

} // frelsim::util