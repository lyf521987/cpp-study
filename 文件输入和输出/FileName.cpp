#include<fstream>
using namespace std;

int main()
{
	ofstream out;
	out.open("Data.txt");
	out << "Hello, World!" << endl;
	return 0;
}
