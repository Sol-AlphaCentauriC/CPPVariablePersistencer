// variable_persistencer_windows.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "json.hpp"
#include <iostream>
#define _ENABLE_VP_TEST 1
#include "variable_persistence_by_ref_wrapper.h"

class TestClassToSaveSub {
public:
	std::string string_in_class_sub;
	int int_in_class_sub;
	TestClassToSaveSub() = default;
	TestClassToSaveSub(std::string string, int num)
		: string_in_class_sub(string),
			int_in_class_sub(num) {}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(TestClassToSaveSub, string_in_class_sub, int_in_class_sub)
};

class TestClassToSave : public TestClassToSaveSub {
public:
	std::string string_in_class;
	TestClassToSaveSub sub_class;
	TestClassToSave() = default;
	TestClassToSave(int num, std::string string, std::string string_sub)
		: string_in_class(string),
			sub_class(string_sub, num) {}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(TestClassToSave, string_in_class, sub_class)
};

int main()
{

	TestClassToSave class_test = TestClassToSave(114, "test string content", "test string in sub class content");
	using PersistType = VariablePersistencer::VariablePersistencerByRefWrapper<TestClassToSave>;
	PersistType upload_persistence_db_ = PersistType(std::vector<std::string>({ GET_CXX_STRING(class_test) }), std::make_tuple(std::ref(class_test)), "./json_db.txt");
	std::cout << "saving variables to file" << std::endl;
	upload_persistence_db_.DoSave();
	std::cout << "loading file's variables in" << std::endl;
	upload_persistence_db_.DoLoad();
	std::cout << "test end, exiting" << std::endl;
	exit(0);
}
