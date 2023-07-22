#include "yaml-cpp/yaml.h"
#include <stdexcept>
#include <iostream>

namespace YEDIT{

class YamlNodeMatchException : public std::exception{
public:
  YamlNodeMatchException(std::string currentNode, std::string accumulatedPath = ""):
    std::exception(), graphPath_(currentNode + ":" + accumulatedPath)
  {
  }
  const char* what() const noexcept override{
    return graphPath_.c_str();
  }
private:
  std::string graphPath_;
};

namespace{
void impl_find_and_replace(YAML::Node src, YAML::Node key){
  switch (key.Type()){
    case YAML::NodeType::Map:
      for(auto n : key){
        std::string k = static_cast<std::string>(n.first.Scalar());
        try{
          impl_find_and_replace(src[k], key[k]);
        }
        catch(YamlNodeMatchException& e)
        {
          throw YamlNodeMatchException(k, std::string(e.what()));
        }
      }
      break;
    case YAML::NodeType::Sequence:
      for(int i=0; i<key.size(); ++i){
        impl_find_and_replace(src[i], key[i]);
      }
      break;
    case YAML::NodeType::Scalar:
      {
      if(src.Type() != key.Type()){
          // we have a invalid path in the key
          throw YamlNodeMatchException("");
      }
      src = key;
      break;
      }
    default:
      break;
  }
}
}

void find_and_replace(YAML::Node src, YAML::Node key){
  try{
    impl_find_and_replace(src, key);
  }
  catch(YamlNodeMatchException& e){
    auto failingPath = static_cast<std::string>(e.what());
    std::string message = "\nFailure trying to replace YAML\nFailing Graph:\n\t";
    message+=failingPath+"\n";
    message+="Please double check and make sure this matches the source YAML";
    throw std::runtime_error(message);
  }
}

//case 1: it's a map
//- pass the contents of the map recursively
//case 2: it's a list
//- pass the contents of the list recursively
//case 3: it's a scalar
//- replace value

}
