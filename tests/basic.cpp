#include<Testmap.hpp>
#include<volt/JobSystems/ThreadPool.hpp>

void can(const std::string& S) {
	std::cout << S << "\n";
}


int main()
{
	volt::thread_pool pool(8);

	std::string s = "KK";

	for (int i = 0; i < 8; ++i) {
		s+= 'P';
		pool.submit(can, s);
	}

	pool.shutdown();
	
}