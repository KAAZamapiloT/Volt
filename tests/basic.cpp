#include<Testmap.hpp>
#include<volt/JobSystems/ThreadPool.hpp>

void can(const std::string& S) {
	std::cout << S << "\n";
}


int main()
{
	volt::thread_pool pool(8);
	volt::map<int, std::string> mp;
	std::string s = "KK";

	for (int i = 0; i < 8; ++i) {
		s+= 'P';
		mp[i] = s;
		pool.submit(can, s);
	}
	

	pool.shutdown();

	for (int i = 0; i < 8; ++i) {
		auto val = mp.find(i);
		if (val) {
			std::cout << "Found: " << *val << "\n";
		}
		else {
			std::cout << "Not Found: " << i << "\n";
		}
	}
	
}