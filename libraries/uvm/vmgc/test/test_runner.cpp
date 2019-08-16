#include <iostream>
#include <string>
#include "vmgc/vmgc.h"

#define BOOST_TEST_MODULE VmGcTest
#include <boost/test/unit_test.hpp>

using namespace vmgc;

BOOST_AUTO_TEST_SUITE(vmgc_test_suite)

BOOST_AUTO_TEST_CASE(malloc_string_test)
{
	vmgc::GcState state;

	auto p = (std::string*) state.gc_malloc(sizeof(std::string));
	new (p)std::string("hello world");
	std::string* s = (std::string*)p;
	BOOST_CHECK(*s == "hello world");
	p->~basic_string();
	state.gc_free(p);

	auto p2 = (std::string*) state.gc_malloc_vector(sizeof(std::string), 10);
	for (int i = 0; i < 10; i++) {
		new (p2 + i)std::string(std::string("hello ") + std::to_string(i));
	}
	BOOST_CHECK(*(p2 + 9) == "hello 9");
	for (int i = 0; i < 10; i++) {
		(p2 + i)->~basic_string();
	}
}

struct GcString : vmgc::GcObject
{
	const static vmgc::gc_type type = 100;
	std::string value;

	virtual ~GcString() {
		std::cout << "gc string released " << value << std::endl;
	}
};

BOOST_AUTO_TEST_CASE(new_gc_object_test)
{
	vmgc::GcState state;

	auto p = state.gc_new_object<GcString>();
	p->value = "hello world";
	BOOST_CHECK(p->value == "hello world");
	state.gc_free(p);

	auto p2 = state.gc_new_object_vector<GcString>(10);
	for (int i = 0; i < 10; i++) {
		(p2 + i)->value = std::string("hello ") + std::to_string(i);
	}
	BOOST_CHECK((p2+9)->value == "hello 9");
	state.gc_free_array(p2, 10, sizeof(GcString));
}

BOOST_AUTO_TEST_CASE(gc_grow_vector)
{
	GcState state;
	size_t count1 = 0;
	auto p1 = (int64_t*) state.gc_grow_vector(nullptr, 0, &count1, sizeof(int64_t), INT32_MAX);
	BOOST_CHECK(count1 == GC_MINSIZEARRAY);
	for (size_t i = 0; i < count1; i++) {
		*(p1 + i) = i;
	}
	size_t count2 = count1;
	auto p2 = (int64_t*)state.gc_grow_vector(p1, count1, &count2, sizeof(int64_t), INT32_MAX);
	BOOST_CHECK(count2 == 2 * count1);
	for (size_t i = 0; i < count2; i++) {
		std::cout << "p2[" << i << "]=" << p2[i] << std::endl;
	}
	BOOST_CHECK(p2[3] == 3);

	// grow object vector test case
	size_t count3 = 0;
	auto p3 = state.gc_grow_object_vector<GcString>(nullptr, 0, &count3, INT32_MAX);
	BOOST_CHECK(count3 == GC_MINSIZEARRAY);
	for (size_t i = 0; i < count3; i++) {
		(p3 + i)->value = std::string("hello") + std::to_string(i);
	}
	size_t count4 = count3;
	auto p4 = state.gc_grow_object_vector(p3, count3, &count4, INT32_MAX);
	BOOST_CHECK(count4 == 2 * count1);
	for (size_t i = 0; i < count4; i++) {
		std::cout << "p4[" << i << "]=" << p4[i].value << std::endl;
	}
	BOOST_CHECK(p4[3].value == "hello3");

	state.gc_free_array(p4, count4, sizeof(GcString));
}

BOOST_AUTO_TEST_SUITE_END()
