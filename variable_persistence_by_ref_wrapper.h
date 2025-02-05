#ifndef _VARIABLE_PERSISTENCE_BY_REF_WRAPPER_H
#define _VARIABLE_PERSISTENCE_BY_REF_WRAPPER_H

#include <tuple>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>
#include "json.hpp"

#define _ENABLE_VP_TEST 0 // enable debug print

#define _VP_ErrorPrint(obj) \
{ \
    std::cerr << (obj) << '\t' << __FILE__ << ':' << __LINE__ << '\t' << std::endl; \
}

#if _ENABLE_VP_TEST
    #define _VP_DebugPrint(obj) \
    { \
        std::cout << (obj) << '\t' << __FILE__ << ':' << __LINE__ << '\t' << std::endl; \
    }
#else
    #define _VP_DebugPrint(obj) {}
#endif

#define GET_C_STRING(obj)   #obj
#define GET_CXX_STRING(obj) std::string(#obj)

namespace VariablePersistencer {
template<typename T>
bool JsonGetToStrict(const nlohmann::json & j, const std::string & key_name, T & output_val) {
  auto iter = j.find(key_name);
  if (iter == j.end()) {
    _VP_ErrorPrint("key missing, keyname: " + key_name);
    _VP_ErrorPrint("from json: " + j.dump());
    return false;
  }
  j.at(key_name).get_to(output_val);
  return true;
}

// persist variables to disk in json form
// use variables's reference

// how to use:
// TestClassToSave class_test = TestClassToSave(114, "test string content", "test string in sub class content");
// using PersistType = VariablePersistencer::VariablePersistencerByRefWrapper<TestClassToSave>;
// PersistType upload_persistence_db_ = PersistType(std::vector<std::string>({ GET_CXX_STRING(class_test) }), std::make_tuple(std::ref(class_test)), "./json_db.txt");
template<typename... Ttupletypes>
class VariablePersistencerByRefWrapper {
  std::vector<std::string> persist_variable_names_; // variables' names
  std::tuple<std::reference_wrapper<Ttupletypes>...> persist_variables_refwrapper_; // variable's references, each reference need to/from json method
  std::string db_path_; // database path for persisting
  std::mutex mutex_;

 public:
  VariablePersistencerByRefWrapper() = default;
  VariablePersistencerByRefWrapper(std::vector<std::string> persist_variable_names,
                                    std::tuple<std::reference_wrapper<Ttupletypes>...> persist_variables_refwrappers,
                                    const std::string & db_path = "./json_db.txt")
    : persist_variable_names_(persist_variable_names),
      persist_variables_refwrapper_(persist_variables_refwrappers),
      db_path_(db_path) {
    if(db_path_ == "") {
      _VP_ErrorPrint("warning: VariablePersistencer db_path_ set as empty, replace to default: ./json_db.txt");
      db_path_ = "./json_db.txt";
    }
  }

  // load variables from datebase, try to load as much as possible when some of them not in json
  // return - true: all variables load success
  //          false: at least one variable load fail
  bool DoLoad() {
    if (db_path_.empty()) {
      _VP_ErrorPrint("");
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream infile(db_path_.c_str());
    if (!infile) {
      _VP_ErrorPrint("db json file not exist or cant open :" + db_path_ + " , load nothing");
      return false;
    }
    std::string file_content_string;
    file_content_string.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

    nlohmann::json j_objs;
    if(nlohmann::json::accept(file_content_string)) { // check json empty or invalid
      j_objs = nlohmann::json::parse(file_content_string);
      _VP_DebugPrint("accepted json: " + j_objs.dump());
    }
    else { // empty or invalid json, do not read
      _VP_ErrorPrint("empty or invalid db json file :" + db_path_ + " , load nothing");
      return false;
    }
    
    int index = 0;
    std::string variable_name;
    bool ret = true;
    std::apply([&](auto & ... variable_ref){
                (
                  (
                    variable_name = persist_variable_names_[index],
                    ret = ret && JsonGetToStrict<decltype(variable_ref)>(j_objs, variable_name, variable_ref),
                    index += 1
                  ),
                  ...
                );
              }, 
              persist_variables_refwrapper_);
    return ret;
  }

  // save variables to database
  // return - true: save to database success
  //          false: save to database fail
  bool DoSave() {
    nlohmann::json j_objs;
    int index = 0;
    std::lock_guard<std::mutex> lock(mutex_);
    try
    {
      std::apply([&](auto ... variable_ref){
                  (
                    (
                      j_objs[persist_variable_names_[index]] = nlohmann::json(variable_ref.get()),
                      index += 1
                    ), 
                    ...
                  );
                }, 
                persist_variables_refwrapper_);

    }
    catch (nlohmann::json::parse_error& ex)
    {
      std::cerr << "parse error at byte " << ex.byte << std::endl;
      return false;
    }

    std::ofstream outfile(db_path_.c_str());
    if (!outfile) {
      return false;
    }
    outfile << j_objs.dump(2);
    return true;
  }
};

} // namespace VariablePersistencer

#endif // _VARIABLE_PERSISTENCE_BY_REF_WRAPPER_H
