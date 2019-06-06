#include <iostream>

class TooBig {
public:
	int MyNumber;
	void* a;
	void* b;


	TooBig() = default;

	TooBig(const TooBig& p) = default;
	TooBig& operator=(const TooBig& p) = default;

	friend std::istream& operator>> (std::istream& is, TooBig& p)
	{
		return is;
	}
	friend std::ostream& operator<< (std::ostream& os, const TooBig& p)
	{
		return os;
	}
};

long GCounter = 0;

namespace leda {
int compare(const TooBig& p, const TooBig& q)
{
	GCounter++;
	if (p.MyNumber < q.MyNumber) return  -1;
	if (p.MyNumber > q.MyNumber) return   1;
	return 0;
}
}





class tree_bridge {

};












#include <LEDA/graph/graph.h>
#include <LEDA/graph/graph_gen.h>
#include <LEDA/graph/node_set.h>
#include <LEDA/core/p_queue.h>
#include <LEDA/core/dictionary.h>
#include <LEDA/core/impl/ab_tree.h>




using KeyType = TooBig;
using DataType = TooBig;

constexpr char nl = '\n';

int main() {
	leda::dictionary<KeyType, DataType, leda::ab_tree> test(2, 4);


	for (int i = 0; i < 8; i++) {
		test.insert({ i }, { i * 10 });
	}


	
	
	test.lookup({ 1 });

	std::cout << test.access({ 1 }).MyNumber << nl;

	std::cout << GCounter << nl;


}