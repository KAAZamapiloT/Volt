
#include<SPSCqueue_test.hpp>
int main()
{
	volt::tests::run_spsc_suite();
	volt::SPSCQueueStatic<int, 1024> cc;
	int arr[] = { 1,2,2,23,3,3,3,3,12312,12,2,12,21,2,12,2,2,123 };
	cc.push_bulk(arr);
	std::cout << "\n WUTHNURTHBSBFHDVFHBVRG  \n";
	while (!cc.empty()) {
		int a;
		cc.pop(a);
		std::cout << a <<"\n";
	}

    return 0;
}