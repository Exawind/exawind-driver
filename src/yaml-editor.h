#include "yaml-cpp/yaml.h"
#include <stdexcept>
#include <iostream>

namespace YEDIT {

/* Exception if the graph is not formatted correctly
 * This exception is designed to reproduce the failing portion of the YAML graph
 * when called recursively so it can be echo'd to the user
 */
class YamlNodeMatchException : public std::exception
{
public:
    YamlNodeMatchException(
        std::string currentNode, std::string accumulatedPath = "")
        : std::exception()
    {
        std::string extra =
            accumulatedPath.empty() ? "" : ":" + accumulatedPath;
        graphPath_ = currentNode + extra;
    }
    const char* what() const noexcept override { return graphPath_.c_str(); }

private:
    std::string graphPath_;
};

namespace {
/* A function that will traverse the src node based on the key node
 * The two nodes must be identical until the final leaf values and then the
 * values of the key will override the src values. If the two graphs don't match
 * at any point prior to the leaves then it will trigger a
 * YamlNodeMatchException.
 */
inline void impl_find_and_replace(YAML::Node src, YAML::Node key)
{
    switch (key.Type()) {
    case YAML::NodeType::Map:
        // case 1: it's a map
        //- pass the contents of the map recursively
        for (auto n : key) {
            std::string k = static_cast<std::string>(n.first.Scalar());
            try {
                impl_find_and_replace(src[k], key[k]);
            } catch (YamlNodeMatchException& e) {
                throw YamlNodeMatchException(k, std::string(e.what()));
            }
        }
        break;
    case YAML::NodeType::Sequence:
        // case 2: it's a list
        //- pass the contents of the list recursively (order matters when
        //looking for a match)
        for (int i = 0; i < key.size(); ++i) {
            try {
                impl_find_and_replace(src[i], key[i]);
            } catch (YamlNodeMatchException& e) {
                throw YamlNodeMatchException(
                    "[" + std::to_string(i) + "]", std::string(e.what()));
            }
        }
        break;
    case YAML::NodeType::Scalar: {
        // case 3: it's a scalar
        //- replace value
        if (src.Type() != key.Type()) {
            // we have a invalid path in the key
            throw YamlNodeMatchException(key.as<std::string>());
        }
        src = key;
        break;
    }
    default:
        break;
    }
}
} // namespace

/* The accessible version of impl_find_and_replace that collects the final
 * error and makes it more readable
 */
inline void find_and_replace(YAML::Node src, YAML::Node key)
{
    try {
        impl_find_and_replace(src, key);
    } catch (YamlNodeMatchException& e) {
        auto failingPath = static_cast<std::string>(e.what());
        std::string message =
            "\nFailure trying to replace YAML\nFailing Graph:\n\t";
        message += failingPath + "\n";
        message +=
            "Please double check and make sure this matches the source YAML";
        throw std::runtime_error(message);
    }
}

} // namespace YEDIT
