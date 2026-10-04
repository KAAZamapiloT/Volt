#include<Testmap.hpp>
#include<volt/JobSystems/ThreadPool.hpp>
#include<volt/Containers/ObjectPool.hpp>

#include<volt/Containers/map.hpp>
void can(const std::string& S) {
	std::cout << S << "\n";
}



class obj {

public:
	obj(std::string d, int v) :d(d), v(v){}
	obj() :d(""), v(0) {}

	void print() {
		std::cout << d << " " << v << "\n";
	}
private:
	std::string d;
	int v;
};
	
	int main()
{
		volt::ObjectPool<obj> pool(10);
		obj* o = pool.allocate("hello", 1);
		o->print();

	
}